#include "bcm2835.h"
#include "Bubbles.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <pigpio.h>
#include <raspicam/raspicam_cv.h>
#include <raspicam/raspicamtypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/socket.h>             // for socket stuff.
#include <sys/time.h>               // for gettimeofday()
#include <termios.h>
#include <unistd.h>
#include <vector>

#include <strings.h>
#include <sys/types.h>
#include <sys/time.h>
#include<netinet/in.h>
#include <ctype.h>
#include "udpPacket_1.h"

using namespace cv;
using namespace std;
using namespace raspicam;

// Pin mapping
#define LARGE_PUMP 5
#define SMALL_PUMP 6
#define LEFT_MOTOR_BRAKE 10
#define RIGHT_MOTOR_BRAKE 11
#define PAN 17
#define TILT 18
//#define BLADDER FULL 19
#define ECHO_PIN 20
#define VALVE 21
#define LEFT_MOTOR_ENABLE 22
#define RIGHT_MOTOR_ENABLE 23
#define LEFT_MOTOR_DIR 24
#define RIGHT_MOTOR_DIR 25
#define TRIG_PIN 26
#define HEADLIGHTS 27

#define SEC_TO_US(sec) ((sec)*1000000)
#define NS_TO_US(ns) ((ns)/1000)


int port1 = 6000;
int port2 = 6005;
char* sub_address = "192.168.1.10";

int comsokt;
struct sockaddr_in servaddr1, servaddr2;

int packet_count = 1;
int packet_size = 0;
int packets_needed = 0;
int kb = 1024;
int last_int_packets = 0;

int maxline = 1024;

vector<string> sent_cmd_pipe;

int sos = 343;


bool command_thread_started = false;
bool vid_thread_started = false;


/**
* Shortcut of cmd.find()
*/
bool bINa(string a, string b){
    return (a.find(b) != string::npos);
}

//From: stackoverflow.com/questions/5833094/get-a-timestamp-in-c-in-microseconds/67731965#67731965
uint64_t micros(){
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_TO_US((uint64_t)ts.tv_sec) + NS_TO_US((uint64_t)ts.tv_nsec);
    return us;
}

/**
    Effectively a panic button in case the motors go nuts. Works with any keyboard key and left bumper.
*/
void emergency_stop(){
    cout << "Emergency stop!" << endl;

    //Disable ballast
    gpioWrite(VALVE, 1);
    gpioWrite(SMALL_PUMP, 1);
    gpioWrite(LARGE_PUMP, 1);

    //Disable engines
    gpioWrite(LEFT_MOTOR_BRAKE,1);
    gpioWrite(RIGHT_MOTOR_BRAKE,1);

    usleep(250000);

    command_thread_started = false;
    vid_thread_started = false;

    gpioServo(LEFT_MOTOR_ENABLE,0);
    gpioServo(RIGHT_MOTOR_ENABLE,0);
    gpioWrite(HEADLIGHTS,0);

    gpioTerminate();
    cout << "5" << endl;
    usleep(500000);
    cout << "4" << endl;
    usleep(500000);
    cout << "3" << endl;
    usleep(500000);
    cout << "2" << endl;
    usleep(500000);
    cout << "1" << endl;
    usleep(500000);

    cout << "Press any key to exit." << endl;
    getchar();
    exit(-1);
}

void reset_pan_tilt(){
    gpioServo(PAN, 1500);
    gpioServo(TILT, 1500);
}

bool pan_tilt_locked = false;

void toggle_pan_tilt_lock(){
    if(pan_tilt_locked) pan_tilt_locked = false;
    else pan_tilt_locked = true;
}

bool headlight_on = false;

int brightness_max = 255;
int brightness_current = brightness_max;
int brightness_step = 5;

double true_brightness = 100.0;

void toggle_headlight(){

    if(headlight_on){
        headlight_on = false;
        gpioWrite(HEADLIGHTS, 0);
        true_brightness = 0.0;
    }
    else{
        headlight_on = true;
        gpioWrite(HEADLIGHTS, 1);
        true_brightness = 100.0;
    }

}

char show_time = '0';
bool time_on = false;

void toggle_time(){

    if(time_on){
        show_time = '0';
        time_on = false;
    }
    else{
        show_time = '1';
        time_on = true;
    }
}

char show_sensors = '0';
bool sensors_shown = false;

void toggle_sensors(){

    if(sensors_shown){
        show_sensors = '0';
        sensors_shown = false;
    }
    else{
        show_sensors = '1';
        sensors_shown = true;
    }

}





double remapInput(int in, int input_min, int input_max, int output_min, int output_max){
    double slope = ((output_max - output_min)/double(input_max - input_min)); //Output end - output start / input end - input start
    return double(output_min + double(slope * (in - input_min)));
}

//Unlikely to actually work.
void burst(){

    cout << "Burst time!" << endl;

    gpioServo(LEFT_MOTOR_ENABLE,1800);
    gpioServo(RIGHT_MOTOR_ENABLE,1800);

    usleep(250000);

    cout << "End burst." << endl;
}

int speed, dir;

int res = -999;

int last_hor_dir = 0;
int last_ver_dir = 0;

int min_speed = 600; //600; //600 stops the motor?, min 800 to start, maybe 775 with help. Like an airplane.
int max_speed = 1200; //1200; //1600 is safe top speed?
int min_start_speed = 850; //850 is true min start
int speed_step = 25;

int left_speed;
int right_speed;

int mot_pow = -999;

int pos1, pos2, pos3,x,y;

string tmp_motor_com, x_dir, y_dir;

int motor_brake_uS = 250000;

void handleMotors(char* buf){

    tmp_motor_com = string(buf);

    pos1 = tmp_motor_com.find(':');
    pos2 = tmp_motor_com.find_first_of(',');
    pos3 = tmp_motor_com.find_first_of('|');

    x_dir = tmp_motor_com.substr(pos1+1, pos2-pos1-1);
    y_dir = tmp_motor_com.substr(pos2+1,pos3-pos2-1);

    cout << "x: " << x_dir << ", y:" << y_dir << endl;

    x = atoi(x_dir.c_str());
    y = atoi(y_dir.c_str());

    if(x*x + y*y < 1200*1200){

        cout << "Motor should be stopped." << endl;

        last_hor_dir = 0;
        last_ver_dir = 0;

        gpioWrite(LEFT_MOTOR_BRAKE,1);
        gpioWrite(RIGHT_MOTOR_BRAKE,1);

        gpioWrite(LEFT_MOTOR_ENABLE,0);
        gpioWrite(RIGHT_MOTOR_ENABLE,0);

        usleep(motor_brake_uS);

        //Reset direction
        gpioWrite(LEFT_MOTOR_DIR,1);
        gpioWrite(RIGHT_MOTOR_DIR,1);

        gpioWrite(LEFT_MOTOR_BRAKE,0);
        gpioWrite(RIGHT_MOTOR_BRAKE,0);

    }
    else{
        if(y > 0 && y > abs(x) && ((x*x+y*y > 32767*32767) == false) || (x < 24000 && x > -24000) && y == 32767){ //The left equation is up on cartesian graphed circle.

            //cout << "Down" << endl;

            if(last_ver_dir == 1){
                cout << "Sudden reverse detected. Slow motors." << endl;

                //Sleep
                gpioWrite(LEFT_MOTOR_BRAKE,1);
                gpioWrite(RIGHT_MOTOR_BRAKE,1);

                //Slow for 0.25 seconds
                usleep(motor_brake_uS);

                //Change direction
                gpioWrite(LEFT_MOTOR_DIR,0);
                gpioWrite(RIGHT_MOTOR_DIR,0);

                //Release brakes.
                gpioWrite(LEFT_MOTOR_BRAKE,0);
                gpioWrite(RIGHT_MOTOR_BRAKE,0);

                burst();

            }
            last_ver_dir = -1;

            mot_pow = int(remapInput(y, 0, 32767, min_speed, max_speed));
            cout << "Reverse power: " << mot_pow << endl;

            //Set both motors reverse.
            gpioServo(LEFT_MOTOR_ENABLE,mot_pow);
            gpioServo(RIGHT_MOTOR_ENABLE,mot_pow);

        }
        else if(y < 0 && y < (-1*abs(x)) && ((x*x+y*y > 32767*32767) == false) || (x < 24000 && x > -24000) && y == -32767){    //Forward

            //cout << "Up" << endl;

            if(last_ver_dir == -1){
                cout << "Sudden move forward detected. Slow motors." << endl;

                //Sleep
                gpioWrite(LEFT_MOTOR_BRAKE,1);
                gpioWrite(RIGHT_MOTOR_BRAKE,1);

                //Slow for 0.25 seconds
                usleep(motor_brake_uS);

                //Change direction
                gpioWrite(LEFT_MOTOR_DIR,1);
                gpioWrite(RIGHT_MOTOR_DIR,1);

                //Release brakes.
                gpioWrite(LEFT_MOTOR_BRAKE,0);
                gpioWrite(RIGHT_MOTOR_BRAKE,0);

                burst();
            }
            last_ver_dir = 1;

            mot_pow = int(remapInput(y, 0, -32767, min_speed, max_speed));
            cout << "Forward power: " << mot_pow << endl;

            //Set both motors forward.
            gpioServo(LEFT_MOTOR_ENABLE,mot_pow);
            gpioServo(RIGHT_MOTOR_ENABLE,mot_pow);

        }
        else if(x > 0 && x > abs(y) && ((x*x+y*y > 32767*32767) == false) || (y < 24000 && y > -24000) && x == 32767){      //Right

            //cout << "Right" << endl;

            if(last_hor_dir == -1){
                cout << "Sudden change right detected. Slow motors." << endl;

                //Sleep
                gpioWrite(LEFT_MOTOR_BRAKE,1);
                gpioWrite(RIGHT_MOTOR_BRAKE,1);

                //Slow for 0.25 seconds
                usleep(motor_brake_uS);

                //Change direction
                gpioWrite(LEFT_MOTOR_DIR,1);
                gpioWrite(RIGHT_MOTOR_DIR,0);

                //Release brakes.
                gpioWrite(LEFT_MOTOR_BRAKE,0);
                gpioWrite(RIGHT_MOTOR_BRAKE,0);

                burst();
            }
            last_hor_dir = 1;

            mot_pow = int(remapInput(x, 0, 32767, min_speed, max_speed));
            cout << "Left Forward power: " << mot_pow << endl;
            cout << "Right Reverse power: " << mot_pow << endl;

            //Set both motors active.
            gpioServo(LEFT_MOTOR_ENABLE,mot_pow);
            gpioServo(RIGHT_MOTOR_ENABLE,mot_pow);

        }
        else if(x < 0 && x < (-1*abs(y)) && ((x*x+y*y > 32767*32767) == false) || (y < 24000 && y > -24000) && x == -32767){    //Left
            //cout << "Left" << endl;

            if(last_hor_dir == 1){
                cout << "Sudden change left detected. Slow motor." << endl;

                //Sleep
                gpioWrite(LEFT_MOTOR_BRAKE,1);
                gpioWrite(RIGHT_MOTOR_BRAKE,1);

                //Slow for 0.25 seconds
                usleep(motor_brake_uS);

                //Change direction
                gpioWrite(LEFT_MOTOR_DIR,0);
                gpioWrite(RIGHT_MOTOR_DIR,1);

                //Release brakes.
                gpioWrite(LEFT_MOTOR_BRAKE,0);
                gpioWrite(RIGHT_MOTOR_BRAKE,0);

                burst();

            }
            last_hor_dir = -1;

            mot_pow = int(remapInput(x, 0, -32767, min_speed, max_speed));
            cout << "Left Reverse power: " << mot_pow << endl;
            cout << "Right Forward power: " << mot_pow << endl;

            //Set both motors active.
            gpioServo(LEFT_MOTOR_ENABLE,mot_pow);
            gpioServo(RIGHT_MOTOR_ENABLE,mot_pow);

        }

        //Release brakes if they haven't already.
        gpioWrite(LEFT_MOTOR_BRAKE,0);
        gpioWrite(RIGHT_MOTOR_BRAKE,0);
    }

}
//*/

void prime_pumps(){
    gpioWrite(VALVE, 0); //open the valve (1 closed it?)
    gpioWrite(SMALL_PUMP, 0);
    gpioWrite(LARGE_PUMP, 0);
}

void stop_prime(){
    gpioWrite(VALVE, 1); //close the valve, was 0
    gpioWrite(SMALL_PUMP, 1);
    gpioWrite(LARGE_PUMP, 1);
}

bool a_button_pressed = false;
bool b_button_pressed = false;
bool x_button_pressed = false;
bool y_button_pressed = false;

bool lb_button_pressed = false;
bool rb_button_pressed = false;
bool lm_button_pressed = false;
bool rm_button_pressed = false;
bool mx_button_pressed = false;
bool rs_button_pressed = false;

void interpret(char* buf){

    //cout << "Processing: " << buf << endl;

    if(strstr(buf, "ls_pos")){

        handleMotors(buf);

    }
    else if(strstr(buf,"rs_pos")){
        string tmp = string(buf);

        int pos1 = tmp.find(':');
        int pos2 = tmp.find(',');

        string x_dir = tmp.substr(pos1+1, string::npos-pos1+pos2);
        string y_dir = tmp.substr(pos2+1);

        int x = atoi(x_dir.c_str());
        int y = atoi(y_dir.c_str());

        int x_deg = (int) remapInput(x, -32767, 32767, 2500, 500);
        int y_deg = (int) remapInput(y, -32767, 32767, 2500, 500);

        if(pan_tilt_locked == false){
            gpioServo(PAN, x_deg);
            gpioServo(TILT, y_deg);
        }

    }
    else if(strstr(buf,"LB_Button")){
        if(buf[strlen(buf)-1] == '1'){
            if(!lb_button_pressed) lb_button_pressed = true;
        }
        else{
            if(lb_button_pressed){
                emergency_stop();
                lb_button_pressed = false;
            }
        }
    }
    else if(strstr(buf,"RB_Button")){
        if(buf[strlen(buf)-1] == '1'){
                if(!rb_button_pressed) rb_button_pressed = true;
            }
            else{
                if(rb_button_pressed){
                    toggle_pan_tilt_lock();
                    rb_button_pressed = false;
                }
            }
    }
    else if(strstr(buf,"B_Button")){
        if(buf[strlen(buf)-1] == '1'){
            if(!b_button_pressed) b_button_pressed = true;
        }
        else{
            if(b_button_pressed){
                toggle_headlight();
                b_button_pressed = false;
            }
        }
    }
    else if(strstr(buf,"X_Button")){
        if(strstr(buf,"MX")){ //Xbox button

        }
        else{ // X button, toggles pan/tilt lock.
            if(buf[strlen(buf)-1] == '1'){
                if(!x_button_pressed){
                    x_button_pressed = true;
                    prime_pumps();
                }
            }
            else{
                if(x_button_pressed){
                    stop_prime();
                    x_button_pressed = false;
                }
            }
        }
    }
    else if(strstr(buf,"A_Button")){ //Small Pump Control
        if(buf[strlen(buf)-1] == '1'){
            if(!a_button_pressed){
                a_button_pressed = true;
            }
            gpioWrite(VALVE, 0); //open the valve
            gpioWrite(SMALL_PUMP, 0); //Small pump and some others are pullup, requiring writing 0 to work.
        }
        else{
            if(a_button_pressed){       //Stop small pump
                gpioWrite(VALVE, 1); //close
                gpioWrite(SMALL_PUMP, 1);
                a_button_pressed = false;
            }
        }
    }
    else if(strstr(buf,"Y_Button")){
        if(buf[strlen(buf)-1] == '1'){ //Start large pump
            if(!y_button_pressed) y_button_pressed = true;
            gpioWrite(LARGE_PUMP, 0);
            gpioWrite(VALVE, 1); //while 1, close valve.
        }
        else{
            if(y_button_pressed){ //Stop large pump
                gpioWrite(LARGE_PUMP, 1);
                gpioWrite(VALVE, 1);
                y_button_pressed = false;
            }
        }
    }
    else if(strstr(buf, "RS_Button")){
        if(buf[strlen(buf)-1] == '1'){
            if(!rs_button_pressed) rs_button_pressed = true;
        }
        else{
            if(rs_button_pressed){

                reset_pan_tilt();

                rs_button_pressed = false;
            }
        }
    }
    else if(strstr(buf, "Dpad_action")){

        if(headlight_on){
            cout << "Headlight is on." << endl;
            if(buf[strlen(buf)-1] == '0'){      //Up - max brightness
                gpioPWM(HEADLIGHTS, 255); //255 is default max if not changed.
                //cout << "\tBrightness now 100%!" << endl;
                //true_brightness = 100.0;
            }
            else if(buf[strlen(buf)-1] == '1'){ //Down - off.
                gpioPWM(HEADLIGHTS, 0);
                // << "\tBrightness now 0%!" << endl;
                //true_brightness = 0.0;
            }
            else if(buf[strlen(buf)-1] == '2'){ //Left - decrease brightness
                brightness_current -= brightness_step;
                if(brightness_current < 0) brightness_current = 0;
                gpioPWM(HEADLIGHTS, brightness_current);
                //cout << "\tBrightness now " << (brightness_current / double(brightness_max)) * 100.0 << " %!" << endl;
                //true_brightness = (brightness_current / double(brightness_max)) * 100.0;
            }
            else{                               //Right - increase brightness
                brightness_current += brightness_step;
                if(brightness_current > brightness_max) brightness_current = brightness_max;
                gpioPWM(HEADLIGHTS, brightness_current);
                //cout << "\tBrightness now " << (brightness_current / double(brightness_max)) * 100.0 << " %!" << endl;
                //true_brightness = (brightness_current / double(brightness_max)) * 100.0;
            }
        }
    }
    else if(strstr(buf,"LM_")){
        //cout << "Show time!" << endl;
        if(buf[strlen(buf)-1] == '1'){
            if(!lm_button_pressed) lm_button_pressed = true;
        }
        else{
            if(lm_button_pressed){

                toggle_time();

                lm_button_pressed = false;
            }
        }
    }
    else if(strstr(buf,"RM_")){
        cout << "Show sensors!" << endl;
        if(buf[strlen(buf)-1] == '1'){
            if(!rm_button_pressed) rm_button_pressed = true;
        }
        else{
            if(rm_button_pressed){

                toggle_sensors();

                rm_button_pressed = false;
            }
        }
    }
    else if (strlen(buf) > 0){
        cout << "recbuf: " << buf << endl;
    }


}

struct timeval last_packet_tv;
uint64_t accum;

void* vidthread(void* v){

    cout << "vidthread started" << endl;
    vid_thread_started = true;

    int sockfd;
    char buffer[maxline];
    char *hello = "Hello from server vidthread";
    struct sockaddr_in servaddr, cliaddr;

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("vid socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family    = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = inet_addr(sub_address); //INADDR_ANY;
    servaddr.sin_port = htons(port2);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10;

    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
        printf("vid setsockopt failed\n");

    // Bind the socket with the server address
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,sizeof(servaddr)) < 0 ){
       perror("vid socket bind failed");

        if(strstr(strerror(errno),"Cannot assign")){
            cout << "Check cable is connected, will keep retrying until then." << endl;
            while(bind(sockfd, (const struct sockaddr *)&servaddr,sizeof(servaddr)) < 0){ ; }
        }
        else{
            exit(-1);
        }
    }

    int n;

    socklen_t len = sizeof(cliaddr);  //len is value/resuslt

    RaspiCam_Cv cap;

    cout << "Configuring camera..." << endl;
    //set Camera parameters
    cap.set( CAP_PROP_FORMAT, CV_8UC3 ); // define to be color

    int rs = 480, cs = 800;
    int kbps = 25000;

    cap.set(CAP_PROP_FRAME_HEIGHT, rs);
    cap.set(CAP_PROP_FRAME_WIDTH, cs);

    int rows= 480, cols = 800, sz = 3;
    int leng = rows*cols*sz;

    string sub_side_name = "/home/pi/Desktop/ntsc3.jpeg";
    Mat image = imread(sub_side_name);

    // Check for failure
    if (image.empty()){
        cout << "Could not open or find the test image. Assuming hard coded test values (480*800*3)." << endl;
    }

    leng = rows * cols * sz;

    while(packet_count < 65){

        packet_size = kb * packet_count;

        if(leng%(packet_size) == 0) last_int_packets = leng/(packet_size);
        if(last_int_packets < 65) break;
        packet_count++;

    }

    packets_needed = (leng/(kb*last_int_packets));
    packet_size = kb*last_int_packets;

    //Print diagnostics
    cout << "For the image [" << sub_side_name << "] (" << rows << "*" << cols << "*" << sz << ")" << endl;
    cout << "[" << packets_needed << "] packets of size: [" << packet_size << "] bytes (" << last_int_packets << " KB) are needed." << endl;

    Mat img_transport;
    Mat flip_dest;

    //$$XX$$
    unsigned char* packandnum = (unsigned char*) calloc(6+packet_size,sizeof(unsigned char));
    packandnum[0] = '$';
    packandnum[1] = '$';
    packandnum[4] = '$';
    packandnum[5] = '$';

    int frame = 0;
    int bytes;

    if(!cap.open()) perror("Failed to open camera");

    //Consider: https://github.com/chenxiaoqino/udp-image-streaming/blob/master/Client.cpp (Compression)

    unsigned char* imgdata;

    timeval tv11;

    while(vid_thread_started){

        cap.grab();
        cap.retrieve(img_transport);
        flip(img_transport, flip_dest, -1);  //Flipping in place is known to segfau

        n = recvfrom(sockfd, (char *)buffer, maxline,
                    MSG_WAITALL, ( struct sockaddr *) &cliaddr,
                    &len);

        imgdata = flip_dest.data;

        for(int i = 0; i < packets_needed; i++){

            packandnum[2] = ((i/10)%10) + '0';
            if(i < 10) packandnum[2] = '0';
            packandnum[3] =  (i%10) + '0';

            memcpy(packandnum+6,imgdata+(i*packet_size),packet_size);

            sendto(sockfd, (const char *)packandnum, packet_size+6,
            MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
                len);

        }

        //gettimeofday(&tv11,0);

        accum = micros(); //(tv11.tv_sec * 1000000) + tv11.tv_usec;
        //

        string stamp = "##[" + to_string(accum) + "]";

        sendto(sockfd, (const char *)stamp.c_str(), strlen(stamp.c_str()),
        MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
            len);

        //cout << "\tTimestamp of packet: " << stamp << endl;


    }
}

struct timeval start, stop;
long secs_used,micros_used;
double dst;

string command, sent;

double densf = 997.0474;
double denss = 1023.6;
double g = 9.80665;

double raw_depth;
double fresh_depth;
double salt_depth;
double pressure_reading;
double temper;

void* comthread(void* v){

    cout << "comthread started" << endl;
    command_thread_started = true;

    int sockfd;
    char buffer[maxline];
    char *hello = "Hello from server comthread";
    struct sockaddr_in servaddr, cliaddr;

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("com socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family    = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = inet_addr(sub_address); //INADDR_ANY;
    servaddr.sin_port = htons(port1);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10;

    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
        printf("com setsockopt failed\n");


    // Bind the socket with the server address
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,sizeof(servaddr)) < 0 ){
        perror("com socket bind failed");

        if(strstr(strerror(errno),"Cannot assign")){
            cout << "Check cable is connected, and will keep retrying until then." << endl;
            while(bind(sockfd, (const struct sockaddr *)&servaddr,sizeof(servaddr)) < 0){ ; }
        }
        else{
            exit(-1);
        }
    }

    int n;

    socklen_t len = sizeof(cliaddr);  //len is value/resuslt

    long i = 0;
    int j = 0;

    /**
    AVERY SECTION
    */

    struct SUB_OPERATING_INFO  subInfoPacket;
    int packetSize = sizeof(subInfoPacket);

    while(command_thread_started){

        //cout << "Waiting " << i++ << endl;
        n = recvfrom(sockfd, (char *)buffer, maxline,
                    0, ( struct sockaddr *) &cliaddr,
                    &len);
        //buffer[n] = '\0';
        //if(strlen(buffer) != 0)
        //printf("com thread recieved: %s\n", buffer);

        command = string(buffer);

        //cout << "ComThread recieved command: " << command << endl;

        if(command != ""){
            interpret((char*) command.c_str());                                                     //Real interpret
            command = "";
            memset(buffer, '\0', sizeof(buffer));
        }

        /**
        AVERY SECTION
        */

        subInfoPacket.sonicDist    = dst;
		subInfoPacket.depth        = fresh_depth; //depth();
		subInfoPacket.temp         = temperature();
//		subInfoPacket.capacity     = 'e';
//		subInfoPacket.water        = 'n';
		subInfoPacket.sonDistUnits = 'm';
		subInfoPacket.depthUnits   = 'm';
		subInfoPacket.tempUnits    = 'c';
		subInfoPacket.lights = (double(brightness_current) / double(brightness_max)) * 100.0;;
		subInfoPacket.showTime = show_time;
		subInfoPacket.showSensors = show_sensors;
		subInfoPacket.pressure = pressure(1.0);

		sendto(sockfd, &subInfoPacket, packetSize, 0, (struct sockaddr*)&cliaddr, len);



        //signal 11 is a segment violation, such as not init or null

        /*
        string units = "M";

        //Send the sonar distance
        if(units == "M")
            sent = string("[Rear_Distance:") + to_string(dst) + string(" m]");
        else{
            dst = dst * 3.28;
            sent = string("[Rear_Distance:") + to_string(dst) + string("ft]");
        }

        sendto(sockfd, sent.c_str(), strlen(sent.c_str()),
            0, (const struct sockaddr *) &cliaddr,
                len);
                */

        //cout << "Sent (" << sent.c_str() << ")"<< endl;

        //printf("Depth %f m, Temperature %f C, Pressure %f mbar\n",depth(),temperature(), pressure());
		//cout << pressure() / (densf*g) << " fresh, " << pressure()/(denss*g) << " saltwater" << endl;

		/*
		sent = "[Raw_Depth:" + to_string(raw_depth) + "m|Fresh_Depth:" + to_string(fresh_depth) + "m|Salt_Depth:" + to_string(salt_depth) +
		"m|Temperature:" + to_string(temper) + "C|Pressure:" + to_string(pressure_reading) +"mbar]";

        sendto(sockfd, sent.c_str(), strlen(sent.c_str()),
            0, (const struct sockaddr *) &cliaddr,
                len);
                */

        //cout << "Sent (" << sent.c_str() << ")"<< endl;

        //sent = "";
    }

}

int leftMotorDir = 1;
int rightMotorDir = 1;

int main(){

    cout << "Bubbles Under The Sea - Version 1.0 final" << endl;

    //cout << std::flush;
    //isReady();

    pthread_t com_thread, com_thread2, vid_thread;

    cout << "Starting pigpio..." << endl;

    int tries = 0;
    while(gpioInitialise() < 0){
        usleep(150000);
        tries++;
        if(tries > 100) system("sudo reboot"); //Currently only option is to reboot.
    }

    cout << "Configuring pins..." << endl;
    gpioSetMode(VALVE,PI_OUTPUT);
    gpioWrite(VALVE,1); //CLOSE THE VALVE IMMEDIATELY!!!

    gpioSetMode(HEADLIGHTS, PI_OUTPUT);
    gpioSetPWMfrequency(HEADLIGHTS, 50);

    gpioSetPWMfrequency(PAN, 50);
    gpioSetPWMfrequency(TILT, 50);

    gpioSetMode(SMALL_PUMP,PI_OUTPUT);
    gpioSetMode(LARGE_PUMP,PI_OUTPUT);
    gpioWrite(SMALL_PUMP, 1);
    gpioWrite(LARGE_PUMP, 1);

    gpioSetMode(ECHO_PIN,PI_INPUT);
    gpioSetMode(TRIG_PIN,PI_OUTPUT);

    /////////////////////////////////////////////////////////////////////////

    gpioSetMode(RIGHT_MOTOR_BRAKE,PI_OUTPUT);
    gpioWrite(RIGHT_MOTOR_BRAKE,0);

    gpioSetMode(RIGHT_MOTOR_DIR, PI_OUTPUT);
    gpioWrite(RIGHT_MOTOR_DIR,0);

    gpioSetMode(RIGHT_MOTOR_ENABLE,PI_OUTPUT);
    gpioSetPWMfrequency(RIGHT_MOTOR_ENABLE, 50);

    gpioSetMode(LEFT_MOTOR_BRAKE,PI_OUTPUT);
    gpioWrite(LEFT_MOTOR_BRAKE,0);

    gpioSetMode(LEFT_MOTOR_DIR, PI_OUTPUT);
    gpioWrite(LEFT_MOTOR_DIR,0);

    gpioSetMode(LEFT_MOTOR_ENABLE,PI_OUTPUT);
    gpioSetPWMfrequency(LEFT_MOTOR_ENABLE, 50);

    gpioSetPWMrange(LEFT_MOTOR_ENABLE,1500);
    gpioSetPWMrange(RIGHT_MOTOR_ENABLE,1500);

    /////////////////////////////////////////////////////////////////////////

    cout << "Starting I2C..." << endl;

    bcm2835_init();

    if(!depth_init()){
		cout<<"MS5837 Init Failed"<<endl;
	}
	else cout<<"MS5837 Init OK"<<endl;

    cout << "Initializing emergency shutdown parameters..." << endl;
    struct termios oldSettings, newSettings;

    tcgetattr(fileno(stdin),&oldSettings);
    newSettings = oldSettings;
    newSettings.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(fileno(stdin), TCSANOW, &newSettings);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10;

    fd_set sett;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10;

    cout << "Giving time to read..." << endl;
    usleep(4000000);

    cout << "Launching threads..." << endl;
    pthread_create(&com_thread, NULL, comthread, NULL);
    pthread_create(&vid_thread, NULL, vidthread, NULL);

    int tries2 = 0;

    int res2;
    while(true) {

        FD_ZERO(&sett);
        FD_SET(fileno(stdin), &sett);

        res2 = select( fileno(stdin) + 1, &sett, NULL, NULL, &tv);

        if(res2 > 0){
                emergency_stop();
        }

        //Sonar Read
        gpioWrite(TRIG_PIN,1);
        usleep(10); // Sleep 10 microseconds
        gpioWrite(TRIG_PIN,0);
        while(gpioRead(ECHO_PIN) == 0){ ; }
        gettimeofday(&start, NULL);
        while(gpioRead(ECHO_PIN) == 1){ ; }
        gettimeofday(&stop, NULL);
        secs_used=(stop.tv_sec - start.tv_sec); //avoid overflow by subtracting first
        micros_used= ((secs_used*1000000) + stop.tv_usec) - (start.tv_usec);
        dst = ((micros_used/1000000.0) * sos) / 2;

        //Depth Sensor Read

        depth_read();
		raw_depth = depth();
		temper = temperature();
		pressure_reading = pressure();
		fresh_depth = (pressure()/(densf*g));
		salt_depth = (pressure()/(denss*g));
		bcm2835_delay(1000);

        usleep(500000); //Sleep for 5 mS required by sonar.


    }

    return 0;

}


//Unused test functions

/*
void valveTest(char* buf){
    //Valvve has power. closes.

    if(strstr(buf,"A_Button")){ //Small Pump Control
        if(buf[strlen(buf)-1] == '1'){
            if(!a_button_pressed){
                a_button_pressed = true;
            }
            cout << "Valve should be closed" << endl;
            gpioWrite(VALVE, 1); //open the valve               //Valve 0 closes (light should be off), Vavle 1 opens

        }
        else{
            if(a_button_pressed){       //Stop small pump
                cout << "Valve should be open." << endl;
                gpioWrite(VALVE, 0);
                a_button_pressed = false;
            }
        }
    }
}


void interpret2(char* buf){
    if(strstr(buf, "ls_pos")){

        handleMotors2(buf);

    }

    else if(strstr(buf,"LB_Button")){
        if(buf[strlen(buf)-1] == '1'){
            if(!lb_button_pressed) lb_button_pressed = true;
        }
        else{
            if(lb_button_pressed){
                emergency_stop();
                lb_button_pressed = false;
            }
        }
    }
}

void forwardMotorTest(char* buf){

    tmp_motor_com = string(buf);

    pos1 = tmp_motor_com.find(':');
    pos2 = tmp_motor_com.find_first_of(',');
    pos3 = tmp_motor_com.find_first_of('|');

    x_dir = tmp_motor_com.substr(pos1+1, pos2-pos1-1); //cause of sig 11?
    y_dir = tmp_motor_com.substr(pos2+1,pos3-pos2-1);

    //cout << tmp << endl;
    cout << "x: " << x_dir << ", y:" << y_dir << endl;

    x = atoi(x_dir.c_str());
    y = atoi(y_dir.c_str());

    if(x*x + y*y < 1200*1200){

        cout << "Motor should be stopped." << endl;

        last_hor_dir = 0;
        last_ver_dir = 0;

        gpioWrite(LEFT_MOTOR_BRAKE,1);
        gpioWrite(RIGHT_MOTOR_BRAKE,1);

        gpioWrite(LEFT_MOTOR_ENABLE,0);
        gpioWrite(RIGHT_MOTOR_ENABLE,0);

        //usleep(motor_brake_uS);

        //Reset direction
        gpioWrite(LEFT_MOTOR_DIR,1);
        gpioWrite(RIGHT_MOTOR_DIR,1);

        gpioWrite(LEFT_MOTOR_BRAKE,0);
        gpioWrite(RIGHT_MOTOR_BRAKE,0);

    }
    else{

        if(y < 0 && y < (-1*abs(x)) && ((x*x+y*y > 32767*32767) == false) || (x < 24000 && x > -24000) && y == -32767){    //Forward

            //cout << "Up" << endl;

            if(last_ver_dir == -1){
                cout << "Sudden move forward detected. Slow motors." << endl;

                //Sleep
                gpioWrite(LEFT_MOTOR_BRAKE,1);
                gpioWrite(RIGHT_MOTOR_BRAKE,1);

                //Slow for 0.25 seconds
                //usleep(motor_brake_uS);

                //Change direction
                gpioWrite(LEFT_MOTOR_DIR,1);
                gpioWrite(RIGHT_MOTOR_DIR,1);

                //Release brakes.
                gpioWrite(LEFT_MOTOR_BRAKE,0);
                gpioWrite(RIGHT_MOTOR_BRAKE,0);
            }
            last_ver_dir = 1;

            mot_pow = int(remapInput(y, 0, -32767, min_speed, max_speed));
            cout << "Forward power: " << mot_pow << endl;

            //Set both motors forward.
            //gpioServo(LEFT_MOTOR_ENABLE,mot_pow);
            //gpioServo(RIGHT_MOTOR_ENABLE,mot_pow);

            gpioPWM(LEFT_MOTOR_ENABLE, mot_pow);
            gpioPWM(RIGHT_MOTOR_ENABLE, mot_pow);

        }

    }

}
*/
