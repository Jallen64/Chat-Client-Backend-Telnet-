
const char * usage=
"IRCServer:                                                   \n"
"                                                               \n"
"Simple server program used to communicate multiple users       \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   IRCServer <port>                                          \n"
"                                                               \n"
"Where 1024 < port < 65536.                                     \n"
"                                                               \n"
"In another window type:                                        \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where talk-server      \n"
"is running. <port> is the port number you used when you run    \n"
"daytime-server.                                                \n"
"                                                               \n";

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <map>

#include "IRCServer.h"

using namespace std;

int QueueLength = 5;

//For notepad needs
fstream reader;
ofstream writer;

struct ROOM{

	string name;
	vector <string> guestVec;
	vector <string> messages;

};

vector<string> userVec;
vector<string> passVec;
vector<struct ROOM> rooms;

map<string, struct ROOM> roomMap;

//For reading in args
char * user;
char * password;
char * args;

int
IRCServer::open_server_socket(int port) {

	// Set the IP address and port for this server
	struct sockaddr_in serverIPAddress; 
	memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
	serverIPAddress.sin_family = AF_INET;
	serverIPAddress.sin_addr.s_addr = INADDR_ANY;
	serverIPAddress.sin_port = htons((u_short) port);

	// Allocate a socket
	int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
	if ( masterSocket < 0) {
		perror("socket");
		exit( -1 );
	}

	// Set socket options to reuse port. Otherwise we will
	// have to wait about 2 minutes before reusing the sae port number
	int optval = 1; 
	int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
			(char *) &optval, sizeof( int ) );

	// Bind the socket to the IP address and port
	int error = bind( masterSocket,
			(struct sockaddr *)&serverIPAddress,
			sizeof(serverIPAddress) );
	if ( error ) {
		perror("bind");
		exit( -1 );
	}

	// Put socket in listening mode and set the 
	// size of the queue of unprocessed connections
	error = listen( masterSocket, QueueLength);
	if ( error ) {
		perror("listen");
		exit( -1 );
	}

	return masterSocket;
}

	void
IRCServer::runServer(int port)
{
	int masterSocket = open_server_socket(port);

	initialize();

	while ( 1 ) {

		// Accept incoming connections
		struct sockaddr_in clientIPAddress;
		int alen = sizeof( clientIPAddress );
		int slaveSocket = accept( masterSocket,
				(struct sockaddr *)&clientIPAddress,
				(socklen_t*)&alen);

		if ( slaveSocket < 0 ) {
			perror( "accept" );
			exit( -1 );
		}

		// Process request.
		processRequest( slaveSocket );		
	}
}

	int
main( int argc, char ** argv )
{
	// Print usage if not enough arguments
	if ( argc < 2 ) {
		fprintf( stderr, "%s", usage );
		exit( -1 );
	}

	// Get the port from the arguments
	int port = atoi( argv[1] );

	IRCServer ircServer;

	// It will never return
	ircServer.runServer(port);

}

//
// Commands:
//   Commands are started y the client.
//
//   Request: ADD-USER <USER> <PASSWD>\r\n
//   Answer: OK\r\n or DENIED\r\n
//
//   REQUEST: GET-ALL-USERS <USER> <PASSWD>\r\n
//   Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//
//   REQUEST: CREATE-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LIST-ROOMS <USER> <PASSWD>\r\n
//   Answer: room1\r\n
//           room2\r\n
//           ...
//           \r\n
//
//   Request: ENTER-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LEAVE-ROOM <USER> <PASSWD>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: SEND-MESSAGE <USER> <PASSWD> <MESSAGE> <ROOM>\n
//   Answer: OK\n or DENIED\n
//
//   Request: GET-MESSAGES <USER> <PASSWD> <LAST-MESSAGE-NUM> <ROOM>\r\n
//   Answer: MSGNUM1 USER1 MESSAGE1\r\n
//           MSGNUM2 USER2 MESSAGE2\r\n
//           MSGNUM3 USER2 MESSAGE2\r\n
//           ...\r\n
//           \r\n
//
//    REQUEST: GET-USERS-IN-ROOM <USER> <PASSWD> <ROOM>\r\n
//    Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//

	void
IRCServer::processRequest( int fd )
{
	// Buffer used to store the comand received from the client
	const int MaxCommandLine = 1024;
	char commandLine[ MaxCommandLine + 1 ];
	int commandLineLength = 0;
	int n;

	// Currently character read
	unsigned char prevChar = 0;
	unsigned char newChar = 0;

	//
	// The client should send COMMAND-LINE\n
	// Read the name of the client character by character until a
	// \n is found.
	//

	// Read character by character until a \n is found or the command string is full.
	while ( commandLineLength < MaxCommandLine &&
			read( fd, &newChar, 1) > 0 ) {

		if (newChar == '\n' && prevChar == '\r') {
			break;
		}

		commandLine[ commandLineLength ] = newChar;
		commandLineLength++;

		prevChar = newChar;
	}

	// Add null character at the end of the string
	// Eliminate last \r
	commandLineLength--;
	commandLine[ commandLineLength ] = 0;

	printf("RECEIVED: %s\n", commandLine);

	/*
	   printf("The commandLine has the following format:\n");
	   printf("COMMAND <user> <password> <arguments>. See below.\n");
	   printf("You need to separate the commandLine into those components\n");
	   printf("For now, command, user, and password are hardwired.\n");
	 */


	char* command = strtok(commandLine, " ");

	//Tokenizing input into usable data
	user = strtok(NULL, " ");
	password = strtok(NULL, " ");
	args = strtok(NULL, "");

	printf("command=%s\n", command);
	printf("user=%s\n", user);
	printf( "password=%s\n", password );
	printf("args=%s\n", args);

	if (!strcmp(command, "ADD-USER")) {
		addUser(fd, user, password, args);
	}
	else if (!strcmp(command, "CREATE-ROOM")) {
		createRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "ENTER-ROOM")) {
		enterRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "LEAVE-ROOM")) {
		leaveRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "SEND-MESSAGE")) {
		sendMessage(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-MESSAGES")) {
		getMessages(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-USERS-IN-ROOM")) {
		getUsersInRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-ALL-USERS")) {
		getAllUsers(fd, user, password, args);
	}
	else {
		const char * msg =  "UNKNOWN COMMAND\r\n";
		write(fd, msg, strlen(msg));
	}

	// Send OK answer
	//const char * msg =  "OK\n";
	//write(fd, msg, strlen(msg));

	close(fd);	
}

	void
IRCServer::initialize()
{
	/*
	// Open password file
	reader.open("password.txt");

	if(reader.is_open()){

	string input;
	int parity=0;

	// Initialize users in room
	while(getline(reader,input)){


	if(parity%2 == 0){
	userVec.push_back(input);			
	}
	else{
	passVec.push_back(input);
	}
	parity++; 
	}

	}
	reader.close();
	 */
	// Initalize message list
	//vector<string> messages;

}

bool
IRCServer::checkPassword(int fd, const char * user, const char * password) {

	int i;
	for (i=0; i < userVec.size();i++){

		if( (userVec[i].compare(user) ==0 ) && (passVec[i].compare(password) ==0)){

			return true;

		}
	}

	return false;
}

void
IRCServer::addUser(int fd, const char * user, const char * password, const char * args)
{
	//First checks to see if user has been added yet
	int i;
	for(i=0; i<userVec.size();i++){

		if(userVec[i].compare(user) ==0){

			const char * msg =  "DENIED\r\n";
			write(fd, msg, strlen(msg));
			return;
		}
	}

	//Adds user(be sure to convert to string first)
	string userS(user);
	string passS(password);

	userVec.push_back(userS);
	passVec.push_back(passS);

	const char * msg =  "OK\r\n";
	write(fd, msg, strlen(msg));

}

void
IRCServer::createRoom(int fd, const char * user, const char * password, const char * args)
{

	struct ROOM r;
	string argsS(args);	

	r.name = argsS;
	roomMap[args] = r;

	const char * msg =  "OK\r\n";
	write(fd, msg, strlen(msg));
}


void
IRCServer::enterRoom(int fd, const char * user, const char * password, const char * args)
{

	string argsS(args);
	string userS(user);

	map<string, struct ROOM>::iterator it; //define iterator "it" suited for this type of map

	it=roomMap.find(argsS);
	it->second.guestVec.push_back(userS);// "->second" is needed to access attributes from the value at key

	const char * msg =  "OK\r\n";
	write(fd, msg, strlen(msg));

}

void
IRCServer::leaveRoom(int fd, const char * user, const char * password, const char * args)
{

	string argsS(args);
	string userS(user);

	//First have to check that user is in room in the first place
	map<string, struct ROOM>::iterator it;
	it=roomMap.find(argsS);	

	if( !(find(it->second.guestVec.begin(), it->second.guestVec.end(), userS) != it->second.guestVec.end()) ) {

		const char * msg =  "ERROR (No user in room)\r\n";
		write(fd, msg, strlen(msg));
		return;
	}
	
	//Has user leave room
	map<string, struct ROOM>::iterator it2;

	it2=roomMap.find(argsS);
	it2->second.guestVec.erase(remove( it2->second.guestVec.begin(),  it2->second.guestVec.end(), userS),  it2->second.guestVec.end());

	const char * msg =  "OK\r\n";
	write(fd, msg, strlen(msg));

}

void
IRCServer::sendMessage(int fd, const char * user, const char * password, const char * args)
{

	string argsS(args);
	string userS(user);
	
	int pos = argsS.find_first_of(' ');//Splits args into "message" and "roomName"
	
	string message= argsS.substr(pos+1);
	string roomName = argsS.substr(0, pos);
	string messageFinal = userS + " " +message;

	map<string, struct ROOM>::iterator it;

	it=roomMap.find(roomName);
	it->second.messages.push_back(messageFinal);

	const char * msg =  "OK\r\n";
	write(fd, msg, strlen(msg));

}

void
IRCServer::getMessages(int fd, const char * user, const char * password, const char * args)
{	

	string argsS(args);
	
	//Splits into roomName and number
	int pos = argsS.find_first_of(' ');
	string roomName = argsS.substr(pos+1);
	string number = argsS.substr(0, pos);
	int finalNumber = atoi(number.c_str());	

	map<string, struct ROOM>::iterator it;
	it=roomMap.find(roomName);

	int i;
	for( i =finalNumber+1; i< it->second.messages.size() ; i++){

		//bascially toString function since running on old version... 
		ostringstream convert;
		convert << i;
		string num = convert.str();

		string finalMessage= num + " " + it->second.messages[i] + "\r\n";

		const char *msg = finalMessage.c_str();
		write(fd, msg, strlen(msg));

	}

	const char *msg = "\r\n\0"  ;
	write(fd, msg, strlen(msg));

}

void
IRCServer::getUsersInRoom(int fd, const char * user, const char * password, const char * args)
{
	string argsS(args);
	string str2(user);

	map<string, struct ROOM>::iterator it;

	it=roomMap.find(argsS);
	int i;
	for( i =0; i < it->second.guestVec.size() ; i++){

		sort( it->second.guestVec.begin(),it->second.guestVec.end() ); 
		
		string s= it->second.guestVec[i] + "\r\n";

		const char *msg = s.c_str();
		write(fd, msg, strlen(msg));

	}

	const char *msg = "\r\n\0"  ;
	write(fd, msg, strlen(msg));

}

void
IRCServer::getAllUsers(int fd, const char * user, const char * password,const  char * args)
{
	if( checkPassword(fd, user, password,args) == false){	
		const char * msg =  "ERROR (Wrong Password)\r\n";
		write(fd, msg, strlen(msg));
		return;		
	} 

	sort( userVec.begin(), userVec.end() );

	int i;
	for(i=0; i<userVec.size();i++){
		string s = userVec[i] + "\r\n" ;

		const char *msg = s.c_str();	
		write(fd, msg, strlen(msg));

	}
	const char *msg = "\r\n\0"  ;
	write(fd, msg, strlen(msg));

}

