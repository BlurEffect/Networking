#ifndef SERVER_H
#define SERVER_H

#include <list>
#include <mutex>
#include <iostream>
#include <thread>
#include <condition_variable>

#include "ProcessClientsFunctor.h"
#include "ServerSharedData.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

class Server
{ 
public:
	Server(void);
	~Server(void);
	int start();
	void run();
private:
	Networker networker;
	thread workerThread; // the thread processing clients that have connected to the server
	//SOCKET listenSocket; // the socket used to listen for new connections
	ServerSharedData sharedData; // data structure used for communication with the worker thread
};

#endif
