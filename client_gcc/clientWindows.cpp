//159.334 - Networks
//////////////////////
//	CLIENT GCC
// 
// Adapted by Harry Felton, Massey ID 18032692 to support
// RSA-CBC based message encryption
/////////////////////

//Ws2_32.lib
#define _WIN32_WINNT 0x501 //to recognise getaddrinfo()

//#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <stdlib.h>
#include <cstdio>
#include <iostream>
#include <sstream>

#include <gmp.h>
#include <gmpxx.h>
#include "../rsa.hpp"

using namespace std;

#define DEFAULT_PORT "1234"

// These make up the PUBLIC key for the server to use when initially communicating
// the RSA handshake. In practise these would be stored by a certificate authority.
#define RSA_CA_KEY_N "65650656922093225868188553737060901124501059642203171520450070656258308462871"
#define RSA_CA_KEY_E "65537"
#define RSA_NONCE_BIT_LENGTH 32

#define WSVERS MAKEWORD(2, 2)

#define USE_IPV6 true
#define BUFFER_SIZE 1024

WSADATA wsadata;

/**
 * This method performs the RSA handshake between this client and the connected
 * server. We pass the recv and send buffers by reference here just to limit
 * the creation of unnescesarry arrays.
 * 
 * Nonce is also passed by reference so that we can use this value outside
 * of this method (in main) to use when encrypting our strings.
 * 
 * The rest of the public key elements are returned inside of a rsa::rsa_public_key struct
 */
template <size_t size>
rsa::rsa_public_key rsa_handshake(SOCKET &s, char (&receive_buffer)[size], char (&send_buffer)[size], mpz_class &nonce)
{
	rsa::rsa_public_key pubkey;
	mpz_class rsa_ca_n(RSA_CA_KEY_N);
	mpz_class rsa_ca_e(RSA_CA_KEY_E);

	std::cout << "\n--------------------------------------------" << std::endl;
	std::cout << "<<<CLIENT>>> Awaiting RSA handshake from server" << std::endl;

	int bytes = recv(s, receive_buffer, BUFFER_SIZE, 0);
	if ((bytes == SOCKET_ERROR) || (bytes == 0))
	{
		printf("recv failed\n");
		exit(1);
	}

	std::cout << "<<<CLIENT>>> RSA handshake packet received. Deciphering..." << std::endl;

	std::istringstream buff_stream(receive_buffer);
	mpz_class rsa_cipher_n;
	mpz_class rsa_cipher_e;

	buff_stream >> rsa_cipher_n;
	buff_stream >> rsa_cipher_e;

	pubkey.n = rsa::cipher_byte(rsa_cipher_n, rsa_ca_n, rsa_ca_e);
	pubkey.e = rsa::cipher_byte(rsa_cipher_e, rsa_ca_n, rsa_ca_e);

	//*******************************************************************
	// ACK pubkey recv
	//*******************************************************************
	std::cout << "<<<CLIENT>>> Sending ACK 226 public key recvd" << std::endl;

	sprintf(send_buffer, "ACK 226 public key recvd");
	bytes = send(s, send_buffer, strlen(send_buffer), 0);
	if (bytes == 0 || bytes == SOCKET_ERROR)
	{
		std::cout << "<<<CLIENT>>> FATAL - Failed to acknowledge public key receipt (ACK 226) - Socket error/0 bytes sent." << std::endl;
		WSACleanup();
		exit(1);
	}

	//********************************************************************
	// Generate and send NONCE
	//********************************************************************
	nonce = rsa::random_bits(RSA_NONCE_BIT_LENGTH);
	mpz_class cipher_nonce = rsa::cipher_byte(nonce, pubkey.n, pubkey.e);
	sprintf(send_buffer, cipher_nonce.get_str().c_str());

	std::cout << "<<<CLIENT>>> Sending encrypted NONCE to server" << std::endl;
	bytes = send(s, send_buffer, strlen(send_buffer), 0);
	if (bytes == 0 || bytes == SOCKET_ERROR)
	{
		std::cout << "<<<CLIENT>>> FATAL - Failed to send cipher NONCE packet - Socket error/0 bytes sent." << std::endl;
		WSACleanup();
		exit(1);
	}

	//*******************************************************************
	// Await nonce ACK 220 nonce OK
	//*******************************************************************
	bytes = recv(s, receive_buffer, BUFFER_SIZE, 0);
	if ((bytes == SOCKET_ERROR) || (bytes == 0) || strncmp(receive_buffer, "ACK 220 nonce OK", 16) != 0)
	{
		std::cout << "<<<CLIENT>>> FATAL - Failed recv nonce ACK (220 nonce OK)" << std::endl;
		WSACleanup();
		exit(1);
	}

	std::cout << "<<<CLIENT>>> Received ACK 220 nonce OK" << std::endl;
	std::cout << "\n--------------------------------------------" << std::endl;
	std::cout << "<<<CLIENT>>> RSA Handshake complete - You may now communicate securely with the server" << std::endl;

	return pubkey;
}

/**
 * This method handles the fetching of user input, encryption of the data, and sending/receiving the data from the server.
 * 
 * Requires references to the socket to communicate over (s), the buffers for sending and receiving, the nonce
 * for rolling CBC encryption, and the pubkey (n,e) to cipher the input text with.
 */
template<size_t size>
void client_communication_loop(SOCKET &s, char (&receive_buffer)[size], char (&send_buffer)[size], mpz_class &nonce, rsa::rsa_public_key pubkey) {
	int bytes;

	while (1)
	{
		std::cout << "\n--------------------------------------------" << std::endl;

		std::string user_input;
		std::cout << "> ";
		std::getline(cin, user_input);

		if (user_input == ".")
			break;
		else if(user_input.empty())
		{
			std::cout << "<<<CLIENT>>> WARN - Input empty - Please type some ASCII characters to encrypt and send to the server..." << std::endl;
			continue;
		}

		//*******************************************************************
		// Encrypt & send entered text
		//*******************************************************************
		std::cout << "<<<CLIENT>>> Encrypting message and sending to server" << std::endl;
		std::string cipher_text = rsa::encrypt_string(user_input.append("\n"), pubkey.n, pubkey.e, nonce);

		cipher_text.append("\n");
		char *to_send = new char[cipher_text.length() + 1];
		std::strcpy(to_send, cipher_text.c_str());

		bytes = send(s, to_send, strlen(to_send), 0);
		if (bytes == SOCKET_ERROR)
		{
			std::cout << "<<<CLIENT>>> FATAL - Send failed... Error: " << WSAGetLastError() << std::endl;
			break;
		}


		//*******************************************************************
		// Wait for server response
		//*******************************************************************
		std::cout << "<<<CLIENT>>> Sent - Awaiting reply" << std::endl;

		int i = 0;
		std::string received_text;
		while (1)
		{
			if (i >= BUFFER_SIZE - 1)
			{
				// Prevent buffer overflow from i exceeded buffer size.
				// This would manifest as adjacent memory being populated with our information.
				i = 0;
				received_text.append(receive_buffer);
				memset(receive_buffer, 0, BUFFER_SIZE);
			}

			bytes = recv(s, &receive_buffer[i], 1, 0);
			if (bytes == SOCKET_ERROR)
			{
				std::cout << "Socket error: " << WSAGetLastError() << " - Aborting." << std::endl;
				break;
			}
			else if (bytes == 0)
			{
				std::cout << "<<<CLIENT>>> Server appears to be closing their socket!" << std::endl;
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

		if (bytes == SOCKET_ERROR || bytes == 0)
			break;

		received_text.append(receive_buffer);

		std::cout << "<<<CLIENT>>> Server reply received" << std::endl;
		std::cout << "SERVER's reply: " << received_text << std::endl;
	}
}

/**
 * Main method initialises socket for communication with server, and creates our buffers
 * for receiving and sending content from/to the server.
 */
int main(int argc, char *argv[])
{
	//*******************************************************************
	// Initialization
	//*******************************************************************

	char portNum[12];

	SOCKET s;
	char send_buffer[BUFFER_SIZE], receive_buffer[BUFFER_SIZE];

	std::cout << "Initialising RSA components" << std::endl;
	rsa::initialise();

	//*******************************************************************
	//WSASTARTUP
	//*******************************************************************

	if (WSAStartup(WSVERS, &wsadata) != 0)
	{
		WSACleanup();
		printf("WSAStartup failed\n");
		exit(1);
	}
	else
	{
		printf("\n\n===================<< CLIENT >>==================\n");
		printf("\nThe Winsock 2.2 dll was initialised.\n");
	}

	//********************************************************************
	// Set the socket address structure.
	//********************************************************************
	struct addrinfo *result = NULL, hints;
	int iResult;

	memset(&hints, 0, sizeof(struct addrinfo));

	if (USE_IPV6)
		hints.ai_family = AF_INET6;
	else
		hints.ai_family = AF_INET;

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	//*******************************************************************
	// Dealing with user's arguments
	//*******************************************************************

	if (argc == 3)
	{
		sprintf(portNum, "%s", argv[2]);
		iResult = getaddrinfo(argv[1], portNum, &hints, &result);
	}
	else
	{
		printf("USAGE: ClientWindows IP-address [port]\n"); //missing IP address
		sprintf(portNum, "%s", DEFAULT_PORT);
		printf("Default portNum = %s\n", portNum);
		printf("Using default settings, IP:127.0.0.1, Port:1234\n");
		iResult = getaddrinfo("127.0.0.1", portNum, &hints, &result);
	}

	if (iResult != 0)
	{
		printf("getaddrinfo failed: %d\n", iResult);
		std::cout << "Error: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	//*******************************************************************
	//CREATE CLIENT'S SOCKET
	//*******************************************************************
	s = INVALID_SOCKET;
	s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (s == INVALID_SOCKET)
	{
		printf("socket failed\n");
		freeaddrinfo(result);
		WSACleanup();
		exit(1);
	}

	if (connect(s, result->ai_addr, result->ai_addrlen) != 0)
	{
		printf("connect failed\n");
		std::cout << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		WSACleanup();
		exit(1);
	}
	else
	{

		char ipver[80];

		// Get the pointer to the address itself, different fields in IPv4 and IPv6
		if (result->ai_family == AF_INET)
		{
			strcpy(ipver, "IPv4");
		}
		else if (result->ai_family == AF_INET6)
		{
			strcpy(ipver, "IPv6");
		}

		printf("\nConnected to SERVER with IP address: %s, %s at port: %s\n", argv[1], ipver, portNum);
	}

	//*******************************************************************
	// RSA HANDSHAKE
	//*******************************************************************
	mpz_class nonce;
	rsa::rsa_public_key pubkey = rsa_handshake(s, send_buffer, receive_buffer, nonce);

	//********************************************************************
	// Begin main communication
	//********************************************************************

	client_communication_loop(s, send_buffer, receive_buffer, nonce, pubkey);

	printf("\n--------------------------------------------\n");
	printf("CLIENT is shutting down...\n");
	//*******************************************************************
	//CLOSESOCKET
	//*******************************************************************
	closesocket(s);
	WSACleanup();
	return 0;
}