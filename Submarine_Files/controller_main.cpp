// Client side implementation of UDP client-server model
#include <arpa/inet.h>
#include <ctime>
#include <iostream>
#include <math.h>
#include <opencv2/opencv.hpp>
#include <pthread.h>
#include <string>
#include <string.h>
#include <sys/socket.h>             // for socket stuff.
#include <sys/time.h>               // for gettimeofday()
#include <unistd.h>
#include <strings.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <vector>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include "udpPacket_1.h"
#include <stdio.h>

using namespace cv;
using namespace std;

int port1 = 6000;
int port2 = 6005;
#define PORT 6010
char* sub_address = "192.168.1.10";

#define MAXLINE 1024

bool vidinitdone = false;
bool cominitdone = false;
bool data_on_video = false;

#define SEC_TO_US(sec) ((sec)*1000000)
#define NS_TO_US(ns) ((ns)/1000)

//See stackoverflow.com/questions/5833094/get-a-timestamp-in-c-in-microseconds
uint64_t micros(){
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_TO_US((uint64_t)ts.tv_sec) + NS_TO_US((uint64_t)ts.tv_nsec);
    return us;
}

int openFifo()
{
	string fifoName = "/home/pi/Desktop/mxButtonFifo";
	int fd = open(fifoName.c_str(), O_RDONLY);

	if (fd < 0 )
    {
		printf("ERROR: opening FIFO for MX Button!\n");
		return -1;
	}

	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
    {
		printf("ERROR: setting FIFIO for MX Button to non-blocking!\n");
		return -1;
	}
	return fd;
}

bool readFifo(int fd)
{
	bool retval = false;
	char buf[12];
	string bufStr;
	int nread = read(fd, buf, 12);
	switch (nread)
	{
		case -1:
			// case -1 means pipe is empty and errono set = EAGAIN
            if (errno != EAGAIN)
			{
                perror("read");
            }
            break;

        case 12:
            cout << buf << endl;
            if (string(buf).find("MX_Button 1") != string::npos)
            {
				retval = true;
			}
            break;

        default:
            //printf("%d bytes were read\n", nread);
            break;
    }
	return retval;
}

string distTxt    = "Distance: ";
string depthTxt   = "Depth: ";
string tempTxt    = "Temperature: ";
string bladderTxt = "Bladder: ";
string waterTxt   = "Water: ";
string lightTxt = "Lights: ";
string pressTxt = "Pressure: ";

bool showSensors = false;
bool showTime = false;


void* vidthread(void* v){

    int sockfd;
    char buffer[MAXLINE];
    char *hello = "Hello from client vid";
    struct sockaddr_in     servaddr;

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("vid socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port2);
    servaddr.sin_addr.s_addr = inet_addr(sub_address);

    srand((unsigned) time(0));

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;

    //Let the controller be the one to time out on recieve, as we want the commands to be
    //executed as soon as possible on the sub.
    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
        printf("vid setsockopt failed\n");

    int n;
    socklen_t len = sizeof(servaddr);

    // open MX_Button FIFO
    int fifoFD = openFifo();

    string controller_side_name = "/home/josh/Desktop/ntsc2.png";
    Mat image2 = imread(controller_side_name);

     // Check for failure
     if (image2.empty()){
      cout << "Could not open or find the image. Who cares?" << endl;
     }

     bool debug = false;

    int rows2 = image2.rows;
    int cols2 = image2.cols;
    int sz2 = image2.elemSize();
    int expected_len = rows2 * cols2 * sz2;

     if(debug){
        rows2 = image2.rows;
        cols2 = image2.cols;
        sz2 = image2.elemSize();
        expected_len = rows2*cols2*sz2;
     }
     else{
        rows2 = 480;
        cols2 = 800;
        sz2 = 3;
        expected_len = rows2*cols2*sz2;
     }

    int packet_count2 = 1;
    int packet_size2 = 0;
    int packets_needed2 = 0;
    int kb2 = 1024;
    int last_int_packets2 = 0;

    while(packet_count2 < 65){

        packet_size2 = kb2 * packet_count2;

        if(expected_len%(packet_size2) == 0) last_int_packets2 = expected_len/(packet_size2);
        if(last_int_packets2 < 65) break;
        packet_count2++;

    }

    packets_needed2 = (expected_len/(kb2*last_int_packets2));
    packet_size2 = kb2*last_int_packets2;

    cout << "C: For the image [" << controller_side_name << "] (" << rows2 << "*" << cols2 << "*" << sz2 << ")" << endl;
    cout << "C: [" << packets_needed2 << "] packets of size: [" << packet_size2 << "] bytes (" << last_int_packets2 << " KB) are needed." << endl;

    Mat rebuild = Mat(rows2,cols2,CV_8UC3);

    unsigned char* rebuilddata = (unsigned char*) calloc(expected_len, sizeof(unsigned char));
    unsigned char* recvpackandnum = (unsigned char*) calloc(packet_size2 + 6, sizeof(unsigned char));

    int offset;

    struct timeval t1, t2;
    double elapsedTime;

    vidinitdone = true;

    string fps = " fps";
    string tmp;
    double d;

    Point p = Point(10,rebuild.rows/10);

    uint64_t nowtime, rectime, remaining_time;

    timeval tv1;

    struct sockaddr_in subAddr;
    int subAddrLen = sizeof(subAddr);

    while(true){
        sendto(sockfd, (const char *)hello, strlen(hello),
            MSG_CONFIRM, (const struct sockaddr *) &servaddr,
                sizeof(servaddr));

        //printf("S_V: Hello message sent.\n");

        //cout << "Waiting on vid socket" << endl;
        /*
        n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                    MSG_WAITALL, (struct sockaddr *) &servaddr,
                    &len);
        buffer[n] = '\0';
        */

        n = recvfrom(sockfd, recvpackandnum, packet_size2+6,
                    MSG_WAITALL, (struct sockaddr *) &servaddr,
                    &len);

        if(recvpackandnum[0] == '$' && recvpackandnum[1] == '$'){

            offset = ((recvpackandnum[2] - '0')*10) + (recvpackandnum[3] - '0');

            if(offset == 0){ gettimeofday(&t1, NULL); }

            memcpy(rebuilddata+(offset*packet_size2), recvpackandnum+6,packet_size2*sizeof(unsigned char));

            if(offset == packets_needed2-1){

                //cout << "Last packet found" << endl;
                rebuild.data = rebuilddata;

                gettimeofday(&t2, NULL);

                elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
                elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms

                //cout << "Got here 1" << endl;

                //printf("\t%f fps.\n", 1000/elapsedTime);

                // check MX_BUTTON fifo
                if (readFifo(fifoFD))
                {
					// toggle bool flag to display data on video
					data_on_video = !data_on_video;
				}

                if (showSensors == false && showTime == false && data_on_video)
                {
					string tmp1 = to_string(1000/elapsedTime) + fps;
					putText(rebuild,tmp1,p,FONT_HERSHEY_DUPLEX,1.0,CV_RGB(118,185,0),2);

				}

				if(data_on_video == false && showSensors == false && showTime){

                    time_t now = time(0);
                    char* dt = ctime(&now);

                    string tmp2(dt);

                    putText(rebuild, tmp2,p,FONT_HERSHEY_DUPLEX, 1.0, CV_RGB(118,185,0),2);

				}

				if(showTime == false && data_on_video == false && showSensors){

                     Point a = Point(10,rebuild.rows/15);
					 putText(rebuild,distTxt,a,FONT_HERSHEY_DUPLEX,0.65,CV_RGB(118,185,0),2);
					 Point b = Point(10,(rebuild.rows*2)/15);
					 putText(rebuild,depthTxt,b,FONT_HERSHEY_DUPLEX,0.65,CV_RGB(118,185,0),2);
					 Point c = Point(10,(rebuild.rows*3)/15);
					 putText(rebuild,tempTxt,c,FONT_HERSHEY_DUPLEX,0.65,CV_RGB(118,185,0),2);
					 Point d = Point(10,(rebuild.rows*4)/15);
					 putText(rebuild,lightTxt,d,FONT_HERSHEY_DUPLEX,0.65,CV_RGB(118,185,0),2);
					 Point e = Point(10,(rebuild.rows*5)/15);
					 putText(rebuild,pressTxt,e,FONT_HERSHEY_DUPLEX,0.65,CV_RGB(118,185,0),2);


					// p.y += 1;	// move to new row
					// putText(rebuild,bladderTxt,p,FONT_HERSHEY_COMPLEX,1.0,CV_RGB(118,185,0),2);
					// p.y += 1;	// move to new row
					// putText(rebuild,bladderTxt,p,FONT_HERSHEY_COMPLEX,1.0,CV_RGB(118,185,0),2);
					// p.y += 1;	// move to new row

				}

				imshow("",rebuild);
                waitKey(1);

            }
		}
        if(recvpackandnum[0] == '#' && recvpackandnum[1] == '#'){ //Get time from last packet of frame sent to time received
            int p = 20; //Where the end bracket is                  //The uS number is right, with a delay being more noticable the more the clocks between the pis are off.
                                                                    //Can be as low as 200 mS if synced properly!

            char buf[p-2];
            memcpy(buf,recvpackandnum+3,p-3);
            buf[p-2] = '\0';

            //cout << buf << endl;

            rectime = strtoull(buf,NULL,0);

            //gettimeofday(&tv1, 0);

            nowtime = micros();

            cout << "Components: " << nowtime << " , " << rectime << endl;

            if(rectime > nowtime) remaining_time = rectime - nowtime;
            else remaining_time = nowtime - rectime;

            cout << "Received last packet of new frame in " << remaining_time << " uS" << endl;



        }

        //printf("R_V: %s\n", buffer);
    }

    close(sockfd);
    return 0;
}

vector<string> commands;

void* comthread(void* v){

    int sockfd;
    char buffer[MAXLINE];
    char *hello = "Hello from client com";
    struct sockaddr_in     servaddr;

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("com socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port1);
    servaddr.sin_addr.s_addr = inet_addr(sub_address); //INADDR_ANY;

    srand((unsigned) time(0));

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;

    //Let the controller be the one to time out on recieve, as we want the commands to be
    //executed as soon as possible on the sub.
    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
        printf("com setsockopt failed\n");

    int n;
    socklen_t len = sizeof(servaddr);

    cominitdone = true;

    string next_command = "";

    struct SUB_OPERATING_INFO  subInfoPacket = {0};
	int packetSize = sizeof(subInfoPacket);

    while(true){

    /*
        if(commands.empty() == false){
            next_command = commands[0];
        }
        */

        sendto(sockfd, next_command.c_str(), strlen(next_command.c_str()),
            MSG_CONFIRM, (const struct sockaddr *) &servaddr,
                sizeof(servaddr));

        //if(commands.empty() == false) commands.erase(commands.begin());

        /*
        sendto(sockfd, (const char *)hello, strlen(hello),
            MSG_CONFIRM, (const struct sockaddr *) &servaddr,
                sizeof(servaddr));
        //printf("S_C: Hello message sent.\n");
        */

        //cout << "Waiting on command socket..." << endl;
        /*
        n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                    0, (struct sockaddr *) &servaddr,
                    &len);
        */
        //buffer[n] = '\0';

        int bytesR = recvfrom(sockfd, &subInfoPacket, packetSize,
				MSG_DONTWAIT, (struct sockaddr *) &servaddr,
                &len);

		distTxt    = "Distance: ";
		depthTxt   = "Depth: ";
		tempTxt    = "Temperature: ";
		//bladderTxt = "Bladder: ";
		//waterTxt   = "Water: ";
		lightTxt = "Lights: ";
        pressTxt = "Pressure: ";

		if (bytesR > 0) {
			distTxt += to_string(subInfoPacket.sonicDist);
			if (toupper(subInfoPacket.sonDistUnits) == 'F')
				distTxt.append(" feet");
			else
				distTxt.append(" meters");
			//cout << distTxt << endl; 	// use in putText below


 			depthTxt += to_string(subInfoPacket.depthUnits);
			if (toupper(subInfoPacket.depthUnits) == 'F')
				depthTxt.append(" feet");

			else
				depthTxt.append(" meters");
			//cout << depthTxt << endl;


 			tempTxt += to_string(subInfoPacket.tempUnits);
			if (toupper(subInfoPacket.tempUnits) == 'F')
				tempTxt.append(" fahrenheit");
			else
				tempTxt.append(" celcius");
			//cout << tempTxt << endl;


			/*
			bladderTxt += to_string(subInfoPacket.capacity);
			if (toupper(subInfoPacket.capacity) == 'F')
				bladderTxt.append(" Full");
			else
				bladderTxt.append(" Empty");
			cout << bladderTxt << endl;


			waterTxt += to_string(subInfoPacket.water);
			if (toupper(subInfoPacket.water) == 'y')
				waterTxt.append(" detected");
			else
				waterTxt.append(" not detected");
			cout << waterTxt << endl;

			*/

            //cout << "Lights: " << subInfoPacket.lights << endl;
            lightTxt += to_string(subInfoPacket.lights);
            lightTxt += " %";

            //lightTxt.append("%");
            //cout << "ShowTime: " << subInfoPacket.showTime << endl;
            //cout << "ShowSensors: " << subInfoPacket.showSensors << endl;

            pressTxt += to_string(subInfoPacket.pressure);
            pressTxt += " millibar";

            if(subInfoPacket.showTime == '1'){
                showTime = true;
            }
            else{
                showTime = false;
            }

            if(subInfoPacket.showSensors == '1'){
                showSensors = true;
            }
            else{
                showSensors = false;
            }



        }

        //if(n < 1) cout << "com socket did not recieve." << endl;

        //printf("R_C: %s\n", buffer);
    }

    close(sockfd);
    return 0;
}

void setupSocket(int* sok, sockaddr_in* inaddr, int p, char* addr, char* error){

    if ((*sok = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror(error);
        exit(EXIT_FAILURE);
    }
    memset(inaddr, 0, sizeof(*inaddr));
    inaddr->sin_family = AF_INET;
    inaddr->sin_port = htons(p);
    inaddr->sin_addr.s_addr = inet_addr(addr);

}

int main(){

    pthread_t com_thread, vid_thread;
    pthread_create(&com_thread, NULL, comthread, NULL);
    pthread_create(&vid_thread, NULL, vidthread, NULL);

    string next_command;

    while(vidinitdone == false || cominitdone == false){ ; }

    cout << endl;

    char last_char;


    while(true){

        ;

        /*
        cout << "Next command:";
        cin >> next_command;

        last_char = next_command.c_str()[strlen(next_command.c_str())-1];

        if(last_char != '#') cout << "Invalid command." << endl;
        else commands.push_back(next_command);

        cout << "\nCurrent commands:" << endl;

        for(int i = 0; i < commands.size(); i++){

            cout << "\t" << i << ": " << commands[i] << endl;

        }
        */


    }

    return 0;
}
