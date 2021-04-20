//=======================================================================================================================
//ACTIVE FTP SERVER Start-up Code for Assignment 1 (WinSock 2)

//This code gives parts of the answers away but this implementation is only IPv4-compliant. 
//Remember that the assignment requires a full IPv6-compliant FTP server that can communicate with a built-in FTP client either in Windows 10 or Ubuntu Linux.
//You must change parts of this program to make it IPv6-compliant (replace all data structures and functions that work only with IPv4).

//Hint: The sample TCP server codes show the appropriate data structures and functions that work in both IPv4 and IPv6. 

//=======================================================================================================================

#include <winsock2.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#define WSVERS MAKEWORD(2,0)
WSADATA wsadata;

//********************************************************************
//MAIN
//********************************************************************
int main(int argc, char *argv[]) {
//********************************************************************
// INITIALIZATION
//********************************************************************
	     int err = WSAStartup(WSVERS, &wsadata);

		 if (err != 0) {
		      WSACleanup();
			  // Tell the user that we could not find a usable WinsockDLL
		      printf("WSAStartup failed with error: %d\n", err);
		      exit(1);
		 }
		 struct sockaddr_in localaddr,remoteaddr;  //ipv4 only
		 struct sockaddr_in local_data_addr_act;   //ipv4 only
		 SOCKET s,ns;
		 SOCKET ns_data, s_data_act;
		 char send_buffer[200],receive_buffer[200];
		
         ns_data=INVALID_SOCKET;

		 int active=0;
		 int n,bytes,addrlen;
		 
		 printf("\n===============================\n");
		 printf("     159.334 FTP Server");
		 printf("\n===============================\n");
	
		 
		 memset(&localaddr,0,sizeof(localaddr));//clean up the structure
		 memset(&remoteaddr,0,sizeof(remoteaddr));//clean up the structure
		 
//********************************************************************
//SOCKET
//********************************************************************
		 s = socket(AF_INET, SOCK_STREAM, 0);
		 if (s <0) {
			 printf("socket failed\n");
		 }
		 localaddr.sin_family = AF_INET;
		 
		 //CONTROL CONNECTION:  port number = content of argv[1]
		 if (argc == 2) {
			 localaddr.sin_port = htons((u_short)atoi(argv[1])); //ipv4 only
		 }
		 else {
			 localaddr.sin_port = htons(1234);//default listening port //ipv4 only
		 }
		 localaddr.sin_addr.s_addr = INADDR_ANY;//server address should be local
		 
//********************************************************************
//BIND
//********************************************************************
		 if (bind(s,(struct sockaddr *)(&localaddr),sizeof(localaddr)) != 0) {
			 printf("Bind failed!\n");
			 exit(0);
		 }
		 
//********************************************************************
//LISTEN
//********************************************************************
		 listen(s,5);
		
//********************************************************************
//INFINITE LOOP
//********************************************************************
		 
		 //====================================================================================
		 while (1) {//Start of MAIN LOOP
		 //====================================================================================
			    addrlen = sizeof(remoteaddr);
//********************************************************************
//NEW SOCKET newsocket = accept  //CONTROL CONNECTION
//********************************************************************
			 printf("\n------------------------------------------------------------------------\n");
			 printf("SERVER is waiting for an incoming connection request...");
			 printf("\n------------------------------------------------------------------------\n");
			 ns = accept(s,(struct sockaddr *)(&remoteaddr),&addrlen); 
			 if (ns < 0 ) break;
				 
			 printf("\n============================================================================\n");
	 		 printf("connected to [CLIENT's IP %s , port %d] through SERVER's port %d",
			         inet_ntoa(remoteaddr.sin_addr),ntohs(remoteaddr.sin_port),ntohs(localaddr.sin_port)); //ipv4 only
			 printf("\n============================================================================\n");
			 //printf("detected CLIENT's port number: %d\n", ntohs(remoteaddr.sin_port));

			 //printf("connected to CLIENT's IP %s at port %d of SERVER\n",
			 //    inet_ntoa(remoteaddr.sin_addr),ntohs(localaddr.sin_port));
			 
			 //printf("detected CLIENT's port number: %d\n", ntohs(remoteaddr.sin_port));
//********************************************************************
//Respond with welcome message
//*******************************************************************
			 sprintf(send_buffer,"220 FTP Server ready. \r\n");
			 bytes = send(ns, send_buffer, strlen(send_buffer), 0);

			//********************************************************************
			//COMMUNICATION LOOP per CLIENT
			//********************************************************************

			 while (1) {
				 
				 n = 0;
				 //********************************************************************
			     //PROCESS message received from CLIENT
			     //********************************************************************
				 
				 while (1) {
//********************************************************************
//RECEIVE MESSAGE AND THEN FILTER IT
//********************************************************************
				     bytes = recv(ns, &receive_buffer[n], 1, 0);//receive byte by byte...

					 if ((bytes < 0) || (bytes == 0)) break;
					 if (receive_buffer[n] == '\n') { /*end on a LF*/
						 receive_buffer[n] = '\0';
						 break;
					 }
					 if (receive_buffer[n] != '\r') n++; /*Trim CRs*/
				 //=======================================================
				 } //End of PROCESS message received from CLIENT
				 //=======================================================
				 if ((bytes < 0) || (bytes == 0)) break;

				 printf("<< DEBUG INFO. >>: the message from the CLIENT reads: '%s\\r\\n' \n", receive_buffer);

//********************************************************************
//PROCESS COMMANDS/REQUEST FROM USER
//********************************************************************				 
				 if (strncmp(receive_buffer,"USER",4)==0)  {
					 printf("Logging in... \n");
					 sprintf(send_buffer,"331 Password required (anything will do really... :-) \r\n");
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes < 0) break;
				 }
				 //---
				 if (strncmp(receive_buffer,"PASS",4)==0)  {
					 
					 sprintf(send_buffer,"230 Public login sucessful \r\n");
					 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes < 0) break;
				 }
				 //---
				 if (strncmp(receive_buffer,"SYST",4)==0)  {
					 printf("Information about the system \n");
					 sprintf(send_buffer,"215 Windows 64-bit\r\n");
					 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes < 0) break;
				 }
				 //---
				 if (strncmp(receive_buffer,"OPTS",4)==0)  {
					 printf("unrecognised command \n");
					 sprintf(send_buffer,"502 command not implemented\r\n");
					 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes < 0) break;
				 }
				 //---
				 if (strncmp(receive_buffer,"QUIT",4)==0)  {
					 printf("Quit \n");
					 sprintf(send_buffer,"221 Connection close by client\r\n");
					 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes < 0) break;
					 // closesocket(ns);
				 }
				 //---
				 if(strncmp(receive_buffer,"PORT",4)==0) {
					 s_data_act = socket(AF_INET, SOCK_STREAM, 0);
					 //local variables
					 //unsigned char act_port[2];
					 int act_port[2];
					 int act_ip[4], port_dec;
					 char ip_decimal[40];
					 printf("===================================================\n");
					 printf("\n\tActive FTP mode, the client is listening... \n");
					 active=1;//flag for active connection
					 //int scannedItems = sscanf(receive_buffer, "PORT %d,%d,%d,%d,%d,%d",
					 //		&act_ip[0],&act_ip[1],&act_ip[2],&act_ip[3],
					 //     (int*)&act_port[0],(int*)&act_port[1]);
					 
					 int scannedItems = sscanf(receive_buffer, "PORT %d,%d,%d,%d,%d,%d",
							&act_ip[0],&act_ip[1],&act_ip[2],&act_ip[3],
					      &act_port[0],&act_port[1]);
					 
					 if(scannedItems < 6) {
		         	    sprintf(send_buffer,"501 Syntax error in arguments \r\n");
						printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
						bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					    //if (bytes < 0) break;
			            break;
		             }
					 
					 local_data_addr_act.sin_family=AF_INET;//local_data_addr_act  //ipv4 only
					 sprintf(ip_decimal, "%d.%d.%d.%d", act_ip[0], act_ip[1], act_ip[2],act_ip[3]);
					 printf("\tCLIENT's IP is %s\n",ip_decimal);  //IPv4 format
					 local_data_addr_act.sin_addr.s_addr=inet_addr(ip_decimal);  //ipv4 only
					 port_dec=act_port[0];
					 port_dec=port_dec << 8;
					 port_dec=port_dec+act_port[1];
					 printf("\tCLIENT's Port is %d\n",port_dec);
					 printf("===================================================\n");
					 local_data_addr_act.sin_port=htons(port_dec); //ipv4 only
					 if (connect(s_data_act, (struct sockaddr *)&local_data_addr_act, (int) sizeof(struct sockaddr)) != 0){
						 printf("trying connection in %s %d\n",inet_ntoa(local_data_addr_act.sin_addr),ntohs(local_data_addr_act.sin_port));
						 sprintf(send_buffer, "425 Something is wrong, can't start active connection... \r\n");
						 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
						 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
						 closesocket(s_data_act);
					 }
					 else {
						 sprintf(send_buffer, "200 PORT Command successful\r\n");
						 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
						 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
						 printf("Connected to client\n");
					 }

				 }
				 //---				 
				 //technically, LIST is different than NLST,but we make them the same here
				 if ( (strncmp(receive_buffer,"LIST",4)==0) || (strncmp(receive_buffer,"NLST",4)==0))   {
					 //system("ls > tmp.txt");//change that to 'dir', so windows can understand
					 system("dir > tmp.txt");
					 FILE *fin=fopen("tmp.txt","r");//open tmp.txt file
					 //sprintf(send_buffer,"125 Transfering... \r\n");
					 sprintf(send_buffer,"150 Opening ASCII mode data connection... \r\n");
					 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 char temp_buffer[80];
					 while (!feof(fin)){
						 fgets(temp_buffer,78,fin);
						 sprintf(send_buffer,"%s",temp_buffer);
						 if (active==0) send(ns_data, send_buffer, strlen(send_buffer), 0);
						 else send(s_data_act, send_buffer, strlen(send_buffer), 0);
					 }
					 fclose(fin);
					 //sprintf(send_buffer,"250 File transfer completed... \r\n");
					 sprintf(send_buffer,"226 File transfer complete. \r\n");
					 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (active==0 )closesocket(ns_data);
					 else closesocket(s_data_act);
						 
					 
					 //OPTIONAL, delete the temporary file
					 //system("del tmp.txt");
				 }
                 //---			    
			 //=================================================================================	 
			 }//End of COMMUNICATION LOOP per CLIENT
			 //=================================================================================
			 
//********************************************************************
//CLOSE SOCKET
//********************************************************************
			 closesocket(ns);
			 printf("DISCONNECTED from %s\n",inet_ntoa(remoteaddr.sin_addr));
			 //sprintf(send_buffer, "221 Bye bye, server close the connection ... \r\n");
			 //printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
			 //bytes = send(ns, send_buffer, strlen(send_buffer), 0);
			 
		 //====================================================================================
		 } //End of MAIN LOOP
		 //====================================================================================
		 closesocket(s);
		 printf("\nSERVER SHUTTING DOWN...\n");
		 exit(0);
}

