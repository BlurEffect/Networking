/*
#include "Server.h"


// communicate with clients
class WorkerThread
{
public:
	void operator()(ServerSharedData* sharedData)
	{
		// create an instance of the Networker class to use its functions for communication with the clients
		Networker networker;
		
		// iterate through all active clients;
		// start with the first client again after iterating through all of them
		// todo: how to stop this loop
		while(sharedData -> doRun)
		{
			{
				lock_guard<std::mutex> lock(sharedData -> newSocketsMutex_);
				if(sharedData -> newSockets_.size() != 0)
				{
					// use splice as it does not invalidate iterators, which allows to add new clients while iterating through the respective sockets
					sockets.splice(sockets.end(), sharedData -> newSockets_);
				}
			}

			// only execute the following code if there are active clients
			if(sockets.size() != 0){
				for(list<SOCKET>::iterator it = sockets.begin(); it != sockets.end(); ++it)
				{
					cout << "\nSelecting next client for drawing";
					// send the draw command to the client
					ServerCommand command = StartDrawing;

					networker.send<ServerCommand>(*it, &command, sizeof(int), 0);
					// old call: int sentBytes = send(*it, reinterpret_cast<const char*>(&command), sizeof(int), 0);

					// wait for client to send a return code that is not '-1' to indicate it has finished receiving and processing the data from 
					// the server
					ClientState state = Idle;
					while(state != Done)
					{
						networker.receive<ClientState>(*it, &state, sizeof(int), 0);
						// old call: int receivedBytes = recv(*it, reinterpret_cast<char*>(&state), sizeof(int), 0);
					}
					cout << "\nClient finished. Selecting next client...";
				
					// this will add new clients to the current circle (no waiting for restart)
					{
						lock_guard<std::mutex> lock(sharedData -> newSocketsMutex_);
						if(sharedData -> newSockets_.size() != 0)
						{
							// use splice as it does not invalidate iterators, which allows to add new clients while iterating through the respective sockets
							sockets.splice(sockets.end(), sharedData -> newSockets_);
						}
					}
				}
			}
		}

		for(list<SOCKET>::iterator it = sockets.begin(); it != sockets.end(); ++it)
		{
			// notify client of server termination
			ServerCommand command = Disconnect;
			networker.send<ServerCommand>(*it, &command, sizeof(int), 0);

			// shutdown and close the socket
			shutdown(*it, SD_BOTH);
			closesocket(*it);
		}
	}
private:
	list<SOCKET> sockets;
};

// server stuff, main thread, port as param?
Server::Server(void)
{
}

int Server::run(void)
{	
	WSADATA WsaDat;

	// Initialise Windows Sockets
	if (WSAStartup(MAKEWORD(2,2), &WsaDat)!=0)
	{
		std::cout << "WSA Initialization failed!\r\n";
		WSACleanup();
		return 0;
	}

	// Create a unbound socket.
	SOCKET listenSocket = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
	
	if (listenSocket == INVALID_SOCKET)
	{
		std::cout << "Socket creation failed.\r\n";
		WSACleanup();
		return 0;
	}

	// Now, try and bind the socket to any incoming IP address on Port 8888.
	SOCKADDR_IN serverInf;

	serverInf.sin_family		= AF_INET;
	serverInf.sin_addr.s_addr	= INADDR_ANY;
	serverInf.sin_port			= htons(8888);

	if (::bind(listenSocket, (SOCKADDR*)(&serverInf), sizeof(serverInf)) == SOCKET_ERROR)
	{
		std::cout << "Unable to bind socket!\r\n";
		WSACleanup();
		return 0;
	}
		
	cout << "\nWaiting for incoming connections...";

	// start a thread that will process the clients

	sharedData.doRun = true;
	//old: thread t1(WorkerThread(), &sharedData);
	future<void> workerFuture(async(launch::async, WorkerThread(), &sharedData));

	// start listening for clients using the listenSocket
	listen(listenSocket, 1);
	
	SOCKET tempSock = SOCKET_ERROR;

	// does this require mutex, use condition here as well 
	while (sharedData.doRun)
	{
		// Signal to accept the connection on this IP address on this Port number.
		tempSock = accept(listenSocket, NULL, NULL);
			
		if(tempSock != SOCKET_ERROR)
		{
			{
				// make sure no other thread is currently accessing the list for new sockets
				lock_guard<std::mutex> lock(sharedData.newSocketsMutex_);
				// add the new socket in the container for new clients
				sharedData.newSockets_.push_back(tempSock);
			}
			tempSock = SOCKET_ERROR;
			cout << "\nNew client connected!";
		}
	}

	// Shutdown the socket.
	shutdown(listenSocket, SD_BOTH);
	// Close our socket entirely.
	closesocket(listenSocket);

	// wait for the worker thread to finish
	workerFuture.get();

	// Cleanup Winsock - release any resources used by Winsock.
	// "In a multithreaded environment, WSACleanup terminates Windows Sockets operations for all threads."
	WSACleanup();

	cout << "Server terminated.";
	return 1;
}

void Server::shutDown()
{
	// stop while loop in run function
	// stop listening thread (doListen = false)
	// send notification to all clients, disconnect
}

Server::~Server(void)
{
}

*/