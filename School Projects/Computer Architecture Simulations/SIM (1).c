/* EEL 4768 - 11/03/19
This program is written by: Joshua L. Nichols*/

	#include <limits.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <stdbool.h>
	#include <time.h>
	#include <math.h>

	#define EMPTY -1
	#define INIT_SIZE 16

	//
	//Input Variables
	//

	int assoc = -1;
	int repl_policy = -1;

	//
	//Other Variables
	//

	int block_size = 64;	//64B for the block size as per the instructions.

	FILE *trace_file;

	//Actual cache.
	bool **cache_dirty;
	long long **cache_tags; //number of rows will be number of sets, number of columns is associativity
	long long **cache_data;
	long long *lower_memory;
	int **LRU_metadata;	//Array I will use for LRU operations.

	//
	//Output Variables
	//

	int hits = 0;	//Since the miss ratio is just 1 - hit ratio, just keep track of hits.
	float miss_ratio = 0.00;
	int mem_writes = 0;	//Number of successful writes to cache.
	int mem_reads = 0;	//Number of successful reads from cache.

	int true_writes = 0;
	int true_reads = 0;

	int LRU_Accesses = 1;
	int FIFO_Accesses = 1;

	//This section of code is from Dr. Arup Guha's circularQueueNoRear.c file made available to Computer Science 1 students.
	struct queue {
		long* elements;
		int front;
		int numElements;
		int queueSize;
	};

	void init(struct queue* qPtr) {
		 qPtr->elements = (long*)malloc(sizeof(long)*INIT_SIZE);
		 qPtr->front = 0;
		 qPtr->numElements = 0;
		 qPtr->queueSize = INIT_SIZE;
	}

	int enqueue(struct queue* qPtr, long val) {
		//int i;
		if (qPtr->numElements != qPtr->queueSize) {
			qPtr->elements[(qPtr->front+qPtr->numElements)%qPtr->queueSize] = val;
			(qPtr->numElements)++;
			return 1;
		}
		else{
			return 0;
		}
	}

	long dequeue(struct queue* qPtr) {
		long retval;
		if (qPtr->numElements == 0)
			return EMPTY;
		retval = qPtr->elements[qPtr->front];
		qPtr->front = (qPtr->front + 1)% qPtr->queueSize;
		(qPtr->numElements)--;
		return retval;
	}

	int empty(struct queue* qPtr) {
		return qPtr->numElements == 0;
	}

	long front(struct queue* qPtr) {
		if (qPtr->numElements != 0)
			return qPtr->elements[qPtr->front];
		else
			return EMPTY;
	}

	////////////////////////////////// end of Guha code.

	long WB(char op, int set_num,long long tag_address, struct queue* FIFO_Q, char* address){

		//From the flowcharts:

		//Memory Request Comes In
		//Is it a read or write?

		//Reading:
		//Was it a cache hit?
		//YES:
			//Return that data.
			//You're done.
		//NO:
			//Locate a cache block to use
			//Is it "dirty"?
			//YES:
				//Write its previous data back to the lower memory.
			//NO:
				//Read data from lower memory into the cache block
				//Mark the cache block as "not dirty"
				//Return data

		//Writing:
		//Was it a cache hit?
		//YES:
			//Write that data into the cache block.
			//Mark the cache block as 'dirty'
		//NO:
			//Read data from lower memeory into the cache block
			//Write the new data into the cache block.
			//Mark the cache block as "Dirty"

		bool cache_hit = false;
		int index_hit = INT_MAX;

		long return_data = 0;

		//Calculate cache hit

		for (int i=0; i < assoc; i++)
			if(tag_address==cache_tags[set_num][i] && cache_data[set_num][i] == tag_address){

				hits++;
				cache_hit = true;
				index_hit = i;

				if(repl_policy == 0){

					LRU_metadata[set_num][i]++;	//Update its usage stat.

				}

				break;
			}

		if(op == 'R'){

			if(cache_hit == false){

				int index_to_replace = 0;
				long prev_data = 0;

				//Locate a cache block to use. (Likely depends on replace policy)

				if(repl_policy == 0){	//LRU

					int least_used = 0;

					for(int i = 0; i < assoc; i++){

						if(LRU_metadata[set_num][i] <= least_used){		//Find the least used to replace.
							least_used = LRU_metadata[set_num][i];
							index_to_replace = i;
						}
					}

					cache_tags[set_num][index_to_replace] = tag_address;
					cache_data[set_num][index_to_replace] = tag_address;
					LRU_metadata[set_num][index_to_replace] = 0;	//Reset the usage stat for this new data.

				}
				else if(repl_policy == 1){	//FIFO

					//Operates like a queue.

					long FIFO_Address = 0;
					int front = 0;

					if(FIFO_Q->numElements < assoc){

						enqueue(FIFO_Q,tag_address);

					}
					else{
						front = FIFO_Q->front;
						FIFO_Address = dequeue(FIFO_Q);
						enqueue(FIFO_Q,tag_address);
					}

					cache_tags[set_num][front] = FIFO_Address;

				}

				int mem_entrys = sizeof(lower_memory)/sizeof(long);

				int mem_index = -1;

				//Is the cache block dirty?
				if(cache_dirty[set_num][index_to_replace] == true){

					//Write the previous data to lower memory.

					true_writes++;

					prev_data = cache_data[set_num][index_to_replace];

					//Shove this data somewhere.
					int mem_tries = 0;
					for(int j = 0; j < mem_entrys; j++){
						if(lower_memory[j] == -1){
							mem_index = j;
							lower_memory[j] = prev_data;
						}
						else
							mem_tries++;
					}

					int randNum = rand() % mem_entrys;

					if(mem_tries >= mem_entrys){
						mem_index = randNum;
						lower_memory[randNum] = prev_data;
					}
				}

				//Read data from lower memory into cache block.
				true_reads++;
				if(mem_index == -1){

					for(int i = 0; i < sizeof(lower_memory)/sizeof(long); i++){

						if(lower_memory[i] == tag_address)
							mem_index = i;

					}
				}

				if(mem_index != -1){	//Can't read data from lower memory if its not there!

					cache_data[set_num][index_to_replace] = lower_memory[mem_index];	//Without any actual information, the tag_address will have to do.
					cache_tags[set_num][index_to_replace] = tag_address;

				}

				//Mark the cache block as "not dirty"

				cache_dirty[set_num][index_to_replace] = false;

				index_hit = index_to_replace;

			}

			//Return the data.
			return_data = cache_data[set_num][index_hit];

		}
		else if(op == 'W'){

			int index_to_replace = 0;
			long prev_data = 0;

			if(cache_hit == false){

				//Locate a cache block to use

				if(repl_policy == 0){	//LRU

					int least_used = 0;

					for(int i = 0; i < assoc; i++){

						if(LRU_metadata[set_num][i] <= least_used){		//Find the least used to replace.
							least_used = LRU_metadata[set_num][i];
							index_to_replace = i;
						}

					}

					cache_tags[set_num][index_to_replace] = tag_address;
					cache_data[set_num][index_to_replace] = tag_address;
					LRU_metadata[set_num][index_to_replace] = 0;	//Reset the usage stat for this new data.

				}
				else if(repl_policy == 1){	//FIFO

					//Operates like a queue.

					long FIFO_Address = 0;
					int front = 0;

					if(FIFO_Q->numElements < assoc){

						enqueue(FIFO_Q,tag_address);

					}
					else{
						front = FIFO_Q->front;
						FIFO_Address = dequeue(FIFO_Q);
						enqueue(FIFO_Q,tag_address);
					}

					cache_tags[set_num][front] = FIFO_Address;

				}

				int mem_entrys = sizeof(lower_memory)/sizeof(long);
				int mem_index = -1;

				//Is the cache dirty?
				if(cache_dirty[set_num][index_to_replace] == true){

					//Write the previous data to lower memory.

					true_writes++;
					prev_data = cache_data[set_num][index_to_replace];

					mem_writes++;

					//Shove this data somewhere.
					int mem_tries = 0;
					for(int i = 0; i < mem_entrys; i++){
						if(lower_memory[i] == -1){
							lower_memory[i] = prev_data;
							mem_index = i;
						}
						else
							mem_tries++;
					}

					int randNum = rand() % mem_entrys;

					if(mem_tries >= mem_entrys){
						mem_index = randNum;
						lower_memory[randNum] = prev_data;
					}
				}

				//Read data from lower memory into cache block.
				true_reads++;
				if(mem_index == -1){

					for(int i = 0; i < sizeof(lower_memory)/sizeof(long); i++){

						if(lower_memory[i] == tag_address)
							mem_index = i;

					}
				}

				if(mem_index != -1){	//Can't read data from lower memory if its not there!

					cache_data[set_num][index_to_replace] = lower_memory[mem_index];	//Without any actual information, the tag_address will have to do.
					cache_tags[set_num][index_to_replace] = tag_address;

				}
			}

			//Write the new data into the cache block.
			cache_data[set_num][index_to_replace] = tag_address;
			cache_tags[set_num][index_to_replace] = tag_address;

			//Mark the cache block as "Dirty"
			cache_dirty[set_num][index_to_replace] = true;

			return_data = 0;

		}

		return return_data;
	}

	long WT(char op, int set_num,long long tag_address, struct queue* FIFO_Q, char* address){

		//From the flowcharts:

		//Memory Request Comes In
		//Is it a read or write?

		//Reading:
		//Cache hit?
		//YES:
			//Return that data
		//NO:
			//Locate a cache block to use.
			//Read data from lower memory into the cache block.
			//REturn that data.

		//Writing:
		//Cache hit?
		//YES:
			//Write that data into a cache block.
			//Write that data into lower memory.

		//NO:
			//Write data into lower memory.

		bool cache_hit = false;

		int index_hit = 0;
		long return_data;

		//Calculate Cache Hit

		for (int i=0; i < assoc; i++)
			if(tag_address==cache_tags[set_num][i] && cache_data[set_num][i] == tag_address){

				hits++;
				cache_hit = true;
				index_hit = i;

				if(repl_policy == 0){

					LRU_metadata[set_num][i]++;	//Update its usage stat.

				}

				break;
			}

		//Is it a read or a write?
		if(op == 'R'){

			if(cache_hit == false){

				//Locate a cache block to use

				int index_to_replace = 0;
				//long prev_data = 0;

				if(repl_policy == 0){	//LRU

					int least_used = 0;

					for(int i = 0; i < assoc; i++){

						if(LRU_metadata[set_num][i] <= least_used){		//Find the least used to replace.
							least_used = LRU_metadata[set_num][i];
							index_to_replace = i;
						}
					}

					cache_tags[set_num][index_to_replace] = tag_address;
					cache_data[set_num][index_to_replace] = tag_address;
					LRU_metadata[set_num][index_to_replace] = 0;	//Reset the usage stat for this new data.

				}
				else if(repl_policy == 1){	//FIFO

					//Operates like a queue.

					long FIFO_Address = 0;
					int front = 0;

					if(FIFO_Q->numElements < assoc){

						enqueue(FIFO_Q,tag_address);

					}
					else{
						front = FIFO_Q->front;
						FIFO_Address = dequeue(FIFO_Q);
						enqueue(FIFO_Q,tag_address);
					}

					//printf("Spit out: [ %lu ]\n", FIFO_Address);
					cache_tags[set_num][front] = FIFO_Address;

				}

				mem_writes++;

				//Read data from lower memory into cache block.

				//true_reads++;

				int mem_entrys = sizeof(lower_memory)/sizeof(long);
				int mem_index = -1;

				if(mem_index == -1){

					for(int i = 0; i < mem_entrys; i++){

						if(lower_memory[i] == tag_address){
							mem_index = i;
						}
					}
				}

				if(mem_index != -1){	//Can't read data from lower memory if its not there!

					cache_data[set_num][index_to_replace] = lower_memory[mem_index];	//Without any actual information, the tag_address will have to do.
					cache_tags[set_num][index_to_replace] = tag_address;

					mem_reads++;

				}

				true_reads++;

			}

			//Return data.

			return_data = cache_data[set_num][index_hit];

		}
		else if(op == 'W'){

			if(cache_hit == true){

				//Write data into cache block.
				cache_data[set_num][index_hit] = tag_address;	//Lack of actual data to input makes this difficult.

			}

			//Write data into memory.

			mem_writes++;
			true_writes++;

			int mem_entrys = sizeof(lower_memory)/sizeof(long);

			//Shove this data somewhere.
			int mem_tries = 0;
			for(int i = 0; i < mem_entrys; i++){
				if(lower_memory[i] == -1){
					lower_memory[i] = tag_address;
				}
				else
					mem_tries++;
			}

			int randNum = rand() % mem_entrys;

			if(mem_tries >= mem_entrys){
				lower_memory[randNum] = tag_address;
			}
		}

		return return_data;

	}

	int main(int argc, char *argv[]){

		srand(time(0));

		/*
		//
		//Simulated Arguments
		//

		int cache_size = 32768;	//Size of simulated cache in bytes. atoi converts a string to integer.

		assoc = 8;						//The associativity of the cache. (1,2,4,8,etc.)

		repl_policy = 0;				//Replacement policy where:
											// - 0 should be LRU
											// - 1 should be FIFO (stack).

		int wb_policy = 0;					//Write-Back policy where:
											// - 0 should be Write-Through (all the way to memory).
											// - 1 should be Write-Back.

		//trace_file = fopen("C:\\Users\\Josh\\Desktop\\MINIFE.t","r");
		//trace_file = fopen("C:\\Users\\Josh\\Desktop\\XSBENCH.t","r");

		//trace_file = fopen("C:\\Users\\gusty\\Desktop\\MINIFE.t","r");
		//trace_file = fopen("C:\\Users\\gusty\\Desktop\\XSBENCH.t","r");
		*/

		//
		//Input variables
		//

		//Eclipse Mode

		//argv[0] is reserved for the program name.

		int cache_size = atoi(argv[1]);	//Size of simulated cache in bytes. atoi converts a string to integer.

		int assoc = atoi(argv[2]);			//The associativity of the cache. (1,2,4,8,etc.)

		int repl_policy = atoi(argv[3]);	//Replacement policy where:
											// - 0 should be LRU
											// - 1 should be FIFO (stack).

		int wb_policy = atoi(argv[4]);		//Write-Back policy where:
											// - 0 should be Write-Through (all the way to memory).
											// - 1 should be Write-Back.

		trace_file = fopen(argv[5],"r");	//Read the trace file.
											//Self-Reminder: The trace file is essentially a big .txt file.



		//
		// MAIN
		//

		if(trace_file == NULL){

			printf("ERROR: Could not read your trace file! The program will now exit for you to try again.");
			printf("\nRecieved: [ %s ]",argv[5]);
			printf("\nAll args: [ %s ] [ %s ] [ %s ] [ %s ] [ %s ] [ %s ]",argv[0],argv[1],argv[2],argv[3],argv[4],argv[5]);
			exit(-1);

		}
		else{

			int num_blocks = cache_size / block_size;	//Blocks are the basic unit of cache storage.
			int num_sets = num_blocks / assoc;			//The rows of the cache. Associativity is the columns.
			int set_num = 0;
			long long tag_address = 0;

			//Cache allocation
			cache_dirty = (bool **) malloc(sizeof(bool*) * num_sets);
			cache_tags = (long long **) malloc(block_size * num_sets);
			cache_data = (long long **) malloc(block_size * num_sets);
			lower_memory = (long long*) malloc(sizeof(long long) * num_sets);
			LRU_metadata = (int **) malloc(sizeof(int*) * num_sets);

			for(int i = 0; i < num_sets; i++){
				cache_dirty[i] = (bool*) malloc(sizeof(bool) * assoc);
				cache_tags[i] = (long long*) malloc(sizeof(long long) * assoc);
				cache_data[i] = (long long*) malloc(sizeof(long long) * assoc);
				LRU_metadata[i] = (int*) malloc(sizeof(int) * assoc);
			}

			//Cache Initializations:

			for(int i = 0; i < num_sets; i++){
				for(int j = 0; j < assoc; j++){
					cache_data[i][j] = 0;
					cache_tags[i][j] = 0;
					cache_dirty[i][j] = true;
					LRU_metadata[i][j] = 0;
				}
			}

			int mem_entrys = sizeof(lower_memory)/sizeof(long);

			for(int k = 0; k < mem_entrys; k++){

				lower_memory[k] = 0;

			}

			setbuf(stdout,NULL);	//NOTE: This line affects Eclipse-C only, and is used to ensure that print statements occur
									//before scan statements. If you have any questions, please send me a WebCourse message.

			printf("\nYour arguments:\n--------------\n");
			printf("[%d]", cache_size);
			printf("[%d]", assoc);
			printf("[%d]",repl_policy);
			printf("[%d]",wb_policy);
			printf("\n\n");

			long entry = 1;

			printf("Processing Entry:\n");

			while(trace_file && !feof(trace_file)){

				char address_str[20];
				char op_str[8];

				fscanf(trace_file,"%s %s",op_str,address_str);

				long long address = (long long) strtoll(address_str,NULL,16);
				char op = op_str[0];

				if(entry % 500000 == 0 && entry < 21174958){

					printf("\r");
					printf("%li",entry);

				}


				entry++;

				set_num = (address/block_size)%num_sets;
				tag_address= address/block_size;

				struct queue* FIFO_Q = (struct queue*)malloc(sizeof(struct queue));
				init(FIFO_Q);

				if(wb_policy == 0){	//Write through

					WT(op,set_num,tag_address,FIFO_Q,address_str);

				}
				else if(wb_policy == 1){	//Write back

					WB(op,set_num,tag_address,FIFO_Q,address_str);

				}
			}

			true_reads--; //Account for the possibility of an empty line at the end.

			float true_ratio = 0.00;

			if(true_reads != 0 || true_writes != 0){
				true_ratio = (float) (1- ((float) true_reads / (float) (true_writes + true_reads)));
			}
			else{
				true_ratio = 1;
			}

			printf("\n\n");
			printf("-----------------------\n");
			printf("\tOutput\t\n");
			printf("-----------------------\n");
			printf("Miss Ratio: %f\n",true_ratio);
			printf("# of Writes: %d\n",true_writes);
			printf("# of Reads: %d\n",true_reads);
			printf("\n\n");

			//Free the memory.

			for(int i = 0; i < num_sets; i++){
				free(cache_dirty[i]);
				free(cache_tags[i]);
				free(cache_data[i]);
				free(LRU_metadata[i]);
			}

			free(cache_dirty);
			free(cache_tags);
			free(cache_data);
			free(lower_memory);
			free(LRU_metadata);

			fclose(trace_file);

		}
	}






