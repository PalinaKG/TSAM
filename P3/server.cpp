//
// Simple chat server for TSAM-409
//
// Command line: ./chat_server 4000 
//
// Author: Jacky Mallett (jacky@ru.is)
//
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <vector>
#include <list>

#include <iostream>
#include <sstream>
#include <thread>
#include <map>

#include <unistd.h>

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif


#define LOCAL_IP "127.0.0.1"

#define BACKLOG  5          // Allowed length of queue of waiting connections

// Simple class for handling connections from clients.
//
// Client(int socket) - socket to send/receive traffic from client.
class Client
{
  public:
    int sock;              // socket of client connection
    std::string name;           // Limit length of name of client's user

    std::string ip_addr; 
    // int port_number;
    std::string port_number;

    Client(int socket) : sock(socket){} 

    ~Client(){}            // Virtual destructor defined for base class
};

// Note: map is not necessarily the most efficient method to use here,
// especially for a server with large numbers of simulataneous connections,
// where performance is also expected to be an issue.
//
// Quite often a simple array can be used as a lookup table, 
// (indexed on socket no.) sacrificing memory for speed.

std::map<int, Client*> clients; // Lookup table for per Client information

// Open socket for specified port.
//
// Returns -1 if unable to create the socket for any reason.

int open_socket(int portno)
{
   struct sockaddr_in sk_addr;   // address settings for bind()
   int sock;                     // socket opened for this port
   int set = 1;                  // for setsockopt

   // Create socket for connection. Set to be non-blocking, so recv will
   // return immediately if there isn't anything waiting to be read.
#ifdef __APPLE__     
   if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
   {
      perror("Failed to open socket");
      return(-1);
   }
#else
   if((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
   {
     perror("Failed to open socket");
    return(-1);
   }
#endif

   // Turn on SO_REUSEADDR to allow socket to be quickly reused after 
   // program exit.

   if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
   {
      perror("Failed to set SO_REUSEADDR:");
   }
   set = 1;
#ifdef __APPLE__     
   if(setsockopt(sock, SOL_SOCKET, SOCK_NONBLOCK, &set, sizeof(set)) < 0)
   {
     perror("Failed to set SOCK_NOBBLOCK");
   }
#endif
   memset(&sk_addr, 0, sizeof(sk_addr));

   sk_addr.sin_family      = AF_INET;
   sk_addr.sin_addr.s_addr = INADDR_ANY;
   sk_addr.sin_port        = htons(portno);

   // Bind to socket to listen for connections from clients

   if(bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0)
   {
      perror("Failed to bind to socket:");
      return(-1);
   }
   else
   {
      return(sock);
   }
}

// Close a client's connection, remove it from the client list, and
// tidy up select sockets afterwards.

void closeClient(int clientSocket, fd_set *openSockets, int *maxfds)
{

     printf("Client closed connection: %d\n", clientSocket);

     // If this client's socket is maxfds then the next lowest
     // one has to be determined. Socket fd's can be reused by the Kernel,
     // so there aren't any nice ways to do this.

     close(clientSocket);      

     if(*maxfds == clientSocket)
     {
        for(auto const& p : clients)
        {
            *maxfds = std::max(*maxfds, p.second->sock);
        }
     }

     // And remove from the list of open sockets.

     FD_CLR(clientSocket, openSockets);

}

// Process command from client on the server

void clientCommand(int clientSocket, fd_set *openSockets, int *maxfds, 
                  char *buffer) 
{
  std::vector<std::string> tokens;
  std::string token;

  std::string commands[10];

  // Split command from client into tokens for parsing
  std::stringstream stream(buffer); //Default hegðu er að hún brýtur niður þar sem er bil
  bool startBool = false;
  bool endBool = false;
  //int start;
  //int end;
  int counter=0;


  for (unsigned int i=0;i<strlen(buffer);i++) 
  {
      if (buffer[i]=='*')
      {
          startBool=true;  
      }
      else if (buffer[i]=='#')
      {
          endBool=true;        
      }

      else if ((startBool == true) && (endBool == false))
      {
          if ((buffer[i] == ',') || (buffer[i] == ';'))
          {
              counter++;
          }
          else
          {
              commands[counter].push_back(buffer[i]);
          }      
      }
  }


  if (endBool){
    for (int i = 0; i < counter+1; i++){
      std::cout << commands[i] << std::endl;
    }
  }

  
  if(commands[0].compare("QUERYSERVERS") == 0)    
  {
    clients[clientSocket]->name = commands[1];
    std::string buffer_str = "*CONNECTED,";

    // CONNECTED,<MY GROUP ID>,<MY IP>,<MY PORT>
    
    // Making the query server list 
    for(auto const& pair : clients){    // go through every client 
        Client *client = pair.second;

        if (client->name == commands[1]){
            continue;
        }

        buffer_str.append(client->name);
        buffer_str.append(",");
        buffer_str.append(client->ip_addr);
        buffer_str.append(",");
        buffer_str.append(client->port_number);
        buffer_str.append(";");

    }

    buffer_str.append("#");

    std::cout << clientSocket << " -- vs -- " << clients[clientSocket]->sock <<  std::endl; 

    std::cout << "Query server list : " << buffer_str << std::endl;

    int nwrite = send(clientSocket , buffer_str.c_str(), strlen(buffer_str.c_str()),0);

    if(nwrite  == -1)
    {
        perror("send() to server failed: ");
    }    
    else{
        printf("Message %s    -- %d long sent!\n",buffer_str.c_str(),nwrite);
    }
  }

     
}

int main(int argc, char* argv[])
{
    bool finished;
    int listenSock;                 // Socket for connections to server
    int clientSock;                 // Socket of connecting client
    fd_set openSockets;             // Current open sockets 
    fd_set readSockets;             // Socket list for select()        
    fd_set exceptSockets;           // Exception socket list
    int maxfds;                     // Passed to select() as max fd in set
    struct sockaddr_in client;
    socklen_t clientLen;
    char buffer[1025];              // buffer for reading from clients

    if(argc != 2)
    {
        printf("Usage: chat_server <ip port>\n");
        exit(0);
    }

    // Setup socket for server to listen to

    const int port = atoi(argv[1]);


    // creating the server it self on a client list 
    // clients[-1] = new Client(-1);
    // clients[-1]->name = "MYSELF";
    // //clients[-1]->ip_addr = ip_address_hard_coded;
    // clients[-1]->ip_addr = LOCAL_IP;
    // clients[-1]->port_number = argv[1];



    listenSock = open_socket(port); 



    // If not main server then connect to main
    // https://www.geeksforgeeks.org/explicitly-assigning-port-number-client-socket/
    //if (port != 5000){
        printf("Connecting main server at port 5000...\n");

        
        struct sockaddr_in sk_addr;   // address of server
        sk_addr.sin_family      = AF_INET;
        sk_addr.sin_addr.s_addr = inet_addr(LOCAL_IP);
        sk_addr.sin_port        = htons(5000);

        // try{
        //    connect(listenSock, (struct sockaddr *)&sk_addr, sizeof(sk_addr) );
        // }
        // catch(errno){
            
            
        // }
        //connect(listenSock, (struct sockaddr *)&sk_addr, sizeof(sk_addr) );
        



        if(connect(listenSock, (struct sockaddr *)&sk_addr, sizeof(sk_addr) )< 0)
        {
            std::cout << "ERROR: " << errno << std::endl;
            printf("Failed to open socket to server: %s\n", argv[1]);
            perror("Connect failed: ");
            exit(0);
        }
        else{
            printf("Connection successful! \n");
        }
        

        char buffer_tx[1025];                        // buffer for writing to server
        
        strcpy(buffer_tx, "*QUERYSERVERS,P3_GROUP_12#");

        // std::cout<<"Sending message: " << buffer_tx << " \n serverSocket = " << serverSocket   << std::endl;
        std::cout<<"Sending message: " << buffer_tx << " \n clientSocket = " << clientSock   << std::endl;

        // Send query servers 
        //int nwrite = send(serverSocket , buffer_tx, strlen(buffer_tx),0);
        //int nwrite = send(listenSock , buffer_tx, strlen(buffer_tx),0);

    //     printf("nwrite = %d\n", nwrite);

    //    if(nwrite  == -1)
    //    {
    //        perror("send() to server failed: ");
    //    }       

    //}
    


    printf("Listening on port: %d\n", atoi(argv[1]));
    //int listenSock1;                 // Socket for connections to server

    //listenSock = open_socket(port); 
    //listen(listenSock, BACKLOG);

    if(listen(listenSock, BACKLOG) < 0)
    {
        std::cout << errno << std::endl;
        printf("Listen failed on port %s\n", argv[1]);
        perror("LISTEN FAILED");
        exit(0);
    }
    else 
    //Add listen socket to socket set we are monitoring
    {
        FD_ZERO(&openSockets);
        FD_SET(listenSock, &openSockets);
        maxfds = listenSock;
    }

    finished = false;

    while(!finished)
    {
        // Get modifiable copy of readSockets
        readSockets = exceptSockets = openSockets;
        memset(buffer, 0, sizeof(buffer));

        // Look at sockets and see which ones have something to be read()
        int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);

        if(n < 0)
        {
            perror("select failed - closing down\n");
            finished = true;
        }
        else
        {
            // First, accept  any new connections to the server on the listening socket
            if(FD_ISSET(listenSock, &readSockets))
            {
               clientSock = accept(listenSock, (struct sockaddr *)&client,
                                   &clientLen);
               printf("accept***\n");
               // Add new client to the list of open sockets
               FD_SET(clientSock, &openSockets);

               // And update the maximum file descriptor
               maxfds = std::max(maxfds, clientSock) ;

               // create a new client to store information.
               clients[clientSock] = new Client(clientSock);

               // Decrement the number of sockets waiting to be dealt with
               n--;

               printf("Client connected on server: %d\n", clientSock);
            }
            // Now check for commands from clients
            std::list<Client *> disconnectedClients;  
            while(n-- > 0)
            {
               for(auto const& pair : clients)
               {
                  Client *client = pair.second;

                  if(FD_ISSET(client->sock, &readSockets))
                  {
                      // recv() == 0 means client has closed connection
                      if(recv(client->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0)
                      {
                          disconnectedClients.push_back(client);
                          closeClient(client->sock, &openSockets, &maxfds);

                      }
                      // We don't check for -1 (nothing received) because select()
                      // only triggers if there is something on the socket for us.
                      else
                      {
                          std::cout << buffer << std::endl;
                          clientCommand(client->sock, &openSockets, &maxfds, buffer);
                      }
                  }
               }
               // Remove client from the clients list
               for(auto const& c : disconnectedClients)
                  clients.erase(c->sock);
            }
        }
    }
}
