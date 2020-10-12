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

    // bool isServer = false;    //  hægt að bæta við fullt af auka gæjum hér 

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
     // Remove client from the clients list
     clients.erase(clientSocket);

     // If this client's socket is maxfds then the next lowest
     // one has to be determined. Socket fd's can be reused by the Kernel,
     // so there aren't any nice ways to do this.

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
    // clients[clientSocket]->ip_addr = commands[2];
    // clients[clientSocket]->port_number = commands[3];

    // std::cout << clientSocket << std::endl; 

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
    

     // sendum á hann server lista
     
     // clients[clientSocket]->isServer = false;   // skítamix til að láta serverinn vita að þessi gaur er ekki server, hægt að nota t.d. í max limit á serverumm connectaðir við þennan server
  }

//   else if((commands[0].compare("CONNECT") == 0) && (commands.size() == 2))    
//   {
//      clients[clientSocket]->name = tokens[1];
//      // clients[clientSocket]->isServer = false;   // skítamix til að láta serverinn vita að þessi gaur er ekki server, hægt að nota t.d. í max limit á serverumm connectaðir við þennan server
//   }



//   else if(tokens[0].compare("LEAVE") == 0)
//   {
//       // Close the socket, and leave the socket handling
//       // code to deal with tidying up clients etc. when
//       // select() detects the OS has torn down the connection.
 
//       closeClient(clientSocket, openSockets, maxfds);
//   }
//   else if(tokens[0].compare("WHO") == 0)
//   {
//      std::cout << "Who is logged on" << std::endl;
//      std::string msg;

//      for(auto const& names : clients)     // skilar nöfnum þeirra sem eru tengdir við serverinn
//      {
//         msg += names.second->name + ",";

//      }
//      // Reducing the msg length by 1 loses the excess "," - which
//      // granted is totally cheating.
//      send(clientSocket, msg.c_str(), msg.length()-1, 0);

//   }
//   // This is slightly fragile, since it's relying on the order
//   // of evaluation of the if statement.
//   else if((tokens[0].compare("MSG") == 0) && (tokens[1].compare("ALL") == 0))  // sendir message á alla sem eru tengdir serverin
//   {
//       std::string msg;
//       for(auto i = tokens.begin()+2;i != tokens.end();i++) 
//       {
//           msg += *i + " ";
//       }

//       for(auto const& pair : clients)
//       {
//           send(pair.second->sock, msg.c_str(), msg.length(),0);  
//       }
//   }
//   else if(tokens[0].compare("MSG") == 0)                                      // sendir á þann sem er í token[1] ef hann er tengdur
//   {
//       for(auto const& pair : clients)
//       {
//           if(pair.second->name.compare(tokens[1]) == 0)
//           {
//               std::string msg;
//               for(auto i = tokens.begin()+2;i != tokens.end();i++) 
//               {
//                   msg += *i + " ";
//               }
//               send(pair.second->sock, msg.c_str(), msg.length(),0);
//           }
//       }
//   }
//   else
//   {
//       std::cout << "Unknown command from client:" << buffer << std::endl;
//   }
     
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


    // int client_counter = 0;         
    // int* client_list[15] = {-1};            // list of connected client sockets


    int n;

    if(argc != 2)
    {
        printf("Usage: chat_server <ip port>\n");
        exit(0);
    }

    // Setup socket for server to listen to

    const int port = atoi(argv[1]);
    char ip_address_hard_coded[] = "127.0.0.1";

    // creating the server it self on a client list 
    clients[-1] = new Client(-1);
    clients[-1]->name = "MYSELF";
    clients[-1]->ip_addr = ip_address_hard_coded;
    clients[-1]->port_number = argv[1];



    listenSock = open_socket(port); 
    printf("Listening on port: %d\n", port);

    if(listen(listenSock, BACKLOG) < 0)
    {
        printf("Listen failed on port %d\n", port);
        exit(0);
    }
    else 
    // Add listen socket to socket set we are monitoring
    {
        FD_ZERO(&openSockets);
        FD_SET(listenSock, &openSockets);
        maxfds = listenSock;
    }


    // If not main server then connect to main
    // https://www.geeksforgeeks.org/explicitly-assigning-port-number-client-socket/
    if (port != 5000){
        printf("Connecting main server at port 5000...\n");

        struct hostent *server;
        server = gethostbyname(ip_address_hard_coded);      // finnur automatic IP tölu ef sett er inn "skel.ru.is" en annars bara bein IP tala


        // struct sockaddr_in serv_addr;           // Socket address for server

        // bzero((char *) &serv_addr, sizeof(serv_addr));
        // serv_addr.sin_family = AF_INET;
        // bcopy((char *)server->h_addr,
        //     (char *)&serv_addr.sin_addr.s_addr,
        //     server->h_length);
        // serv_addr.sin_port = htons(5000);

        // int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

        //int serverSocket = listenSock;

        // Get modifiable copy of readSockets
        // readSockets = exceptSockets = openSockets;

        // Look at sockets and see which ones have something to be read()
                
        
        
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
            
           
            // printf("Success!\n");
        }


        // int set = 1;                              // Toggle for setsockopt
        // if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
        // {
        //     printf("Failed to set SO_REUSEADDR for port %d\n",5000);
        //     perror("setsockopt failed: ");
        // }

        // if(connect(serverSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr) )< 0)
        // {
        //     printf("Failed to open socket to server: %s\n", argv[1]);
        //     perror("Connect failed: ");
        //     exit(0);
        // }

        struct sockaddr_in sk_addr;   // address settings for bind() 
        sk_addr.sin_family      = AF_INET;
        sk_addr.sin_addr.s_addr = inet_addr(ip_address_hard_coded);
        sk_addr.sin_port        = htons(port);

        if(connect(clientSock, (struct sockaddr *)&sk_addr, sizeof(sk_addr) )< 0)
        {
            printf("Failed to open socket to server: %s\n", argv[1]);
            perror("Connect failed: ");
            exit(0);
        }
        

        char buffer_tx[1025];                        // buffer for writing to server
        
        strcpy(buffer_tx, "*QUERYSERVERS,P3_GROUP_12#");

        // std::cout<<"Sending message: " << buffer_tx << " \n serverSocket = " << serverSocket   << std::endl;
        std::cout<<"Sending message: " << buffer_tx << " \n clientSocket = " << clientSock   << std::endl;

        // Send query servers 
        //int nwrite = send(serverSocket , buffer_tx, strlen(buffer_tx),0);
        int nwrite = send(clientSock , buffer_tx, strlen(buffer_tx),0);

        printf("nwrite = %d\n", nwrite);

       if(nwrite  == -1)
       {
           perror("send() to server failed: ");
       }       

    //     sleep(1);

    //    bzero(buffer_tx, sizeof(buffer_tx));
    //    if (recv(serverSocket, buffer_tx, strlen(buffer_tx),0)<0){
    //        perror("recv() from server failed: ");
    //     }
    //    std::cout<< "Message from server: " <<  buffer_tx <<std::endl; 
    }


    finished = false;

    while(!finished)
    {

        // Get modifiable copy of readSockets
        readSockets = exceptSockets = openSockets;
        memset(buffer, 0, sizeof(buffer));

        // Look at sockets and see which ones have something to be read()

        printf("pre-select -- %d\n", maxfds+1);
        n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);
        printf("post-select\n");

        if(n < 0)
        {
            perror("select failed - closing down\n");
            finished = true;
        }
        else
        {
            // First, accept  any new connections to the server on the listening socket
            if (clients.size() == 16){
                printf("Max capacity of clients reached ( max: 15 ) \n");
            }
            if(FD_ISSET(listenSock, &readSockets)  && (clients.size() <= 16))   // server itself is clients[-1] which means max clients is 16
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
                          printf("Client closed connection: %d", client->sock);
                          close(client->sock);      

                          closeClient(client->sock, &openSockets, &maxfds);

                      }
                      // We don't check for -1 (nothing received) because select()
                      // only triggers if there is something on the socket for us.
                      else
                      {
                          std::cout << "Received buffer: "  << buffer << std::endl;
                          clientCommand(client->sock, &openSockets, &maxfds, 
                                        buffer);
                      }
                  }
               }
            }
        }
    }
}
