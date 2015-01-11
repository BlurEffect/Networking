/*
// Creates a Winsock 'client' application.
// This application will try to conect to then obtain data from a 'server' application.

#include "stdafx.h"

#include <iostream>
#include <string>
#include <winsock2.h>
#include <vector>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>

#pragma comment(lib,"ws2_32.lib") 	// Use this library whilst linking - contains the Winsock2 implementation.

using namespace std;



int getClientId()
{
	static int ID = 0;
	return ID++;
}

int _tmain(int argc, _TCHAR* argv[])
{
	string name = "Client ";
	name.append(to_string(getClientId()));
	SetConsoleTitle(name.c_str());


	// Initialise Winsock
	WSADATA WsaDat;
	if (WSAStartup(MAKEWORD(2,2), &WsaDat) != 0)
	{
		std::cout << "Winsock error - Winsock initialization failed\r\n";
		WSACleanup();
		return 0;
	}
	
	// Create our socket
	SOCKET Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Socket == INVALID_SOCKET)
	{
		std::cout << "Winsock error - Socket creation Failed!\r\n";
		WSACleanup();
		return 0;
	}

	// Resolve IP address for hostname.
	struct hostent *host;

	// Change this to point to server, or ip address...

	if ((host = gethostbyname("localhost")) == NULL)   // In this case 'localhost' is the local machine. Change this to a proper IP address if connecting to another machine on the network.
	{
		std::cout << "Failed to resolve hostname.\r\n";
		WSACleanup();
		return 0;
	}

	// Setup our socket address structure.
	SOCKADDR_IN SockAddr;
	SockAddr.sin_port		 = htons(8888);	// Port number
	SockAddr.sin_family		 = AF_INET;		// Use the Internet Protocol (IP)
	SockAddr.sin_addr.s_addr = *((unsigned long*)host -> h_addr);

	// Attempt to connect to server...
	if (connect(Socket, (SOCKADDR*)(&SockAddr), sizeof(SockAddr)) != 0)
	{
		std::cout << "Failed to establish connection with server\r\n";
		WSACleanup();
		return 0;
	}

	while(true){

	// receive information on the size of the vector that is to be received
	size_t dataLength = 0;
	int inDataLength = recv(Socket, reinterpret_cast<char*>(&dataLength), sizeof(size_t), 0);
	// allocate enough memory to hold the vector that will be received
	vector<int> v(dataLength);
	
	// receive data

	// keep track of how many bytes have already been received
	int rcvCount = 0;
	// the position of the vector where to start inserting the received data
	int insertPos = 0;
	// the total amount of bytes that is to be received
	int totalBytes = dataLength * sizeof(int);

	// make sure everything is received
	while(rcvCount < dataLength*sizeof(int))
	{
		rcvCount  += recv(Socket, reinterpret_cast<char*>(&v[insertPos]), totalBytes - rcvCount, 0);
		insertPos = rcvCount / sizeof(int);
		// print state of receiving operation
		cout << "\nReceived bytes: " << rcvCount;
		cout << "\nTotal amount of bytes to be received: " << totalBytes;
	}

	// print the received data to the console
	// cout <<"\nThe vector: ";
	// copy(v.begin(), v.end(), ostream_iterator<int>(cout, " "));
	
	// print the received data to a file
	ofstream to("ClientOutput.txt");
	int id = -1;
	for(vector<int>::iterator it = v.begin(); it != v.end(); ++it)
	{
		to << "\n" << setfill('0') << setw(4) << ++id << ": " << *it;
	}
	to.close();

	// calculate and print the sum of all elements of the vector to the console to allow quick comparison of sent and received data
	LONGLONG sum = accumulate(v.begin(), v.end(), 0);
	cout << "\nThe sum of all elements in the vector is: " << sum;
	
	cout << "\n";

	// notify the server that client is finished
	int code = 1;
	int bla = send(Socket, reinterpret_cast<const char*>(&code), sizeof(int), 0);

	} // end while

	// Shutdown our socket.
	shutdown(Socket, SD_SEND);

	// Close our socket entirely.
	closesocket(Socket);

	// Cleanup Winsock.
	WSACleanup();
	system("PAUSE");
	return 0;
}
*/