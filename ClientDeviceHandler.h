#pragma once
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

class ClientDeviceHandler {
public:
	static void *LaunchListener(void *);
	struct arg_struct;

private:

	ClientDeviceHandler();
	~ClientDeviceHandler();
	static void *listener(void *);
	static void *connection_handler(void *);

};

