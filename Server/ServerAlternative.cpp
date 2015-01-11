/*
#include "Server.h"


// listens for new clients and stores sockets for accepted connections
class ListenThread
{
public:
	int operator()(ServerSharedData* data)
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

		// notify main thread that listening socket has been set up successfully;
		// mutex necessary here?
		data -> listenSocketCreated = true;
		data -> listenSocketCreatedCondition.notify_one();

		// start listening for clients using the listenSocket
		listen(listenSocket, 1);
	
		SOCKET TempSock = SOCKET_ERROR;

		// does this require mutex, use condition here as well 
		while (data -> doListen)
		{
			// Signal to accept the connection on this IP address on this Port number.
			TempSock = accept(listenSocket, NULL, NULL);
			
			if(TempSock != SOCKET_ERROR)
			{
				{
					// make sure no other thread is currently accessing the list for new sockets
					lock_guard<std::mutex> lock(data -> newSocketsMutex_);
					// add the new socket in the container for new clients
					data -> newSockets_.push_back(TempSock);
				}
				TempSock = SOCKET_ERROR;
				cout << "\nNew client connected!";
			}
		}
		// Shutdown the socket.
		shutdown(listenSocket, SD_SEND);

		// Close our socket entirely.
		closesocket(listenSocket);

		// Cleanup Winsock - release any resources used by Winsock.
		WSACleanup();

		// listening thread terminated as expected
		return 1;
	}
};

// server stuff, main thread
Server::Server(void)
{
}

void Server::run(void)
{
	// start listening
	sharedData.doListen = true;

	// start a thread that will listen for new clients and pass a pointer to the shared server data
	//old: thread t1(ListenThread(), &sharedData);
	future<int> listenFuture(async(launch::async, ListenThread(), &sharedData));

	// wait for notification from the listening thread stating that the listening socket has been created;
	// todo: account for socket error!!! --> notification might never come
	//old: unique_lock<std::mutex> lock(sharedData.listenSocketCreatedMutex);
	//old: sharedData.listenSocketCreatedCondition.wait_for(lock, chrono::seconds(5), [this]{return sharedData.listenSocketCreated;});

	// there has to be a better way to do this (use sharedData.listenSocketCreated instead of socket Created)
	bool socketCreated = false;
	// the first part of the expression will evaluate to true when the listening thread has terminated, 
	// what (at this stage) can only happen due to an error while creating the listening socket
	while((listenFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) && (!socketCreated))
	{
		unique_lock<std::mutex> lock(sharedData.listenSocketCreatedMutex);
		// evaluates to true when the listening socket has been created
		socketCreated = sharedData.listenSocketCreatedCondition.wait_for(lock, chrono::milliseconds(1000), [this]{return sharedData.listenSocketCreated;});
	}

	if(socketCreated)
	{
		// iterate through all active clients;
		// start with the first client again after iterating through all of them
		// todo: how to stop this loop
		while(true){

			// wait for clients to connect to the server
			updateActiveClients();

			// only execute the following code if there are active clients
			if(activeSockets.size() != 0){
				for(list<SOCKET>::iterator it = activeSockets.begin(); it != activeSockets.end(); ++it)
				{
					cout << "\nSelecting next client for drawing";
					// send the draw command to the client
					ServerCommand command = StartDrawing;
					int sentBytes = send(*it, reinterpret_cast<const char*>(&command), sizeof(int), 0);

					// wait for client to send a return code that is not '-1' to indicate it has finished receiving and processing the data from 
					// the server
					ClientState state = Idle;
					while(state != Done)
					{
						int receivedBytes = recv(*it, reinterpret_cast<char*>(&state), sizeof(int), 0);
					}
					cout << "\nClient finished. Selecting next client...";
				
					// this will add new clients to the current circle (no waiting for restart)
					updateActiveClients();
				}
			}
		}
	}else
	{
		int errorCode = listenFuture.get();
		cout << "\nThe server failed to create a socket to listen for clients and will terminate.\nError code: " << errorCode << "\n";
	}
}

void Server::updateActiveClients()
{
	lock_guard<std::mutex> lock(sharedData.newSocketsMutex_);
	if(sharedData.newSockets_.size() != 0)
	{
		// use splice as it does not invalidate iterators, which allows to add new clients while iterating through the respective sockets
		activeSockets.splice(activeSockets.end(), sharedData.newSockets_);
	}
	// mutex will be automatically released when the function returns
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
