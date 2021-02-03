/* EEL 4768 - 11/17/19
This program is written by: Joshua L. Nichols*/

	#include <limits.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <stdbool.h>
	#include <math.h>
	
	FILE *trace_file;
	
	int M; //lower part of program counter bits.
	int N; //GBH register bits size.
	
	int* prediction_table;
	char* GBH;
	
	int correct_predictions = 0;
	
	float misprediction_rate;

	///////////////////
	//From: https://www.quora.com/Is-there-a-function-in-C-that-converts-an-integer-into-bits
	#define BitSet(number,x) ((number >> x) & 1U);
 
	char* int2bin(int n) {
		int i, j;
		int nbits = sizeof(n)/4 * CHAR_BIT;
		int spaces = 0;//nbits / 4 - 1;
		int totalLen = nbits + spaces;
		char *s = (char*)calloc(totalLen + 1, 1); 
		memset(s, '0', totalLen);
	 
		for (i = 0, j = totalLen -1; i < nbits; i++, j--){
			bool result =  BitSet((unsigned)n, i);
			if (result)
				s[j] = '1';
		}
		return s;
	}
	///////////////////
	
	///////////////////
	//From: https://stackoverflow.com/questions/5901181/c-string-append
	char* str_combine(char* str1, char* str2){
		char* new_str;
		if((new_str = malloc(strlen(str1)+strlen(str2)+1)) != NULL){
			new_str[0] = '\0';   // ensures the memory is an empty string
			strcat(new_str,str1);
			strcat(new_str,str2);
		}
		return new_str;
	}
	///////////////////


	int main(int argc, char *argv[]){
		
		M = atoi(argv[1]);
		N = atoi(argv[2]);
		trace_file = fopen(argv[3],"r");
		
		char address_str[20];
		char taken_str[3];

		printf("\n");
		
		if(trace_file == NULL){

			printf("ERROR: Could not read your trace file! The program will now exit for you to try again.\n");
			system("pause");
			exit(-1);

		}
		else{
			
			int size = pow(2,M);
			
			prediction_table = (int*) malloc(sizeof(int) * size);
			GBH = (char*) malloc(sizeof(char)*(N+1));

			for(int i = 0; i < size; i++){
				
				prediction_table[i] = 2;
				
			}
			
			for(int j = 0; j < N; j++){
				
				GBH[j] = '0';
				
			}
			GBH[N] = '\0';

			printf("Processing Entry:\n");
			
			long entry = 0;
			
			while(trace_file && !feof(trace_file)){

				if(N < 0 || M < 0){
					
					printf("ERROR: Neither M nor N can be negative! The program will now exit for you to try again.\n");
					system("pause");
					exit(-1);
					
				}

				if(entry % 500000 == 0){

					printf("\r");
					printf("%li",entry);
					
				}
				entry++;

				fscanf(trace_file,"%s %s",address_str,taken_str);
				
				long address = (long) strtol(address_str,NULL,16);
				long shift_address = address >> 2;
				char taken = taken_str[0];

				char* shft_addr_bin = int2bin(shift_address);
				char* addr_bin_short = shft_addr_bin + (strlen(shft_addr_bin) - M);
				int short_addr = (int) strtol(addr_bin_short, NULL, 2);

				int GBH_num = ((int) strtol(GBH,NULL,2)) << (M-N);

				int index = short_addr^GBH_num;

				if(taken == 'T' || taken == 't'){
					
					if(prediction_table[index] == 2 || prediction_table[index] == 3) correct_predictions++;
					
					if(prediction_table[index] >= 0 && prediction_table[index] < 3){
						
						prediction_table[index]++;
						
					}
					
					char* newGBH = (char*) malloc(sizeof(GBH)+sizeof(char));
					
					newGBH[0] = '1';
					for(int i = 1; i < strlen(GBH); i++){
						
						newGBH[i] = GBH[i-1];
						
					}
					newGBH[strlen(GBH)] = '\0';
					
					GBH = newGBH;
					
				}
				else if(taken == 'N' || taken == 'n'){
					
					if(prediction_table[index] == 0 || prediction_table[index] == 1) correct_predictions++;
					
					if(prediction_table[index] > 0 && prediction_table[index] <= 3){
						
						prediction_table[index]--;
						
					}
					
					char* newGBH = (char*) malloc(sizeof(GBH)+sizeof(char));
					newGBH[0] = '0';
					for(int i = 1; i < strlen(GBH); i++){
						
						newGBH[i] = GBH[i-1];
						
					}
					newGBH[strlen(GBH)] = '\0';
					
					GBH = newGBH;
					
				}

			}
			
			misprediction_rate = (1 - ((float) correct_predictions / (float) entry)) * 100;
			
			printf("\n\nOutput Variables:\n\n");
			printf("M: [ %d ]\n",M);
			printf("N: [ %d ]\n",N);
			printf("Misprediction Ratio: [ %.2f%% ]\n\n\n",misprediction_rate);
			
		}
	}