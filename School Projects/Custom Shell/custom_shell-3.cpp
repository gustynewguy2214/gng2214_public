#include <iostream>
#include <limits>
#include <vector>
#include <string.h>
#include <unistd.h> //For Unix process operations
#include <sys/wait.h>
#include <algorithm>
#include <dirent.h>

using namespace std;

//Function declarations
int interpret_command(string);

void byebye();
int whereami();
int movetodir(vector<string> arguments);
int history(vector<string> arguments);
int start(vector<string> arguments);
int background(vector<string> arguments);
int exterminate(vector<string> arguments);
int repeat(vector<string> arguments);
int exterminateall();
int listpids();
int help(vector<string> arguments);

char* str2charr(string s);
bool strcontains(string src, string sub);
char* geterrno();
bool isNumber(string n);
int checkdir(string program);
int listfiles();
vector<string> split(string phrase, string delimiter);

//=====================================

bool bye = false;
string current_directory;
vector<string> cmd_history;
vector<pid_t> child_pids;
bool force_move = true;	//Forcefully change the working directory by moving to where the program supposedly is.

int main(int argc, char **argv) {

	int exit_code = 0;
	char cwd[100];
	string input;

	cout << "Welcome to mysh! Use 'help' to see a list of available commands." << endl;

	if (getcwd(cwd, sizeof(cwd))) {
		current_directory = cwd;

		while(!bye){
			cout << "\n# ";
			if (getline(cin, input)) {
				cmd_history.push_back(input);
				interpret_command(input);
			}
			else{
				cout << "Error attempting to interpret your command. Please try again." << endl;
				cin.clear();												//Clear cin in an attempt to recover.
				cin.ignore(numeric_limits<streamsize>::max(), '\n');
			}
		}
	} else {
		cout << "Error attempting to get the current working directory. Now exiting." << endl;
		exit_code = -1;
	}

	return exit_code;
}

int interpret_command(string input){

	int return_code = 0;

	if (!input.empty()) {

		vector<string> tokens = split(input," ");
		vector<string> arguments(tokens.begin() + 1, tokens.begin() + tokens.size());

		for(unsigned int i = 1; i < tokens.size(); i++){
			arguments[i-1] = tokens[i];
		}

		if(tokens[0] == "byebye"){
			byebye();
		}
		else if(tokens[0] == "whereami"){
			return_code = whereami();
		}
		else if(tokens[0] == "movetodir"){
			return_code = movetodir(arguments);
		}
		else if(tokens[0] == "history"){
			return_code = history(arguments);
		}
		else if(tokens[0] == "start" || tokens[0] == "run"){
			return_code = start(arguments);
		}
		else if(tokens[0] == "background"){
			return_code = background(arguments);
		}
		else if(tokens[0] == "exterminate"){
			return_code = exterminate(arguments);
		}
		else if(tokens[0] == "repeat"){
			return_code = repeat(tokens);
		}
		else if(tokens[0] == "exterminateall"){
			exterminateall();
		}
		else if(tokens[0] == "listpids"){
			return_code = listpids();
		}
		else if(tokens[0] == "listfiles"){
			return_code = listfiles();
		}
		else if(tokens[0] == "help"){
			return_code = help(arguments);
		}
		else{
			cout << tokens[0] << ": command not found." << endl;
			return_code = -1;
		}

	} else {
		cout << "";
	}

	return return_code;
}

//Commands:
int movetodir(vector<string> arguments){

	int return_code = 0;

	string directory = arguments[0];

	return_code = chdir(str2charr(directory));

	if(return_code == 0){
		current_directory = directory;
	}
	else{
		if(strcontains(geterrno(), "No such")){
			cout << "mysh: movetodir: " << directory << ": " << geterrno() << endl;
			return_code = -1;
		}
		else if(strcontains(geterrno(), "Not a dir")){
			cout << "mysh: movetodir: " << directory << ": " << geterrno() << endl;
			return_code = -1;
		}
		else{
			perror("ERROR: ");
		}
	}

	return return_code;
}

int whereami(){
	cout << "You are in: " << current_directory << endl;
	return 0;
}

int history(vector<string> arguments){

	int return_code = 0;

	if(arguments.size() == 0){
		for(unsigned int i = 0; i < cmd_history.size(); i++){
			cout << "  " << i << "  " << cmd_history[i] << endl;
		}
	}
	else{
		if(arguments[0] == "-c"){
			cmd_history.clear();
		}
		else{
			cout << "history: invalid option -- '" << arguments[0] << "'" << endl;
			return_code = -1;
		}
	}

	return return_code;
}

void byebye(){
	printf("Goodbye!\n");
	exit(0);
}

int start(vector<string> arguments){

	int return_code = 0;
	int status = 0;

	string target_dir = arguments[0].substr(0,arguments[0].find_last_of("/"));
	string target_prog = arguments[0].substr(arguments[0].find_last_of("/")+1);
	string tmp_wd = current_directory;

	if(target_dir.find("/") != target_dir.npos){

		if(checkdir(arguments[0]) != 0){
			vector<string> tmp;
			tmp.push_back(target_dir);

			if(force_move) movetodir(tmp);
		}
	}

	if(checkdir(target_prog) == 0 || force_move == false){

		pid_t pid = fork();

		if(pid < 0){
			perror("ERROR");
			return_code = -1;
		}
		else if(pid == 0){	//Child Process

			char **args = new char* [arguments.size() + 1];
			for (unsigned int i = 0;  i < arguments.size();  i++){
				args [i] = (char* ) arguments[i].c_str();
			}
			args[arguments.size()] = NULL;

			if(args[0][0] == '/'){	//If it is a fully qualified path, treat normally.
				execvp(args[0],args);
			}
			else{	//Otherwise assume that it's a program to run directly.

				string directory;
				if(arguments[0].find("/") != arguments[0].npos){
					directory = arguments[0].substr(0,arguments[0].find_last_of("/"));
					args[0] = (char* ) string("./" + directory).c_str();
				}
				execv(args[0],args);
			}
		}
		else{	//Parent Process

			if(getpgid(pid) >= 0){
				cout << "Attempted to start " << arguments[0] << endl;
			}

			if (waitpid(pid, &status, 0) > 0) {
				if (WIFEXITED(status) && !WEXITSTATUS(status)){
				  cout << "Program terminated succesfully." << endl;
				}
				else if (WIFEXITED(status) && (WEXITSTATUS(status) != 127)) {
					cout << "WARNING: Program terminated normally, but returned a non-zero status." << endl;
				}
				else{
					cout << "ERROR: Program didn't terminate normally." << endl;
				}
			}
		}
	}
	else if(checkdir(target_prog) != 0){
		cout << "mysh: start: No such file or directory" << endl;
		return_code = -1;
	}

	if(strcontains(geterrno(), "Permission")){
		cout << "mysh: start: " << arguments[0] << ": Permission denied\nTry placing the program on the desktop or investigating your file permissions." << endl;
	}
	else if(strcontains(geterrno(), "No such file")){
		cout << "mysh: " << arguments[0] << ": No such file or directory" << endl;
	}
	else{
		if(strcontains(geterrno(), "Success") == false){
			if(strcontains(geterrno(), "Invalid")){
				cout << "ERROR: Attempted to read from errno when no new error was created." << endl;
			}
			else{
				perror("ERROR");
			}
		}
	}

	return return_code;
}

//Exec:
// with L: comma separated arguments
// with V: Vector (an array of strings)
// With P: Include normal search path.

int background(vector<string> arguments){

	int return_code = 0;

	string target_dir = arguments[0].substr(0,arguments[0].find_last_of("/"));
	string target_prog = arguments[0].substr(arguments[0].find_last_of("/")+1);
	string tmp_wd = current_directory;

	if(target_dir.find("/") != target_dir.npos){

		if(checkdir(arguments[0]) != 0){
			vector<string> tmp;
			tmp.push_back(target_dir);

			if(force_move) movetodir(tmp);
		}
	}

	if(checkdir(target_prog) == 0 || force_move == false){
		pid_t pid = fork();

		if(pid > 0){
			if(getpgid(pid) >= 0){
				printf("New process PID: %u\n",pid);
				child_pids.push_back(pid);	//In order to not lose the pid.
				vector<string> tmp;
				tmp.push_back(tmp_wd);
				movetodir(tmp);				// Return to where the user had the wd.
			}
		}
		else if(pid == 0){

			char **args = new char* [arguments.size() + 1];
			for (unsigned int i = 0;  i < arguments.size();  i++){
				args [i] = (char* ) arguments[i].c_str();
			}
			args[arguments.size()] = NULL;

			if(args[0][0] == '/'){	//If it is a fully qualified path, treat normally.
				execvp(args[0],args);
			}
			else{	//Otherwise assume that it's a program to run directly.

				string directory;
				if(arguments[0].find("/") != arguments[0].npos){
					directory = arguments[0].substr(0,arguments[0].find_last_of("/"));
					args[0] = (char* ) string("./" + directory).c_str();
				}

				execv(args[0],args);
			}

			return_code = -1;	//If we got here, something went wrong.
		}
	}
	else{
		return_code = -1;

		if(strcontains(geterrno(), "No such")){
			cout << "mysh: background: Could not find " << arguments[0] << " in " << current_directory << endl;
		}
		else{
			cout << "mysh: background: Could not open " << arguments[0] << endl;
		}
	}

	return return_code;
}

int exterminate(vector<string> arguments){

	int sig = 9;
	int return_code = 0;

	if(arguments.size() == 1){
		if(isNumber(arguments[0]) == false){
			cout << "mysh: exterminate: The PID value must be an integer." << endl;
			return -1;
		}
		else{
			return_code = kill(stoi(arguments[0]),sig);
		}
	}
	else if(arguments.size() == 2){

		if(isNumber(arguments[1]) == false){
			cout << "mysh: exterminate: The sig value must be an integer." << endl;
			return -1;
		}
		else{
			sig = stoi(arguments[1]);
			return_code = kill(stoi(arguments[0]),sig);
		}
	}
	else{
		if(arguments.size() == 0){
			cout << "mysh: exterminate: please specify a PID and optional sig code" << endl;
		}
		else{
			cout << "mysh: exterminate: too many arguments" << endl;
		}
	}

	if(return_code == 0){

		if(sig == 0){	//The process wasn't killed since sig 0 was used.
			return -1;
		}

		int pid = stoi(arguments[0]);

		cout << "Succesfully terminated process " << pid << endl;

		for(unsigned int i = 0; i < child_pids.size(); i++){
			if(child_pids[i] == pid){
				int status;
				waitpid(pid, &status, WNOHANG);	//Zombie process hunter.
											//From Chris Turner's answer here:
											//https://stackoverflow.com/questions/42840950/waitpid-and-fork-execs-non-blocking-advantage-over-a-syscall
				child_pids.erase(remove(child_pids.begin(), child_pids.end(), pid), child_pids.end());
				break;
			}
		}
	}
	else{
		if(strcontains(geterrno(), "No such process")){
			cout << "mysh: exterminate: " << geterrno() << endl;
		}
		else{
			if(strcontains(geterrno(), "Success") == false){
				perror("ERROR");
			}
		}
	}

	return return_code;
}

//Extra credit functions

int repeat(vector<string> arguments){

	int return_code = 0;

	if(isNumber(arguments[1])){

		vector<string> tmp;
		string tmp_input;

		for(unsigned int i = 2; i < arguments.size(); i++){	//Get only the useful arguments.
			tmp.push_back(arguments[i]);
			tmp_input = tmp_input+ arguments[i] + " " ;
		}

		tmp_input = tmp_input.substr(0,tmp_input.find_last_of(" "));

		int size = stoi(arguments[1]); //stoi(arguments[0]);

		for(int i = 0; i < size; i++){
			return_code = interpret_command(tmp_input);
			//return_code = background(tmp);
			if(return_code == -1){
				break;
			}
		}
	}else{
		cout << arguments[1] << ": bad command line option " << "'" << arguments[0] << "'" << endl;
		return_code = -1;
	}

	return return_code;
}

int exterminateall(){

	int return_code = 0;

	cout << "Murdering " << child_pids.size() << " processes: " << endl;

	for(unsigned int i = 0; i < child_pids.size(); i++){
		if(getpgid(child_pids[i]) >= 0){
			int status;
			waitpid(child_pids[i], &status, WNOHANG);
			cout << "\t" << child_pids[i] << endl;
		}
	}

	for(unsigned int i = 0; i < child_pids.size(); i++){
		if(getpgid(child_pids[i]) >= 0){
			int status;
			waitpid(child_pids[i], &status, WNOHANG);
			kill(child_pids[i],9);
		}
	}
	child_pids.clear();

	return return_code;
}

int listpids(){

	int return_code = 0;

	if(child_pids.size() < 1){
		return_code = -1;
		cout << "mysh: listpids: No PIDs in the list." << endl;
		cout << "NOTE: Sometimes the list can be slow to refresh. If you suspect there should be PIDs here, retry the command." << endl;
	}
	else{
		cout << "Below is the list of all child PIDs that were opened by this shell session.\n";
		cout << "Beware that some may have been closed before listing." << endl;
		for(unsigned int i = 0; i < child_pids.size(); i++){
			if(getpgid(child_pids[i]) >= 0){
				cout << "  " << i << "  " << child_pids[i] << endl;
			}
		}
	}

	return return_code;
}

int help(vector<string> arguments){
	int return_code = 0;

	cout << "mysh, version 1.0.0-release (x86_64-pc-linux-gnu)\nThe following shell commands are defined internally." << endl;

	string arr[] = {
			"byebye",
			"whereami",
			"movetodir",
			"history",
			"start",
			"background",
			"exterminate",
			"repeat",
			"exterminateall",
			"listpids",
			"listfiles",
			"help",
					};

	string help[] = {
			"\t\t | byebye",
			"\t\t | whereami",
			"\t\t | movetodir [FULL DIRECTORY PATH]",
			"\t\t | history [-c]",
			"\t\t\t | start (or 'run') [PROGRAM / FULL PROGRAM PATH] [PARAMETERS]...",
			"\t\t | background [PROGRAM / FULL PROGRAM PATH] [PARAMETERS]...",
			"\t\t | exterminate [PID] [INT SIG_VAL]",
			"\t\t | repeat [INT] [COMMAND] [ARGUMENTS]...",
			"\t | exterminateall",
			"\t\t | listpids",
			"\t\t | listfiles",
			"\t\t\t | help [-h]",
					};

	if(arguments.size() == 0){
		for(unsigned int i = 0; i < sizeof(arr)/sizeof(string); i++){
			cout << "  " << arr[i] << endl;
		}
		cout << "\nFor usage notes, provide [-h] as an argument to the help command." << endl;
	}
	else{
		if(arguments[0] == "-h"){
			cout << "Usage:" << endl;
			for(unsigned int i = 0; i < sizeof(arr)/sizeof(string); i++){
				cout << "  " << arr[i] << help[i] << endl;
			}
		}
		else{
			cout << "help: invalid option -- '" << arguments[0] << "'" << endl;
			return_code = -1;
		}
	}



	return return_code;
}

//Custom Functions

char* geterrno(){
	char buffer[256];
	char* errorMsg = strerror_r( errno, buffer, 256 ); 	// Get a copy of the error message for printing.
	return errorMsg;
}

char* str2charr(string s){
	char* tmp_array = new char[s.length() + 1];
	strcpy(tmp_array, s.c_str());
	return tmp_array;
}

bool strcontains(string src, string sub){
	return string(src).find(sub) != string(src).npos;
}

bool isNumber(string n){

	bool result = true;

	try{
		stoi(n);
	}
	catch (exception &e) {
		result = false;
	}

	return result;
}

int listfiles(){

	int return_code = 0;

	force_move = false;

	vector<string> tmp;
	tmp.push_back("/bin/ls");
	start(tmp);

	force_move = true;

	return return_code;

}

int checkdir(string program){

	int return_code = 0;

	string tru_directory = program.substr(0,program.find_last_of("/"));
	const char* tru_dir = tru_directory.c_str();

	if(tru_directory.find("/") == tru_directory.npos){
		tru_directory = current_directory;
	}
	else{
		tru_directory = program.substr(0,program.find_last_of("/"));
		tru_dir = tru_directory.c_str();
	}

	if(tru_directory.find("/") != tru_directory.npos && tru_dir[0] != '/'){
		cout << "ERROR: Fully qualified paths must start with a '/'" << endl;
		return -1;
	}

	DIR* dirp;
	struct dirent* directory;
	bool found = false;

	dirp = opendir(tru_dir);
	if (dirp != NULL){
		while ((directory = readdir(dirp)) != NULL){
			if(string(directory->d_name) == program){
				found = true;
				break;
			}
		}
		closedir(dirp);

		if(!found){
			return_code = -1;
		}
	}
	else{
		return_code = -1;
	}

	return return_code;
}

//Functions modified from online code
//Modified code from William Cuervo at: https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
vector<string> split(string phrase, string delimiter){
    vector<string> list;
    size_t pos = 0;
    string token;
    while ((pos = phrase.find(delimiter)) != string::npos) {
        token = phrase.substr(0, pos);
        list.push_back(token);
        phrase.erase(0, pos + delimiter.length());
    }
    list.push_back(phrase);
    return list;
}
