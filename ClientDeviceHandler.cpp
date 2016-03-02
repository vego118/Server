#include "ClientDeviceHandler.h"
#include "InternalCommunicationProvider.h"
#include <pthread.h>
#include <queue>
#include <string>
#include <string.h>
#include <iostream>
#include <sys/socket.h>
#include <signal.h>
#include <stdio.h>
#include <netdb.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <sstream>
#include <ctime>

namespace patch {
template<typename T> std::string to_string(const T& n) {
	std::ostringstream stm;
	stm << n;
	return stm.str();
}
}

ClientDeviceHandler::ClientDeviceHandler() {
}

std::string ClientDeviceHandler::getTime()
{
    //time_t _tm =time(NULL );
    //struct tm * curtime = localtime ( &_tm );
    //return asctime(curtime);
    time_t t;
    struct tm * ptr;
    char buf [20];
    time ( &t );
    ptr= localtime ( &t );
    strftime (buf,20,"%Y-%m-%d/%X",ptr);
    return buf;
}

struct ClientDeviceHandler::arg_struct {
	volatile int flag; //Error in connection flag 1-good, 0-broken
	int watchdog; //watchdog timer value populated by InternalCommunicationProvider
	std::queue<std::string> *inputBuffer;
	std::queue<std::string> *outputBuffer;
	std::queue<std::string> *errorBuffer;
	std::queue<std::string> *stateBuffer;

	int socket;
};

ClientDeviceHandler::~ClientDeviceHandler() {
}

void * ClientDeviceHandler::listener(void *sock) {
	struct arg_struct *args = (struct arg_struct *) sock;

	pthread_t handler_thread;
	int sockfd, portno;
	int *newsockfd;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

	args->stateBuffer->push(
			"Listener thread running - binding to address...\n");

	portno = args->socket;
	args->flag = 1;
	sockfd = 0;
	signal(SIGPIPE, SIG_IGN);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		args->errorBuffer->push(ClientDeviceHandler::getTime() + "\tERROR opening socket\n");
	bzero((char *) &serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		args->errorBuffer->push(ClientDeviceHandler::getTime() + "\tERROR on binding\n");
		close(sockfd);
	}
	std::string message = "Starting listening on port ";
	message += patch::to_string(portno);
	message += " ...\n";
	args->stateBuffer->push(message);

	listen(sockfd, 5);
	while (true) {

		newsockfd = new int();

		*newsockfd = -1;
		clilen = sizeof(cli_addr);
		*newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (*newsockfd > 0) {

			//Initialize arguments structure for ClientHandler
			struct arg_struct argsForClientHandler;
			argsForClientHandler.socket = *newsockfd;
			argsForClientHandler.inputBuffer = args->inputBuffer;
			argsForClientHandler.outputBuffer = args->outputBuffer;
			argsForClientHandler.stateBuffer = args->stateBuffer;
			argsForClientHandler.errorBuffer = args->errorBuffer;
			argsForClientHandler.watchdog = args->watchdog;
			argsForClientHandler.flag = 0;

			pthread_create(&handler_thread, NULL,
					&ClientDeviceHandler::connection_handler,
					(void*) &argsForClientHandler);
			message = "Server accepted connection and assigned socket ";
			message += patch::to_string(*newsockfd) += " \n";

			args->stateBuffer->push(message);
		}
	}
	return NULL;
}

void * ClientDeviceHandler::connection_handler(void *sock) {
	time_t WAtchdogTimer, TimerNow;
	//Reset Timers
	time(&WAtchdogTimer);
	time(&TimerNow);

	struct arg_struct *args = (struct arg_struct *) sock;

	int newsockfd = args->socket;
	args->stateBuffer->push(
			"Socket " + patch::to_string(newsockfd) + " is now handled");
	char buffer[256];
	args->flag = 1;
	while (args->flag == 1 && TimerNow - WAtchdogTimer < args->watchdog)
	{
		std::string xxx = patch::to_string(args->flag);
		if (newsockfd < 0) {
			args->errorBuffer->push(ClientDeviceHandler::getTime() + "\tERROR on accept\n");
			close(newsockfd);
			args->flag = 0;
		}
		bzero(buffer, 256);
		int n = read(newsockfd, buffer, 255);
		if (n < 0) {
			args->errorBuffer->push(ClientDeviceHandler::getTime() + "\tERROR reading from socket\n");
			args->flag = 0;
			close(newsockfd);
		}
		std::string rcv = buffer;
		if (rcv.compare("heartbeatASK") == 0)
			time(&WAtchdogTimer);
		if (rcv.compare("heartbeatASK") != 0 && rcv.compare("") != 0) {
			args->inputBuffer->push(rcv);
		}
		n = send(newsockfd, "heartbeatASKOK", 14, MSG_NOSIGNAL);
		if (n < 0) {
			close(newsockfd);
			args->errorBuffer->push(ClientDeviceHandler::getTime() + "\tERROR writing to socket\n");
			args->flag = 0;
		}
		if (!args->outputBuffer->empty()) {

			std::string tmp = args->outputBuffer->front();
			n = send(newsockfd, tmp.c_str(), strlen(tmp.c_str()), MSG_NOSIGNAL);
			if (n < 0) {
				close(newsockfd);
				args->errorBuffer->push(ClientDeviceHandler::getTime() + "\tERROR writing to socket\n");
				args->flag = 0;
			} else {
				args->outputBuffer->pop();
				std::string tmpState = "data sent: " + tmp;
				args->stateBuffer->push(tmpState);
			}
		}
		time(&TimerNow);
	}
	if (TimerNow - WAtchdogTimer > args->watchdog)
		args->stateBuffer->push("Connection time-out");

	return NULL;
}

void * ClientDeviceHandler::LaunchListener(void * communication) {
	InternalCommunicationProvider * comm =
			(InternalCommunicationProvider *) communication;
	pthread_t listener_thread;
	int *portno;
	portno = new int();
	*portno = 6000;
	comm->ClientStateBuffer.push("Initiating listener...\n");

	struct arg_struct args;
	args.socket = *portno;
	args.inputBuffer = &comm->ClientInputBuffer;
	args.outputBuffer = &comm->ClientOutputBuffer;
	args.errorBuffer = &comm->ClientErrorBuffer;
	args.stateBuffer = &comm->ClientStateBuffer;
	args.watchdog = comm->ClientWatchdog;
	args.flag = 0;

	int threadCreation = pthread_create(&listener_thread, NULL,
			&ClientDeviceHandler::listener, (void*) &args);
	std::string message = "Thread Creation result: ";
	message += patch::to_string(threadCreation);
	comm->ClientStateBuffer.push(message);

	while (true) {
		// Keep Client handler segment alive.
	}
	return NULL;
}
