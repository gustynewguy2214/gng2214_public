/* This is the server code */
#include "file-server.h"
#include <sys/fcntl.h>

#define QUEUE_SIZE 10

int main(int argc, char *argv[]){

	//while(1){ //Server is always on.

	  int s = -1, b = -1, l = -1, sa = -1, on = 1;
	  char buf[BUF_SIZE];		/* buffer for outgoing file */
	  struct sockaddr_in channel;		/* holds IP address */

	  /* Build address structure to bind to socket. */
	  memset(&channel, 0, sizeof(channel));	/* zero channel */
	  channel.sin_family = AF_INET;
	  channel.sin_addr.s_addr = htonl(INADDR_ANY);
	  channel.sin_port = htons(SERVER_PORT);

	  /* Passive open. Wait for connection. */
	  s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);    /* create socket */
	  if (s < 0) fatal("socket failed");
	  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));

	  b = bind(s, (struct sockaddr *) &channel, sizeof(channel));
	  if (b < 0) fatal("bind failed");

	  l = listen(s, QUEUE_SIZE);		/* specify queue size */
	  if (l < 0) fatal("listen failed");

	  sa = accept(s, 0, 0);		/* block for connection request */
	  if (sa < 0) fatal("accept failed");

	  read(sa, buf, BUF_SIZE);		// Get the command mode. /* read file name from socket */

	  int DEBUG = 0;
	  int command_mode = -1;
	  //long upload_size = -1;

		int i = 0;
		char* p = strtok (buf, "|");
		char* tokens[6]; //The five tokens are: mode | file_name | start_byte | end_byte | debug | upload size (B)

		while (p != NULL){
			tokens[i++] = p;
			p = strtok (NULL, "|");
		}

		char* discard;

		command_mode = strtol(tokens[0],&discard,10);
		char* file_name = tokens[1];
		int start_byte = strtol(tokens[2],&discard,10);
		int end_byte = strtol(tokens[3],&discard,10);
		DEBUG = strtol(tokens[4],&discard,10);
		//upload_size = strtol(tokens[5],&discard,10);

		FILE *fileptr;
		char *buffer;
		long filelen;
		int j;

		int name_end = strlen(file_name)-1;
		int ptr = name_end;

		char* fname;

		if(strchr(file_name, '/') != NULL){ //File name has a path.
			while(file_name[ptr] != '/'){
				ptr--;

				if(ptr < 0){
					fname = "unknown_file.bin";
					break;
				}
			}

			if(ptr >= 0){
				int length = name_end-ptr;
				fname = (char*) malloc((length + 1) * sizeof(unsigned char));

				for(int i = 0; i < length; i++){
					ptr++;
					fname[i] = file_name[ptr];
				}
				fname[length] = '\0';
			}
		}
		else{					//The file is in the same location as the server.
			fname = file_name;
		}

		if(command_mode != 1 && command_mode != 3){ //Mode 1 is for complete upload, mode 3 for byte upload

			fileptr = fopen(file_name, "rb");

			//if(strstr(geterrno(), "No such") != NULL){
			if(fileptr == NULL){
				//perror("ERROR"); //Commented out because it causes a core dump. Not great.

				char* ACK = "File does not exist!";
				write(sa,ACK,strlen(ACK) + 1);

				printf("ERROR: The file %s does not exist on this server!\n",file_name);
				fatal("");
			}
			else{
				struct stat sta;
				stat(file_name, &sta);
				char sz[20];
				sprintf(sz, "%ld", sta.st_size);
				char* ACK = (char*) malloc(sizeof("exists_") + sizeof(sz));
				strcpy(ACK, "exists_");
				strcat(ACK,sz);
				write(sa,ACK,strlen(ACK) + 1);
			}

			fseek(fileptr, 0, SEEK_END);
			filelen = ftell(fileptr);
			rewind(fileptr);
			buffer = (char*) malloc((filelen+1) * sizeof(char));

			for(j = 0; j < filelen; j++) {
			   fread(buffer+j, 1, 1, fileptr);
			}

			struct sockaddr_in addr;
			char clientip[20];
			strcpy(clientip, inet_ntoa(addr.sin_addr));

			if(DEBUG == 1){
				printf("Sending <%s> to <%s>\n",file_name,clientip);
			}

			long start = 0;
			long end = filelen;

			if(command_mode == 2){
				start = start_byte-1; //Provide an offset due to 0 indexing.
				end = end_byte;
			}

			long length = end - start;

			long ten_percent = (long) myround(length / 10.0);
			int progress = 1;

			for(long k = start; k < end; k++){

				if(DEBUG == 1 && k == (progress * ten_percent)){
					printf("Sent %d%% of <%s>\n",(progress * 10),file_name);
					progress++;
				}

				char tmp[1] = {buffer[k]};
				write(sa, tmp, sizeof(unsigned char));
			}

			if(DEBUG == 1){
				printf("Finished sending <%s> to <%s>\n",file_name,clientip);
			}
			fclose(fileptr);
		}
		else if(command_mode == 1 || command_mode == 3){ //This is the upload.
			int bytes;
			FILE* write_ptr = NULL;

			if(fopen(fname,"r") != NULL){
				char* error_msg = "File with that name exists, overwriting.\n";
				write(sa, error_msg, strlen(error_msg) + 1);
			}

			write_ptr = fopen(fname,"wb");

			char* ACK = "Server ready for upload";
			write(sa,ACK,strlen(ACK) + 1);

			while (1) {
				unsigned char c[1];
				bytes = read(sa,c,1);

				if (bytes < 0){					//Check the read for errors.
					perror("ERROR");
					fatal("Upload failed.");
				}

				if(bytes == 0){ /* check for end of file */
					if(DEBUG == 1) printf("Upload complete.\n");
					fclose(write_ptr);
					exit(0);
				}

				fseek(write_ptr,0,SEEK_END);
				fwrite(c,sizeof(unsigned char),1,write_ptr);
			}
		}


	  close(sa);			/* close connection */

	}
//}
