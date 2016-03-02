#include "ClientDeviceHandler.h"
#include "InternalCommunicationProvider.h"
#include <queue>
#include <pthread.h>

int main(int argc, char *argv[]) {
	InternalCommunicationProvider comm;

	pthread_t clientHandlerThread;
	pthread_create(&clientHandlerThread, NULL,
			&ClientDeviceHandler::LaunchListener, (void*) &comm);

	while (true) {
		if (!comm.ClientStateBuffer.empty())
		{
			printf("STATE: %s\n", comm.ClientStateBuffer.front().c_str());
			comm.ClientStateBuffer.pop();
			fflush(stdout);
		}
		if (!comm.ClientInputBuffer.empty())
		{
			printf("Received Command: %s\n", comm.ClientInputBuffer.front().c_str());
			comm.ClientOutputBuffer.push("dupa");
			comm.ClientInputBuffer.pop();
			fflush(stdout);
		}
		if (!comm.ClientErrorBuffer.empty())
		{
			printf("Received Command: %s\n", comm.ClientErrorBuffer.front().c_str());
			comm.ClientErrorBuffer.pop();
			fflush(stdout);
		}
	}
	return 0;
}