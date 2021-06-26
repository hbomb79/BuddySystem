//////////////////////////////////////////////////////////////
// TCP SERVER GCC (towards IPV6 ready)
//
//
// References: https://msdn.microsoft.com/en-us/library/windows/desktop/ms738520(v=vs.85).aspx
//             http://long.ccaba.upc.edu/long/045Guidelines/eva/ipv6.html#daytimeServer6
//
// Adapted by Harry Felton, Massey ID 18032692 to support RSA-CBC
// message encryption over TCP sockets.
//////////////////////////////////////////////////////////////
//Ws2_32.lib
#define _WIN32_WINNT 0x501 //to recognise getaddrinfo()

//"For historical reasons, the Windows.h header defaults to including the Winsock.h header file for Windows Sockets 1.1. The declarations in the Winsock.h header file will conflict with the declarations in the Winsock2.h header file required by Windows Sockets 2.0. The WIN32_LEAN_AND_MEAN macro prevents the Winsock.h from being included by the Windows.h header"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

//159.334 - Networks
//single threaded server
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <ctime>
#include "..\rsa.hpp"
#include <gmp.h>


// These make up the private key for the server to use when initially communicating
// the RSA handshake. In practise these would be stored by a certificate authority.
#define RSA_CA_KEY_N "65650656922093225868188553737060901124501059642203171520450070656258308462871"
#define RSA_CA_KEY_D "20810027112921932254844576580156295832119266774638289209998669762252868969873"

#define WSVERS MAKEWORD(2, 2) /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
							  //The high-order byte specifies the minor version number;
							  //the low-order byte specifies the major version number.

#define USE_IPV6 true

#define DEFAULT_PORT "1234"
#define BUFFER_LENGTH 1024

WSADATA wsadata; //Create a WSADATA object called wsadata.

/**
 * This method performs the RSA handshake between this server and the connected
 * client. We pass the recv and send buffers by reference here just to limit
 * the creation of unnescesarry arrays.
 * 
 * Nonce is also passed by reference so that we can use this value outside
 * of this method (in main) to use when encrypting our strings.
 * 
 * The entire key (public and secret components) are set inside the &rsa_key.
 * 
 * Returns 1 on success, -1 on failure.
 */
template <size_t size>
int rsa_handshake(SOCKET &ns, char (&receive_buffer)[size], char (&send_buffer)[size], mpz_class &nonce, rsa::rsa_key &rsa_key)
{
	std::cout << "\n--------------------------------------------" << std::endl;
	std::cout << "<<<SERVER>>> Beginning RSA handshake" << std::endl;

	// Create key pair for this client
	rsa_key = rsa::generateKeyPair();

	// Encrypt public key components with our CA private key.
	mpz_class rsa_ca_n(RSA_CA_KEY_N);
	mpz_class rsa_ca_d(RSA_CA_KEY_D);

	mpz_class cipher_client_n = rsa::cipher_byte(rsa_key.n, rsa_ca_n, rsa_ca_d);
	mpz_class cipher_client_e = rsa::cipher_byte(rsa_key.e, rsa_ca_n, rsa_ca_d);

	// Pack n and e components for transmission
	std::string cipher_public_key;
	cipher_public_key.append(cipher_client_n.get_str()).append(" ");
	cipher_public_key.append(cipher_client_e.get_str());

	std::cout << "<<<SERVER>>> Transmitting encrypted RSA public-key to client" << std::endl;
	int bytes = send(ns, cipher_public_key.c_str(), cipher_public_key.size(), 0);
	if (bytes == SOCKET_ERROR)
		return -1;

	//********************************************************************
	// Wait for ACK 226 pubkey recv
	//********************************************************************
	bytes = recv(ns, receive_buffer, BUFFER_LENGTH, 0);
	if ((bytes == SOCKET_ERROR) || (bytes == 0) || strncmp(receive_buffer, "ACK 226 public key recvd", 24) != 0)
	{
		std::cout << "<<<SERVER>>> FATAL - Failed to recv pubkey ACK (226)" << std::endl;
		return -1;
	}

	//********************************************************************
	// Wait for & decipher NONCE
	//********************************************************************

	std::cout << "<<<SERVER>>> Received ACK 226 public key recvd" << std::endl;
	std::cout << "<<<SERVER>>> Waiting for NONCE from client..." << std::endl;
	bytes = recv(ns, receive_buffer, BUFFER_LENGTH, 0);
	if (bytes == SOCKET_ERROR || bytes == 0)
	{
		std::cout << "<<<SERVER>>> FATAL - Failed to recv NONCE" << std::endl;
		return -1;
	}

	std::string cipher_nonce_string(receive_buffer);
	mpz_class cipher_nonce(cipher_nonce_string);
	nonce = rsa::cipher_byte(cipher_nonce, rsa_key.n, rsa_key.d);

	std::cout << "<<<SERVER>>> Nonce received" << std::endl;

	//********************************************************************
	// ACK 220 nonce OK
	//********************************************************************
	std::cout << "<<<SERVER>>> Sending ACK 220 nonce OK " << std::endl;
	sprintf(send_buffer, "ACK 220 nonce OK");
	bytes = send(ns, send_buffer, strlen(send_buffer), 0);
	if (bytes == SOCKET_ERROR)
	{
		std::cout << "<<<SERVER>>> FATAL - Failed to ACK 220 NONCE" << std::endl;
		return -1;
	}

	std::cout << "\n--------------------------------------------" << std::endl;
	std::cout << "<<<SERVER>>> RSA Handshake complete" << std::endl;

	return 1;
}

/**
 * Handles the main communication between the client and this server. Requires references to
 * the socket to communicate over, the buffers to use for winsock send/recv, the rolling nonce
 * used for CBC ciphering, and the rsa_key used for RSA ciphering.
 */
template <size_t size>
void main_communication_loop(SOCKET &ns, char (&receive_buffer)[size], char (&send_buffer)[size], mpz_class &nonce, rsa::rsa_key &rsa_key) {
	int bytes;

	while (1)
	{
		// Reset contents of buffer
		memset(receive_buffer, 0, BUFFER_LENGTH);
		memset(send_buffer, 0, BUFFER_LENGTH);
		//********************************************************************
		//RECEIVE one command (delimited by \n)
		//********************************************************************

		std::cout << "<<<SERVER>>> Awaiting communication from client" << std::endl;

		int i = 0;
		std::string received_text;
		while (1)
		{
			if (i >= BUFFER_LENGTH - 1)
			{
				// Buffer overflow incoming!
				i = 0;
				received_text.append(receive_buffer);
				memset(receive_buffer, 0, BUFFER_LENGTH);
			}

			bytes = recv(ns, &receive_buffer[i], 1, 0);
			if (bytes == SOCKET_ERROR)
			{
				std::cout << "Socket error: " << WSAGetLastError() << " - BREAKING" << std::endl;
				break;
			}
			else if (bytes == 0)
			{
				std::cout << "<<<SERVER>>> Client appears to be closing their socket. Shutting down connection." << std::endl;
				break;
			}

			/*end on a LF, Note: LF is equal to one character*/
			if (receive_buffer[i] == '\n')
			{
				// We found a newline. End the listening and terminate the buffer with a \0.
				receive_buffer[i] = '\0';
				break;
			}

			if (receive_buffer[i] != '\r')
				i++;
		}

		// Catch errors and break to allow socket connection to close.
		if ((bytes == SOCKET_ERROR) || (bytes == 0))
			break;

		// If this was a misfire/empty data, don't process.
		if (i == 0)
			continue;

		// Finish constructing our text
		received_text.append(receive_buffer);

		//********************************************************************
		//PROCESS REQUEST
		//********************************************************************
		std::string text = rsa::decrypt_string(received_text, rsa_key.n, rsa_key.d, nonce);

		std::cout << "\n\n--------------------------------------------" << std::endl;
		std::cout << "<<<SERVER>>> Message from client received.\nCipher text:" << received_text << "\nDeciphered text:" << text << std::endl;
		std::cout << "<<<SERVER>>> Echoing plaintext message to client" << std::endl;

		//********************************************************************
		//SEND
		//********************************************************************
		text.insert(0, "Client said: ");
		char *to_send = new char[text.length() + 1];
		std::strcpy(to_send, text.c_str());

		bytes = send(ns, to_send, strlen(to_send), 0);
		if (bytes == SOCKET_ERROR)
			break;
	}
}

//*******************************************************************
//MAIN
//*******************************************************************
int main(int argc, char *argv[])
{
	//********************************************************************
	// INITIALIZATION of the SOCKET library
	//********************************************************************
	//struct sockaddr_in clientAddress;  //IPV4
	struct sockaddr_storage clientAddress; //IPV6

	char clientHost[NI_MAXHOST];
	char clientService[NI_MAXSERV];

	SOCKET s, ns;
	char send_buffer[BUFFER_LENGTH], receive_buffer[BUFFER_LENGTH];
	int addrlen;
	char portNum[NI_MAXSERV];

	std::cout << "Initialising RSA components" << std::endl;
	rsa::initialise();

	rsa::rsa_key rsa_key = rsa::generateKeyPair();
	std::cout << "n " << rsa_key.n << std::endl;
	std::cout << "e " << rsa_key.e << std::endl;
	std::cout << "d " << rsa_key.d << std::endl;
	std::cout << "p " << rsa_key.p << std::endl;
	std::cout << "q " << rsa_key.q << std::endl;

	//memset(&localaddr,0,sizeof(localaddr));

	//********************************************************************
	// WSSTARTUP
	/*	All processes (applications or DLLs) that call Winsock functions must 
		initialize the use of the Windows Sockets DLL before making other Winsock 
		functions calls. 
		This also makes certain that Winsock is supported on the system.
	*/
	//********************************************************************
	int err;

	err = WSAStartup(WSVERS, &wsadata);
	if (err != 0)
	{
		WSACleanup();
		/* Tell the user that we could not find a usable */
		/* Winsock DLL.                                  */
		printf("WSAStartup failed with error: %d\n", err);
		exit(1);
	}

	//********************************************************************
	/* Confirm that the WinSock DLL supports 2.2.        */
	/* Note that if the DLL supports versions greater    */
	/* than 2.2 in addition to 2.2, it will still return */
	/* 2.2 in wVersion since that is the version we      */
	/* requested.                                        */
	//********************************************************************

	if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wVersion) != 2)
	{
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		printf("Could not find a usable version of Winsock.dll\n");
		WSACleanup();
		exit(1);
	}
	else
	{
		printf("\n\n<<<TCP SERVER>>>\n");
		printf("\nThe Winsock 2.2 dll was initialised.\n");
	}

	//********************************************************************
	// Set the socket address structure.
	//********************************************************************
	struct addrinfo *result = NULL;
	struct addrinfo hints;
	int iResult;

	//********************************************************************
	// STEP#0 - Specify server address information and socket properties
	//********************************************************************

	memset(&hints, 0, sizeof(struct addrinfo));

	if (USE_IPV6)
	{
		hints.ai_family = AF_INET6;
	}
	else
	{
		hints.ai_family = AF_INET;
	}

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE; // For wildcard IP address
								 //setting the AI_PASSIVE flag indicates the caller intends to use
								 //the returned socket address structure in a call to the bind function.

	// Resolve the local address and port to be used by the server
	if (argc == 2)
	{
		iResult = getaddrinfo(NULL, argv[1], &hints, &result); //converts human-readable text strings representing hostnames or IP addresses
															   //into a dynamically allocated linked list of struct addrinfo structures
															   //IPV4 & IPV6-compliant
		sprintf(portNum, "%s", argv[1]);
		printf("\nargv[1] = %s\n", argv[1]);
	}
	else
	{
		iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result); //converts human-readable text strings representing hostnames or IP addresses
																	//into a dynamically allocated linked list of struct addrinfo structures
																	//IPV4 & IPV6-compliant
		sprintf(portNum, "%s", DEFAULT_PORT);
		printf("\nUsing DEFAULT_PORT = %s\n", portNum);
	}

	if (iResult != 0)
	{
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	//********************************************************************
	// STEP#1 - Create welcome SOCKET
	//********************************************************************

	s = INVALID_SOCKET; //socket for listening
	// Create a SOCKET for the server to listen for client connections

	s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	//check for errors in socket allocation
	if (s == INVALID_SOCKET)
	{
		printf("Error at socket(): %d\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		exit(1); //return 1;
	}
	//********************************************************************

	//********************************************************************
	//STEP#2 - BIND the welcome socket
	//********************************************************************

	// bind the TCP welcome socket to the local address of the machine and port number
	iResult = bind(s, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);

		closesocket(s);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result); //free the memory allocated by the getaddrinfo
						  //function for the server's address, as it is
						  //no longer needed
						  //********************************************************************

	//********************************************************************
	//STEP#3 - LISTEN on welcome socket for any incoming connection
	//********************************************************************
	if (listen(s, SOMAXCONN) == SOCKET_ERROR)
	{
		printf("Listen failed with error: %d\n", WSAGetLastError());
		closesocket(s);
		WSACleanup();
		exit(1);
	}
	else
	{
		printf("\n<<<SERVER>>> is listening at PORT: %s\n", portNum);
	}

	//*******************************************************************
	//INFINITE LOOP
	//********************************************************************
	while (1)
	{									 //main loop
		addrlen = sizeof(clientAddress); //IPv4 & IPv6-compliant
		ns = INVALID_SOCKET;

		//********************************************************************
		// STEP#4 - Accept a client connection.
		//	accept() blocks the iteration, and causes the program to wait.
		//	Once an incoming client is detected, it returns a new socket ns
		// exclusively for the client.
		// It also extracts the client's IP address and Port number and stores
		// it in a structure.
		//********************************************************************

		ns = accept(s, (struct sockaddr *)(&clientAddress), &addrlen); //IPV4 & IPV6-compliant

		if (ns == INVALID_SOCKET)
		{
			printf("accept failed: %d\n", WSAGetLastError());
			closesocket(s);
			WSACleanup();
			return 1;
		}
		else
		{
			printf("\nA <<<CLIENT>>> has been accepted.\n");

			//strcpy(clientHost,inet_ntoa(clientAddress.sin_addr)); //IPV4
			//sprintf(clientService,"%d",ntohs(clientAddress.sin_port)); //IPV4

			memset(clientHost, 0, sizeof(clientHost));
			memset(clientService, 0, sizeof(clientService));

			getnameinfo((struct sockaddr *)&clientAddress, addrlen,
						clientHost, sizeof(clientHost),
						clientService, sizeof(clientService),
						NI_NUMERICHOST);

			printf("\nConnected to <<<Client>>> with IP address:%s, at Port:%s\n", clientHost, clientService);
		}

		//********************************************************************
		// RSA HANDSHAKE
		//********************************************************************

		mpz_class nonce;
		rsa::rsa_key rsa_key;
		int result = rsa_handshake(ns, receive_buffer, send_buffer, nonce, rsa_key);
		if(result <= 0)
			break;

		//********************************************************************
		// Begin main communication
		//********************************************************************
		main_communication_loop(ns, receive_buffer, send_buffer, nonce, rsa_key);
		
		//********************************************************************
		//CLOSE SOCKET
		//********************************************************************
		int iResult = shutdown(ns, SD_SEND);
		if (iResult == SOCKET_ERROR)
		{
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(ns);
			WSACleanup();
			exit(1);
		}

		closesocket(ns);

		printf("\ndisconnected from <<<Client>>> with IP address:%s, Port:%s\n", clientHost, clientService);
		printf("=============================================");

	} 

	closesocket(s);
	WSACleanup(); /* call WSACleanup when done using the Winsock dll */

	return 0;
}
