/* This page contains a client program that can request a file from the server program
 * on the next page. The server responds by sending the whole file.
 */

#include "file-server.h"

//Client must:
/*
 * Client writes recieved bytes to a file with the name identified by the client's user.
 *
 * Client must write hte bytes to the file in order and without introducing any other characters or bytes.
 *
 * Must be able to sucessfully write a binary file recieved from the server.
 *
 * Allow the client to add arguments that specify a byte range instead of requesting to download the whole file.
 *  The usage of byte range should be as: "client server-name file-name -s START_BYTE -e END_BYTE (can range from 1 to anything)
 *  If given 3 and 11, gets all bytes from 3-11 respectively.
 *  The byte flags are not mandatory, and the user can just ask for the whole file.
 *
 * Add a client flag -w that allows the file to be written to the server.
 *  When the client is being executed from the command prompt, the flag will optionally be available to the user.
 *  If the -w flag is given, the server should interpret that as the file will be uploaded/written to hte server side.
 *  Note that the server should check if the file exists in its local directory first before it allows the client to send the file to the server side.
 *
 * Show help message if client gives wrong argument or fewer arguments. Essentially printing some sample commands you're expecting.
 */

//Commands to support:
/*
 * client server-name file-name | get the file from the server.
 * client server-name file-name -s START_BYTE -e END_BYTE
 * client server-name [-w] file-name | Server should interpret as uploading to server side.
 */

//Using a mode system:
//Mode 0: Regular download
//Mode 1: Regular upload
//Mode 2: Download byte range
//Mode 3: Upload byte range

int main(int argc, char **argv)
{
  int c, s, bytes;
  //char buf[BUF_SIZE];		/* buffer for incoming file */
  struct hostent *h;		/* info about server */
  struct sockaddr_in channel;		/* holds IP address */


  char* file_name;

  int command_mode = -1;

  int start_byte = -1, end_byte = -1;

  int DEBUG = 0;

  char* server;

  if(argc == 3){ //Get the file regularly.
	  file_name = argv[2];	//Okay for this to directly be a file or an absolute path to one.

	  command_mode = 0;
  }
  else if(argc == 4){
	  char* write_arg = argv[2];

	  if(strcmp(write_arg, "-w") != 0){ //Download file normally with debug.
		  file_name = argv[2];

		  if(strcmp(argv[3], "-d") != 0){
			  printf("ERROR: Bad option flag: %s\n",argv[3]);
			  fatal("Expected command usage: client server-name [-w] file-name [-d]");
		  }

		  DEBUG = 1;
		  command_mode = 0;
	  }
	  else{			//Upload file to server without debug.
		  server = argv[1];

		  //Check if the argument is there.
		  char* write_arg = argv[2];

		  if(strcmp(write_arg, "-w") != 0){
			  printf("ERROR: Bad option flag: %s\n",write_arg);
			  fatal("Expected command usage: client server-name [-w] file-name [-d]");
		  }

		  //Get the file name.
		  file_name = argv[3];

		  command_mode = 1;
	  }
  }
  else if(argc == 5){ //Write the file to the server with debug.

	  server = argv[1];

	  //Check if the argument is there.
	  char* write_arg = argv[2];

	  if(strcmp(write_arg, "-w") != 0){
		  printf("ERROR: Bad option flag: %s\n",write_arg);
		  fatal("Expected command usage: client server-name [-w] file-name [-d]");
	  }

	  //Get the file name.
	  file_name = argv[3];

	  if(strcmp(argv[4], "-d") != 0){
		  printf("ERROR: Bad option flag: %s\n",argv[4]);
		  fatal("Expected command usage: client server-name [-w] file-name [-d]");
	  }

	  DEBUG = 1;
	  command_mode = 1;
  }
  else if(argc == 7){ //Get only the byte range specified.

	  server = argv[1];
	  file_name = argv[2];

	  char* discard; //Theoretically, there shouldn't be anything in this buffer, as the arguments are already parsed!

	  char* s = argv[3];
	  start_byte = strtol(argv[4],&discard,10); //atoi is supposedly not being used anymore
	  char* e = argv[5];
	  end_byte = strtol(argv[6],&discard,10);

	  int err = 0;

	  if((strcmp(s, "-s") == 0)){
		  if(start_byte > 0){
			  if((strcmp(e,"-e") == 0)){
				  if(end_byte >= start_byte){
					  command_mode = 2;
				  }
				  else{
					  printf("ERROR: End byte must be greater than or same value as the start byte.\n");
					  err = 1;
				  }
			  }
			  else{
				  printf("ERROR: Expected end byte flag.\n");
				  err = 1;
			  }
		  }
		  else{
			  printf("ERROR: Start byte must be greater than 0.\n");
			  err = 1;
		  }
	  }
	  else{
		  printf("ERROR: Expected start byte flag.\n");
		  err = 1;
	  }

	  if(err == 1){
		  fatal("Expected command usage: client server-name [-w] file-name -s START_BYTE -e END_BYTE [-d]");
	  }
  }
  else if(argc == 8){

	  server = argv[1];
	  char* write_arg = argv[2];

	  if(strcmp(write_arg, "-w") == 0){	//Upload argument is there

		  file_name = argv[3];

		  char* discard; //Theoretically, there shouldn't be anything in here, as the arguments are already parsed!

		  char* s = argv[4];
		  start_byte = strtol(argv[5],&discard,10); //atoi is supposedly not being used anymore
		  char* e = argv[6];
		  end_byte = strtol(argv[7],&discard,10);

		  int err = 0;

		  if((strcmp(s, "-s") == 0)){
			  if(start_byte > 0){
				  if((strcmp(e,"-e") == 0)){
					  if(end_byte >= start_byte){
						  command_mode = 3;
					  }
					  else{
						  printf("ERROR: End byte must be greater than or same value as the start byte.\n");
						  err = 1;
					  }
				  }
				  else{
					  printf("ERROR: Expected end byte flag.\n");
					  err = 1;
				  }
			  }
			  else{
				  printf("ERROR: Start byte must be greater than 0.\n");
				  err = 1;
			  }
		  }
		  else{
			  printf("ERROR: Expected start byte flag.\n");
			  err = 1;
		  }

		  if(err == 1){
			  fatal("Expected command usage: client server-name [-w] file-name -s START_BYTE -e END_BYTE [-d]");
		  }
	  }
	  else{		//Partial download with debug on.
		  file_name = argv[2];

		  char* discard; //Theoretically, there shouldn't be anything in here, as the arguments are already parsed!

		  char* s = argv[3];
		  start_byte = strtol(argv[4],&discard,10); //atoi is supposedly not being used anymore
		  char* e = argv[5];
		  end_byte = strtol(argv[6],&discard,10);

		  int err = 0;

		  if((strcmp(s, "-s") == 0)){
			  if(start_byte > 0){
				  if((strcmp(e,"-e") == 0)){
					  if(end_byte >= start_byte){
						  if(strcmp(argv[7], "-d") == 0){
							  DEBUG = 1;
							  command_mode = 2;
						  }
						  else{
							  printf("ERROR: Bad option flag: %s\n",argv[7]);
							  err = 1;
						  }
					  }
					  else{
						  printf("ERROR: End byte must be greater than or same value as the start byte.\n");
						  err = 1;
					  }
				  }
				  else{
					  printf("ERROR: Expected end byte flag.\n");
					  err = 1;
				  }
			  }
			  else{
				  printf("ERROR: Start byte must be greater than 0.\n");
				  err = 1;
			  }
		  }
		  else{
			  printf("ERROR: Expected start byte flag.\n");
			  err = 1;
		  }

		  if(err == 1){
			  fatal("Expected command usage: client server-name [-w] file-name -s START_BYTE -e END_BYTE [-d]");
		  }
	  }
  }
  else if(argc == 9){	//Partial upload with debug messages on.

	  server = argv[1];
	  char* write_arg = argv[2];

	  file_name = argv[3];

	  char* discard; //Theoretically, there shouldn't be anything in here, as the arguments are already parsed!

	  char* s = argv[4];
	  start_byte = strtol(argv[5],&discard,10); //atoi is supposedly not being used anymore
	  char* e = argv[6];
	  end_byte = strtol(argv[7],&discard,10);

	  char* debug = argv[8];

	  int err = 0;

	  if(strcmp(write_arg, "-w") == 0){	//Upload argument is there.
		  if((strcmp(s, "-s") == 0)){
			  if(start_byte > 0){
				  if((strcmp(e,"-e") == 0)){
					  if(end_byte > 0){
						  if(end_byte >= start_byte){
							  if(strcmp(debug, "-d") == 0){
								  DEBUG = 1;
								  command_mode = 3;
							  }
							  else{
								  printf("ERROR: Bad option flag: %s\n",debug);
								  err = 1;
							  }
						  }
						  else{
							  printf("ERROR: End byte must be greater than or same value as the start byte.\n");
							  err = 1;
						  }
					  }
					  else{
						  printf("ERROR: Start byte must be greater than 0.\n");
						  err = 1;
					  }
				  }
				  else{
					  printf("ERROR: Expected end byte flag.\n");
					  err = 1;
				  }
			  }
			  else{
				  printf("ERROR: Start byte must be greater than 0.\n");
				  err = 1;
			  }
		  }
		  else{
			  printf("ERROR: Expected start byte flag.\n");
			  err = 1;
		  }
	  }
	  else{
		  //Error, argument not there.
		  printf("ERROR: Bad option flag: %s\n",write_arg);
		  err = 1;
	  }

	  if(err == 1){
		  fatal("Expected command usage: client server-name [-w] file-name -s START_BYTE -e END_BYTE [-d]");
	  }
  }
  else{
	  printf("ERROR: No command profile matched. Expected command profiles:\n");
	  printf("client server-name [-w] file-name [-d] | Download specified file from server, or if -w is provided, upload to server.\n");
	  printf("client server-name [-w] file-name -s START_BYTE -e END_BYTE [-d]| Download the specified byte range from file on server, or if -w is provided, upload to server.\n");
	  fatal("To use debug, specify with the (-d) flag.\n");
  }

  //All variations of the command require the host to be correct,
  //so just do the check here for all of them.
  h = gethostbyname(argv[1]); /* look up host's IP address */
  if (!h) fatal("ERROR: Could not resolve host name.\n");

  s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s < 0){
	  perror("ERROR (Socket)");
	  fatal("");
  }
  memset(&channel, 0, sizeof(channel));
  channel.sin_family= AF_INET;
  memcpy(&channel.sin_addr.s_addr, h->h_addr, h->h_length);
  channel.sin_port= htons(SERVER_PORT);

  c = connect(s, (struct sockaddr *) &channel, sizeof(channel));
  if (c < 0){
	  perror("ERROR (Connect)");
	  fatal("");
  }

  int end = strlen(file_name)-1;
  int ptr = end;

  char* fname;

  if(strchr(file_name, '/') != NULL){	//The name has a path associated.
	  while(file_name[ptr] != '/'){
		  ptr--;

		  if(ptr < 0){
			  fname = "unknown_file.bin";
			  break;
		  }
		}

		if(ptr >= 0){
		  int length = end-ptr;
		  fname = (char*) malloc((length + 1) * sizeof(unsigned char));

		  for(int i = 0; i < length; i++){
			  ptr++;
			  fname[i] = file_name[ptr];
		  }
		  fname[length] = '\0';
		}
  }
  else{	//There is no slash, can use name directly.
	  fname = file_name;
  }

  FILE* write_ptr = NULL;

  	char command_num[2];
	sprintf(command_num, "%d", command_mode);

	char s_byte[10];
	sprintf(s_byte, "%d", start_byte);

	char e_byte[10];
	sprintf(e_byte, "%d", end_byte);

	char debug[2];
	sprintf(debug, "%d", DEBUG);

	struct stat st;
	stat(fname, &st);

	//Commented out because an incorrect size can cause an allocation error. Can't fix right now.
	//char size[20];	//Should be able to accomodate any file < 999 GB large.
	//sprintf(size, "%ld", st.st_size);

	//Create a "packet" of variables to configure the server.
	char* my_args = (char*) malloc(sizeof(command_num) + 1 + sizeof(file_name) + 1 + sizeof(s_byte) + 1 + sizeof(e_byte) + 1 + sizeof(debug));// + 1 + sizeof(size));
	strcpy(my_args, command_num);
	strcat(my_args,"|");
	strcat(my_args,file_name);
	strcat(my_args,"|");
	strcat(my_args,s_byte);
	strcat(my_args,"|");
	strcat(my_args,e_byte);
	strcat(my_args,"|");
	strcat(my_args,debug);
	//strcat(my_args,"|");
	//strcat(my_args,size);

	int write_val = 0;

	/* Connection is now established. Send packet including the file name and a null byte. */
	write_val = write(s,my_args,strlen(my_args) + 1);

	if(write_val < 0){
		printf("Server setup failed.\n");
		perror("ERROR");
		fatal("");
	}

  /* Go get the file and write it to standard output. */

  if(command_mode == 0 || command_mode == 2){ //This section is for mode 0 - full file download and mode 2 - partial file download.

	char buf[BUF_SIZE];
	long sz = 0;

	while(1){
		bytes = read(s,buf, BUF_SIZE);

		if(strstr(buf, "File does not exist!") != NULL){	//Handle the Error
			printf("ERROR: File does not exist.\n");
			fatal("");
		}

		if(strstr(buf, "exists") != NULL){	//Handle the ACK
			char* szz = buf;
			char* discard;

			int end = strlen(szz)-1;
			int ptr = end;

			int len = 0;

			while(szz[ptr] != '_'){
			  ptr--;

			  if(ptr < 0){
				  break;
			  }
			}

			if(ptr >= 0){
			  len = end-ptr;
			}

			char* token = (char*) malloc(sizeof(char) * (len + 1));

			for(int i = 0; i < len; i++){
				 ptr++;
				 token[i] = szz[ptr];
			}
			token[len] = '\0';

			sz = strtol(token,&discard,10);

			break;
		}

		printf("%s\n",buf);

	}

	write_ptr = fopen(fname,"wb");
	  printf("Recieving file...\n");

	  struct stat sta;

	  while (1) {
		unsigned char c[1];
		bytes = read(s,c,1);

		if (bytes < 0){
			printf("Download failed.\n");
			perror("ERROR");
			fatal("");		/* check for end of file */
		}

		if(bytes == 0){

			stat(fname, &sta);

			if(sta.st_size == (sz-1)){
				printf("Download complete.\n");
			}
			else{
				printf("File partially downloaded. If this was not intentional, the server likely stopped sending.\n");
				printf("If your file is complete, please ignore this message.\n");
			}

		    if(write_ptr != NULL){
		 	    fclose(write_ptr);
		    }
			exit(0);
		}

		fseek(write_ptr,0,SEEK_END);
		fwrite(c,sizeof(unsigned char),1,write_ptr);
	  }
  }
  else if(command_mode == 1 || command_mode == 3){
	FILE* fileptr;
	char* buffer;
	long filelen;
	long j;

	fileptr = fopen(file_name, "rb"); //Need the fully qualified path for upload purposes.

	fseek(fileptr, 0, SEEK_END);
	filelen = ftell(fileptr);
	rewind(fileptr);
	buffer = (char* )malloc((filelen+1)*sizeof(char));

	long sb = 0;
	long eb = filelen;

	if(command_mode == 3){
		sb = start_byte - 1;
		eb = end_byte;
	}

	for(j = 0; j < filelen; j++) {
	   fread(buffer+j, 1, 1, fileptr);
	}

	char buf[BUF_SIZE];

	while(1){
		bytes = read(s,buf, BUF_SIZE);

		if(strstr(buf, "Server ready for upload") != NULL){	//Handle the ACK
			break;
		}

		printf("%s\n",buf);

	}

	long length = filelen;

	if(command_mode == 3){
		length = eb - sb;
	}

	long ten_percent = (long) myround(length / 10.0);
	int progress = 1;

	for(long k = sb; k < eb; k++){

		if(DEBUG == 1 && k == (progress * ten_percent)){
			printf("Uploaded %d%% of <%s> to server.\n",(progress * 10),file_name);
			progress++;
		}

		char tmp[1] = {buffer[k]};
		write_val = write(s, tmp, sizeof(unsigned char));

		if(write_val < 0) perror("ERROR");
	}

	if(DEBUG == 1){
		printf("Finished sending <%s> to <%s>\n",file_name,server);
	}
	else{
		printf("Upload complete.\n");
	}
	fclose(fileptr);

  }


}
