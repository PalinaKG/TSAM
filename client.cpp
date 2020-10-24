//
// Simple chat client for TSAM-409
//
// Command line: ./chat_client 4000
//
// Author: Jacky Mallett (jacky@ru.is)
//
#include <algorithm>
#include <arpa/inet.h>
#include <ctime>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <map>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include <iostream>
#include <map>
#include <sstream>
#include <thread>

std::string list_of_servers[15];
std::ofstream myfile;
// char list_of_servers[15];
int nr_of_servers;

// Threaded function for handling responss from server

void create_list_of_servers(char recvBuffer[1025]) {
  int counter = 0;
  std::string commands[10];

  for (unsigned int i = 0; i < strlen(recvBuffer); i++) {

    if ((recvBuffer[i] == ',') || (recvBuffer[i] == ';')) {
      counter++;
    } else {
      commands[counter].push_back(recvBuffer[i]);
    }
  }
  nr_of_servers = 0;
  for (int i = 1; i < counter; i = i + 3) {
    list_of_servers[nr_of_servers] = commands[i];
    nr_of_servers++;
  }
}

void listenServer(int serverSocket) {
  int nread;             // Bytes read from socket
  char sendBuffer[1025]; // Buffer for reading input
  char recvBuffer[1025];

  while (true) {
    memset(recvBuffer, 0, sizeof(recvBuffer));
    nread = read(serverSocket, recvBuffer, sizeof(recvBuffer));

    if (nread == 0) // Server has dropped us
    {
      printf("Over and Out\n");
      exit(0);
    } else if (nread > 0) {
      std::string buffer_str = ((std::string)recvBuffer).substr(1, 12);
      // std::cout << "STRING: " << buffer_str << std::endl;
      myfile.open("Client.txt", std::ios::app);
      time_t timi = time(0);
      myfile << ctime(&timi) << std::endl;
      myfile << recvBuffer << std::endl;
      myfile.close();
      if ((buffer_str.compare("QUERYSERVERS") != 0) &&
          (buffer_str.compare("KEEPALIVE") != 0)) {

        std::cout << "BUFFER: " << std::endl;
        printf("%s\n", recvBuffer);

        create_list_of_servers(recvBuffer);
      } else {
        bzero(recvBuffer, sizeof(recvBuffer));
      }
    }
  }
}

bool compare_strings(std::string string1, std::string string2) {
  bool same = true;
  if ((string1.length() - 1) != string2.length()) {
    return false;
  }

  for (int i = 0; i < string2.length(); i++) {
    if (string1[i] != string2[i]) {
      return false;
    }
  }
  return true;
}

int main(int argc, char *argv[]) {
  nr_of_servers = 0;
  struct addrinfo hints, *svr;  // Network host entry for server
  struct sockaddr_in serv_addr; // Socket address for server
  int serverSocket;             // Socket used for server
  int nwrite;                   // No. bytes written to server
  char sendBuffer[1025];        // Buffer for reading input
  char recvBuffer[1025];        // buffer for writing to server
  bool finished;
  int set = 1; // Toggle for setsockopt

  std::string port_string = argv[2];

  if (argc != 3) {
    printf("Hard coded to connect to port 5000\n");
    printf("Usage: chat_client <ip  address>\n");
    // printf("Ctrl-C to terminate\n");

    exit(0);
  }

  hints.ai_family = AF_INET; // IPv4 only addresses
  hints.ai_socktype = SOCK_STREAM;

  memset(&hints, 0, sizeof(hints));

  if (getaddrinfo(argv[1], port_string.c_str(), &hints, &svr) != 0) {
    perror("getaddrinfo failed: ");
    exit(0);
  }

  struct hostent *server;
  server = gethostbyname(argv[1]); // finnur automatic IP tölu ef sett er inn
                                   // "skel.ru.is" en annars bara bein IP tala

  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(stoi(port_string));

  serverSocket = socket(AF_INET, SOCK_STREAM, 0);

  // Turn on SO_REUSEADDR to allow socket to be quickly reused after
  // program exit.

  if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) <
      0) {
    printf("Failed to set SO_REUSEADDR for port %s\n", port_string.c_str());
    perror("setsockopt failed: ");
  }

  if (connect(serverSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <
      0) {
    printf("Failed to open socket to server: %s\n", argv[1]);
    perror("Connect failed: ");
    exit(0);
  }

  // Listen and print replies from server
  std::thread serverThread(listenServer, serverSocket);

  finished = false;
  while (!finished) {
    bzero(sendBuffer, sizeof(sendBuffer));

    printf("client input: ");
    fgets(sendBuffer, sizeof(sendBuffer), stdin);

    // std::string buff=(std::string) sendBuffer;
    // std::string buff2="*LISTSERVER#";

    if (compare_strings((std::string)sendBuffer, "*LISTSERVERS#")) {
      char s_buffer[] = "*QUERYSERVERS,client#";
      nwrite = send(serverSocket, s_buffer, strlen(s_buffer), 0);
      if (nwrite == -1) {
        perror("send() to server failed: ");
        finished = true;
      }
      bzero(recvBuffer, sizeof(recvBuffer));
      if (recv(serverSocket, recvBuffer, strlen(recvBuffer), 0) < 0) {
        perror("recv() from server failed: ");
        finished = true;
      }
      sleep(1);
      std::cout << "SERVERS: " << std::endl;
      for (int i = 0; i < nr_of_servers; i++) {
        std::cout << list_of_servers[i] << std::endl;
      }

    } else {
      nwrite = send(serverSocket, sendBuffer, strlen(sendBuffer), 0);

      if (nwrite == -1) {
        perror("send() to server failed: ");
        finished = true;
      }
      bzero(recvBuffer, sizeof(recvBuffer));
      if (recv(serverSocket, recvBuffer, strlen(recvBuffer), 0) < 0) {
        perror("recv() from server failed: ");
        finished = true;
      }
      sleep(1);
    }

    // std::cout<< "Message from server: " <<  recvBuffer <<std::endl;
    // int start;
    // int end;
  }

  //    for (int i=0;i<sizeof(list_of_servers);i++)
  //    {
  //        std::cout << list_of_servers[i] << std::endl;
  //    }
}
