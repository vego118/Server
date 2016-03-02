#pragma once
#include <queue>
#include <string>

class InternalCommunicationProvider {
public:
	volatile int flag;
	static const int ClientWatchdog = 60;
	std::queue<std::string> ClientInputBuffer;
	std::queue<std::string> ClientOutputBuffer;
	std::queue<std::string> ClientErrorBuffer;
	std::queue<std::string> ClientStateBuffer;

	InternalCommunicationProvider();
	~InternalCommunicationProvider();
};

