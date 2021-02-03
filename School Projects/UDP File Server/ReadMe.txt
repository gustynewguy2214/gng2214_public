NOTES: 

I used Eclipse C/C++, the gcc/g++ compilers, and the Snowflake SSH tool (recently renamed to Muon) and Cisco AnyConnect tools to make this project. It was created on Linux Mint 19.3, but should theoretically work for any UNIX-like system. I was however expecting this to be run on Eustis.

The client and server are both able to read any file type. If you choose to test a large file,  I would recommend enabling the debug flag.

Also, please go easy on the grading. Neither the instructions nor the rubric pdfs were as specific as I'd have liked them to be, and there are possibly 25515 unique cases for command profile 2 below! (If you want a derivation of this number, please send me a WebCourse message).

===============

INSTRUCTIONS:

The makefile was left unchanged. Compile the project like you would the example files.

You should be able to put the server and client shared libraries (the things being executed) anywhere you want. I recommend though having a directory called "client_folder" that contains your client library and having that be in the same directory as your server. It should look like this:

wherever
> server (shared library)
> client_folder
    > client (shared library)
  
To enable the DEBUG mode, add the "-d" (without quotes) argument to the end of the command line arguments.

To upload to the server instead of downloading from it, use the "-w" flag.

---

DOWNLOADING / UPLOADING A COMPLETE FILE uses the below command profile (profile 1):

client server-name [-w] file-name [-d]

where:
client      | the process, run as ./client
server-name | the server, such as localhost or eustis.eecs.ucf.edu
-w          | (Optional) changes the mode from file download to file upload
file-name   | the name of the file, which can either be a file in the same directory as
            | the client process or an absolute file path
-d          | (Optional) sets the DEBUG mode to 1

-------------

DOWNLOADING / UPLOADING A BYTE RANGE uses the below command profile (profile 2):

client server-name [-w] file-name -s START_BYTE -e END_BYTE [-d] 

where:
client      | the process, run as ./client
server-name | the server, such as localhost or eustis.eecs.ucf.edu
-w          | (Optional) changes the mode from file download to file upload
file-name   | the name of the file, which can either be a file in the same directory as
            | the client process or an absolute file path
-s          | Required flag used to determine the location of the start bytes
START_BYTE  | An integer value denoting which byte to start from. Must be greater than 0.
-e          | Required flag used to determine the location of the end bytes
END_BYTE    | An integer value denoting which byte to end at inclusive. Must be greater than the
            | start byte.
-d          | (Optional) sets the DEBUG mode to 1

-------------

If you fail to provide the correct number of arguments it should print the two command profiles above. If you don't remember what the fields are, refer either to this file or the assignment pdf.


