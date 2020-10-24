//
// Simple chat server for TSAM-409
//
// Command line: ./chat_server 4000
//
// Author: Jacky Mallett (jacky@ru.is)
//
#include <algorithm>
#include <arpa/inet.h>
#include <errno.h>
#include <list>
#include <map>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include <iostream>
#include <map>
#include <sstream>
#include <thread>

#include <chrono>
#include <ctime>

#include <unistd.h>

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define LOCAL_IP "127.0.0.1"

#define BACKLOG 5 // Allowed length of queue of waiting connections

// Simple class for handling connections from clients.
//
// Client(int socket) - socket to send/receive traffic from client.
// auto func()
// {
//     return std::chrono::system_clock::now();
// }

class Client {
public:
  int sock;         // socket of client connection
  std::string name; // Limit length of name of client's user

  std::string ip_addr;
  // int port_number;
  std::string port_number;
  time_t timestamp;

  Client(int socket) : sock(socket) {}

  ~Client() {} // Virtual destructor defined for base class
};

class Myself {
public:
  const int max_messages = 100;
  std::string name;
  std::string IP;
  std::string port;
  std::string msg[100];       // saving the messages received
  std::string msg_other[100]; // saving received message intended for other
                              // clients that are not currently connected
};
class Myself myself;

// Note: map is not necessarily the most efficient method to use here,
// especially for a server with large numbers of simulataneous connections,
// where performance is also expected to be an issue.
//
// Quite often a simple array can be used as a lookup table,
// (indexed on socket no.) sacrificing memory for speed.

std::map<int, Client *> clients; // Lookup table for per Client information
// std::map<int, auto> timestamps; // Lookup table for per Client information
std::list<Client *> disconnectedClients;

// Open socket for specified port.
//
// Returns -1 if unable to create the socket for any reason.

int open_socket(int portno) {
  struct sockaddr_in sk_addr; // address settings for bind()
  int sock;                   // socket opened for this port
  int set = 1;                // for setsockopt

  // Create socket for connection. Set to be non-blocking, so recv will
  // return immediately if there isn't anything waiting to be read.
#ifdef __APPLE__
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Failed to open socket");
    return (-1);
  }
#else
  if ((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0) {
    perror("Failed to open socket");
    return (-1);
  }
#endif

  // Turn on SO_REUSEADDR to allow socket to be quickly reused after
  // program exit.

  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0) {
    perror("Failed to set SO_REUSEADDR:");
  }
  set = 1;
#ifdef __APPLE__
  if (setsockopt(sock, SOL_SOCKET, SOCK_NONBLOCK, &set, sizeof(set)) < 0) {
    perror("Failed to set SOCK_NOBBLOCK");
  }
#endif
  memset(&sk_addr, 0, sizeof(sk_addr));

  sk_addr.sin_family = AF_INET;
  sk_addr.sin_addr.s_addr = INADDR_ANY;
  sk_addr.sin_port = htons(portno);

  // Bind to socket to listen for connections from clients

  if (bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0) {
    perror("Failed to bind to socket:");
    return (-1);
  } else {
    return (sock);
  }
}

// Close a client's connection, remove it from the client list, and
// tidy up select sockets afterwards.

void closeClient(int clientSocket, fd_set *openSockets, int *maxfds) {

  printf("Client closed connection: %d\n", clientSocket);

  // If this client's socket is maxfds then the next lowest
  // one has to be determined. Socket fd's can be reused by the Kernel,
  // so there aren't any nice ways to do this.

  close(clientSocket);

  if (*maxfds == clientSocket) {
    for (auto const &p : clients) {
      *maxfds = std::max(*maxfds, p.second->sock);
    }
  }

  // And remove from the list of open sockets.

  FD_CLR(clientSocket, openSockets);
}

// Process command from client on the server

void clientCommand(int clientSocket, fd_set *openSockets, int *maxfds,
                   char *buffer) {
  std::vector<std::string> tokens;
  std::string token;

  std::string commands[1024];

  // Split command from client into tokens for parsing
  std::stringstream stream(
      buffer); // Default hegðu er að hún brýtur niður þar sem er bil
  bool startBool = false;
  bool endBool = false;
  // int start;
  // int end;
  int counter = 0;

  for (unsigned int i = 0; i < strlen(buffer); i++) {
    if (buffer[i] == '*') {
      startBool = true;
    } else if (buffer[i] == '#') {
      endBool = true;
    }

    else if ((startBool == true) && (endBool == false)) {
      if ((buffer[i] == ',') || (buffer[i] == ';')) {
        counter++;
      } else {
        commands[counter].push_back(buffer[i]);
      }
    }

    //   if (endBool){
    //     for (int i = 0; i < counter+1; i++){
    //       std::cout << commands[i] << std::endl;
    //     }
    //   }

    if ((startBool == true) && (endBool == true)) {
      std::cout << "COMMANDS: " << commands[0] << std::endl;

      clients[clientSocket]->timestamp = time(0);
      if (commands[0].compare("SENDMSG") == 0) {
        commands[0] = "SEND_MSG";
        commands[3] = commands[2];
        commands[2] = myself.name;
      } else if (commands[0].compare("GETMSG") == 0) {
        commands[0] = "GET_MSG";
        std::cout << "HELLO1" << std::endl;
      }

      if (commands[0].compare("QUERYSERVERS") == 0) {
        clients[clientSocket]->name = commands[1];
        // clients[clientSocket]->ip_addr = commands[2];
        // clients[clientSocket]->port_number = commands[3];

        // std::cout << clientSocket << std::endl;

        std::string buffer_str = "*CONNECTED,";

        // CONNECTED,<MY GROUP ID>,<MY IP>,<MY PORT>

        // Making the query server list
        buffer_str.append(myself.name);
        buffer_str.append(",");
        buffer_str.append(myself.IP);
        buffer_str.append(",");
        buffer_str.append(myself.port);
        buffer_str.append(";");
        for (auto const &pair : clients) { // go through every client
          Client *client = pair.second;
          if ((((client->name).compare(commands[1])) == 0) ||
              (((client->name).compare("")) == 0)) {
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

        // std::cout << clientSocket << " -- vs -- " <<
        // clients[clientSocket]->sock <<  std::endl;

        std::cout << "Query server list : " << buffer_str << std::endl;

        int nwrite = send(clientSocket, buffer_str.c_str(),
                          strlen(buffer_str.c_str()), 0);
        if (nwrite == -1) {
          perror("send() to server failed: ");
        } else {
          printf("Message %s    -- %d long sent!\n", buffer_str.c_str(),
                 nwrite);
        }
      }

      else if (commands[0].compare("CONNECTED") == 0) {
        std::cout << "CONNECTED" << std::endl;
        clients[clientSocket]->name = commands[1];
        clients[clientSocket]->ip_addr = commands[2];
        clients[clientSocket]->port_number = commands[3];
      }

      else if (commands[0].compare("SEND_MSG") == 0) {
        //   for (int i=0;i<sizeof(clients);i++)
        //   {
        //       if ()
        //   }

        if (commands[1].compare(myself.name) == 0) {
          for (int i = 0; i < myself.max_messages; i++) {
            if (myself.msg[i].empty()) {
              (myself.msg)[i] = commands[2] + "," + commands[3];
              break;
            }
          }

        } else {

          int sock = 0;
          for (auto const &pair : clients) {
            Client *client = pair.second;
            if (client->name.compare(commands[1]) == 0) {
              std::cout << "CLIENTSOCK: " << client->sock << std::endl;
              sock = pair.first;
              break;
            }
          }

          // char sendBuffer[] = (char[]) commands[i];
          std::cout << "SOCK: " << sock << std::endl;

          int nwrite = send(sock, buffer, strlen(buffer), 0);
          printf("nwrite = %d\n", nwrite);

          if (nwrite == -1) {
            for (int i = 0; i < myself.max_messages; i++) {
              if (myself.msg_other[i].empty()) {
                (myself.msg_other)[i] = commands[1] + "," + commands[2] + "," +
                                        commands[3]; // TO,FROM,MSG
                break;
              }
            }
            std::cout << "SEND TO FAILED: " << std::endl;
            perror("send() to server failed: ");
          }
        }

      } else if (commands[0].compare("GET_MSG") == 0) {
        std::cout << "HELLO2" << std::endl;
        if (commands[1].compare(myself.name) == 0) {
          for (int i = 0; i < myself.max_messages; i++) {
            if (!(myself.msg)[i].empty()) {
              int index = (myself.msg[i]).find(',');
              std::string buf = (myself.msg)[i];
              std::string buffer_str =
                  "*" + buf.substr(index + 1, sizeof(buf) - 1) + "#";
              int nwrite = send(clientSocket, buffer_str.c_str(),
                                strlen(buffer_str.c_str()), 0);
              if (nwrite == -1) {
                perror("send() to server failed: ");
              } else {
                printf("Message %s    -- %d long sent!\n", buffer_str.c_str(),
                       nwrite);
              }
            }
          }
        } else {
          for (int i = 0; i < myself.max_messages; i++) {
            int index = (myself.msg_other[i]).find(',');
            std::string receiver = (myself.msg_other)[i].substr(0, index);
            if (receiver.compare(commands[1]) == 0) {
              std::string buffer_str =
                  "*SEND_MSG," + (myself.msg_other)[i] + "#";
              int nwrite = send(clientSocket, buffer_str.c_str(),
                                strlen(buffer_str.c_str()), 0);
              if (nwrite == -1) {
                perror("send() to server failed: ");
              } else {
                printf("Message %s    -- %d long sent!\n", buffer_str.c_str(),
                       nwrite);
                (myself.msg_other)[i] = "";
              }
            }
          }
        }

      } else if (commands[0].compare("LEAVE") == 0) {
        for (auto const &pair : clients) {
          Client *client = pair.second;

          std::cout << "CLIENT1: " << client->name << std::endl;
        }

        for (auto const &pair : clients) {
          Client *client = pair.second;
          if ((client->ip_addr == commands[1]) &&
              (client->port_number == commands[2])) {
            int c = client->sock;
            disconnectedClients.push_back(client);
            closeClient(c, openSockets, maxfds);
            break;
          }
        }
        for (auto const &pair : clients) {

          Client *client = pair.second;
          std::cout << "CLIENT2: " << client->name << std::endl;
        }

      } else if ((commands[0].compare("STATUSREQ") == 0) ||
                 (commands[0].compare("STATUSRESP") == 0)) {
        std::map<std::string, int> n_msg; // Lookup table for msg
        std::string msg, server_name;
        int index;
        for (int i = 0; i < myself.max_messages; i++) {
          msg = myself.msg_other[i];
          index = msg.find(',');
          server_name = msg.substr(0, index);
          if (n_msg.find(server_name) != n_msg.end()) {
            n_msg[server_name]++;

          } else {
            n_msg[server_name] = 1;
          }
        }

        for (auto const pair : n_msg) {
          std::cout << "KEY: " << pair.first << "   VALUE: " << pair.second
                    << std::endl;
        }
        std::string to_group;
        if (commands[0].compare("STATUSRESP") == 0) {
          to_group = commands[2];
        } else {
          to_group = clients[clientSocket]->name;
        }
        std::string buffer_str = "STATUSRESP," + commands[1] + "," + to_group;
        for (auto const pair : n_msg) {
          if (pair.first != "") {
            buffer_str = buffer_str + "," + pair.first + "," +
                         std::to_string(pair.second);
          }
        }
        int send_sock;
        if (commands[0].compare("STATUSRESP") == 0) {
          for (auto const pair : clients) {
            Client *client = pair.second;
            std::cout << "CLIENT: " << client->name << "   " << commands[2]
                      << std::endl;

            if ((client->name).compare(commands[2]) == 0) {

              send_sock = client->sock;
              break;
            }
          }
        } else {
          send_sock = clientSocket;
        }

        std::cout << "SOCK: " << send_sock << std::endl;

        int nwrite =
            send(send_sock, buffer_str.c_str(), strlen(buffer_str.c_str()), 0);
        if (nwrite == -1) {
          perror("send() to server failed: ");
        } else {
          printf("Message %s    -- %d long sent!\n", buffer_str.c_str(),
                 nwrite);
        }

      } else if (commands[0].compare("CONNECT") == 0) {

        printf("Connecting main server at port 5000...\n");

        struct sockaddr_in sk_addr; // address of server
        sk_addr.sin_family = AF_INET;
        sk_addr.sin_addr.s_addr = inet_addr(LOCAL_IP);
        sk_addr.sin_port = htons(stoi(commands[1]));

        // try{
        //    connect(listenSock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)
        //    );
        // }
        // catch(errno){

        // }
        // connect(listenSock, (struct sockaddr *)&sk_addr, sizeof(sk_addr) );
        // int listenSock = open_socket(stoi(commands[1]));
        int sock;
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
          perror("Failed to open socket");
        }
        if (connect(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0) {
          // std::cout << "ERROR: " << errno << std::endl;
          printf("Failed to open socket to server\n");
          perror("Connect failed: ");
        } else {
          printf("Connection successful! \n");
        }
        clients[sock] = new Client(sock);
        std::string buffer_tx; // buffer for writing to server

        buffer_tx = "*QUERYSERVERS," + myself.name + "#";

        // std::cout<<"Sending message: " << buffer_tx << " \n serverSocket = "
        // << serverSocket   << std::endl; std::cout<<"Sending message: " <<
        // buffer_tx << " \n clientSocket = " << clientSock   << std::endl;

        // Send query servers
        // int nwrite = send(serverSocket , buffer_tx, strlen(buffer_tx),0);
        int nwrite =
            send(sock, buffer_tx.c_str(), strlen(buffer_tx.c_str()), 0);

        printf("nwrite = %d\n", nwrite);

        if (nwrite == -1) {
          perror("send() to server failed: ");
        }
        // FD_ZERO(openSockets);
        FD_SET(sock, openSockets);
        *maxfds = std::max(sock, *maxfds);
      }
      startBool = false;
      endBool = false;
      commands->clear();
      counter = 0;
    }
  }

  //   if((tokens[0].compare("CONNECT") == 0) && (tokens.size() == 2))
  //   {
  //      clients[clientSocket]->name = tokens[1];
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

  //      for(auto const& names : clients)
  //      {
  //         msg += names.second->name + ",";

  //      }
  //      // Reducing the msg length by 1 loses the excess "," - which
  //      // granted is totally cheating.
  //      send(clientSocket, msg.c_str(), msg.length()-1, 0);

  //   }
  //   // This is slightly fragile, since it's relying on the order
  //   // of evaluation of the if statement.
  //   else if((tokens[0].compare("MSG") == 0) && (tokens[1].compare("ALL") ==
  //   0))
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
  //   else if(tokens[0].compare("MSG") == 0)
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

int main(int argc, char *argv[]) {
  bool finished;
  int listenSock;       // Socket for connections to server
  int clientSock;       // Socket of connecting client
  fd_set openSockets;   // Current open sockets
  fd_set readSockets;   // Socket list for select()
  fd_set exceptSockets; // Exception socket list
  int maxfds;           // Passed to select() as max fd in set
  struct sockaddr_in client;
  socklen_t clientLen;
  char buffer[5000]; // buffer for reading from clients

  if (argc != 3) {
    printf("Usage: chat_server <ip port> <server name>\n");
    exit(0);
  }

  // Setup socket for server to listen to

  const int port = atoi(argv[1]);

  // creating the server it self on a client list

  myself.name = argv[2];
  myself.IP = LOCAL_IP;
  myself.port = argv[1];

  listenSock = open_socket(port);

  // If not main server then connect to main
  // https://www.geeksforgeeks.org/explicitly-assigning-port-number-client-socket/

  printf("Listening on port: %d\n", atoi(argv[1]));
  // int listenSock1;                 // Socket for connections to server

  // listenSock = open_socket(port);
  // listen(listenSock, BACKLOG);

  if (listen(listenSock, BACKLOG) < 0) {
    printf("Listen failed on port %s\n", argv[1]);
    perror("LISTEN FAILED");
    exit(0);
  } else
  // Add listen socket to socket set we are monitoring
  {
    FD_ZERO(&openSockets);
    FD_SET(listenSock, &openSockets);
    maxfds = listenSock;
  }

  finished = false;

  while (!finished) {
    // Get modifiable copy of readSockets
    readSockets = exceptSockets = openSockets;
    memset(buffer, 0, sizeof(buffer));

    // Look at sockets and see which ones have something to be read()
    std::cout << "PRESELECT" << std::endl;
    int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);
    std::cout << "POSTSELECT" << std::endl;

    if (n < 0) {
      perror("select failed - closing down\n");
      finished = true;
    } else {
      // First, accept  any new connections to the server on the listening
      // socket
      if ((FD_ISSET(listenSock, &readSockets)) && clients.size() <= 16) {

        clientSock = accept(listenSock, (struct sockaddr *)&client, &clientLen);
        std::cout << "clientSock: " << clientSock << std::endl;

        printf("accept***\n");
        // Add new client to the list of open sockets
        FD_SET(clientSock, &openSockets);

        // And update the maximum file descriptor
        maxfds = std::max(maxfds, clientSock);

        // create a new client to store information.
        clients[clientSock] = new Client(clientSock);
        clients[clientSock]->timestamp = time(0);
        // timestamps[clientSock]=std::chrono::system_clock::now();

        // Decrement the number of sockets waiting to be dealt with
        n--;

        printf("Client connected on server: %d\n", clientSock);
        std::string buffer_tx;

        buffer_tx = "*QUERYSERVERS," + myself.name + "#";
        int nwrite =
            send(clientSock, buffer_tx.c_str(), strlen(buffer_tx.c_str()), 0);
      }
      // Now check for commands from clients
      disconnectedClients.clear();
      while (n-- > 0) {

        for (auto const &pair : clients) {
          Client *client = pair.second;

          if ((time(0) - (clients[clientSock]->timestamp)) >= 60) {
            std::string buffer_str = "KEEPALIVE";
            std::cout << buffer_str << std::endl;
            send(clientSock, buffer_str.c_str(), strlen(buffer_str.c_str()), 0);
          }
          //   auto current_time = std::chrono::system_clock::now();
          //   std::chrono::duration<double> elapsed_seconds =
          //   (current_time-(timestamps[clients->sock])).count(); if
          //   (elapsed_seconds >= 60)
          //   {
          //       std::string buffer_str="KEEPALIVE";

          //       send(clientSock,buffer_str.c_str(),strlen(buffer_str.c_str()),0);
          //   }
          std::cout << "SET: " << (FD_ISSET(client->sock, &readSockets))
                    << std::endl;
          if (FD_ISSET(client->sock, &readSockets)) {
            // recv() == 0 means client has closed connection
            if (recv(client->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0) {
              disconnectedClients.push_back(client);
              closeClient(client->sock, &openSockets, &maxfds);

            }
            // We don't check for -1 (nothing received) because select()
            // only triggers if there is something on the socket for us.
            else {
              std::cout << "RECEIVE_BUFFER: " << buffer << std::endl;
              clientCommand(client->sock, &openSockets, &maxfds, buffer);
            }
          }
        }
        // Remove client from the clients list
        for (auto const &c : disconnectedClients) {
          clients.erase(c->sock);
        }
      }
    }
  }
}
