#include <stdio.h>	//for printf
#include <string.h> //memset
#include <sys/socket.h>	//for socket ofcourse
#include <stdlib.h> //for exit(0);
#include <errno.h> //For errno - the error number
#include <netinet/udp.h>	//Provides declarations for udp header
#include <netinet/ip.h>	//Provides declarations for ip header
#include <arpa/inet.h> // inet_addr
#include <netinet/in.h>

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>



void print_buffer(char* buf, int start, int finish){
	for (int i = start; i<finish; i++){
		char c = buf[i];	
		std::cout<<c;	
	}
	printf("\n");
}



int main(int argc, char* argv[]){

	if (argc != 3){
		printf("Usage: knocker <port> <hidden port 1>,<hidden port 2>  f.x. sudo ./knocker 4042 4008,4015\n");
        exit(0);
	}

	//Create a raw UDP socket
	int s_raw = socket (AF_INET, SOCK_RAW, IPPROTO_UDP);
	if(s_raw == -1)
	{
		perror("Failed to create raw socket -- (Remember to run with root privileges)");
		exit(1);
	}

	int one = 1;
	const int *val = &one;

	if (setsockopt (s_raw, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)							// header included
	{
		printf ("Error setting IP_HDRINCL. Error number : %d . Error message : %s \n" , errno , strerror(errno));
		exit(0);
	}
		
	
	//Datagram to represent the packet
	char datagram[1000000] , *data;
	
	//zero out the packet buffer
	memset (datagram, 0, sizeof(datagram));
	
	//IP header
	struct ip *iph = (struct ip *) datagram;
	
	//UDP header
	struct udphdr *udph = (struct udphdr *) (datagram + sizeof (struct ip));
	
	//socket address structures
	struct sockaddr_in dest;
	struct sockaddr_in src;
	
	//Data part
	data = datagram + sizeof(struct ip) + sizeof(struct udphdr);
	strcpy(data , argv[2]);

	

	// destionation
	dest.sin_family = AF_INET;
	dest.sin_port =  htons(atoi(argv[1])); // ;
	dest.sin_addr.s_addr = inet_addr ("130.208.243.61");  // IP for skel.ru.is
	

	
	// source
    int const src_port = 5000;
	src.sin_family = AF_INET;
	src.sin_port =  src_port; 

	std::ifstream file("IPv4address.txt");   // get host IPv4 address from text file created with make file
    std::string str; 
    std::getline(file, str);
    
	src.sin_addr.s_addr = inet_addr (str.c_str());
	

	if(bind(s_raw, (struct sockaddr *)&src, sizeof(src)) < 0)   // bind raw socket with source port
   	{
      perror("Failed to bind to socket:");
      return(-1);
   	}
	   
	
	//Fill in the IP Header
	iph->ip_hl = 5;                                                                 // internet header length i.e. number of 32 bit words in header
	iph->ip_v = 4;                                                                  // IP version 
	iph->ip_tos = 0;                                                                // Type of service 
	iph->ip_len = sizeof (struct ip) + sizeof (struct udphdr) + strlen(data);       // total length of header
	iph->ip_id =  11;                                                               // Identification 
	iph->ip_off = 0;                                                                // Fragment offset 
	iph->ip_ttl = 255;                                                              // Time-to-Live set to max
	iph->ip_p = IPPROTO_UDP;                                                        // Internet protocol UDP
	iph->ip_sum = 0;                                                                // checksum
	iph->ip_src = src.sin_addr;                                                     // Source address structure
	iph->ip_dst = dest.sin_addr;                                                    // Destination address structure
	

	//Fill in the UDP header -- every field in big endian (network order)
	udph->uh_sport= htons(src_port);                                                // Source port
	udph->uh_dport= htons(atoi(argv[1]));                                           // Destination port
	udph->uh_ulen= htons(8 + strlen(data));	                                        // Total length of header
	udph->uh_sum = 0;   



	printf("Sending secret ports %s to port %s  (if it takes longer than 2sek, kill program (Ctrl+C) and retry)\n\n",argv[2],argv[1]);

    // sending hidden ports to oracle
    if (sendto (s_raw, datagram, iph->ip_len , 0, (struct sockaddr *) &dest, sizeof (dest)) < 0)
    {
        perror("sendto failed");
    }


	socklen_t socklen = sizeof(dest);
    char buffer[4096];
	int shift_over_headers = sizeof(struct ip) + sizeof(struct udphdr);

    int c = recvfrom(s_raw, buffer, sizeof(buffer), 0, (struct sockaddr *) &dest, &socklen);
    if (c<0){
		perror("recvfrom failed");
		exit(0);
    } 
    else
    {
		printf("Knock order: ");
        print_buffer((char*) buffer, shift_over_headers, c);
		printf("\n");
    }
	

    strcpy(data , "Ennyn Durin Aran Moria. Pedo Mellon a Minno. Im Narvi hain echant. Celebrimbor o Eregion teithant i thiw hin."); // secret phrase
    
	udph->uh_ulen= htons(8 + strlen(data));	
	iph->ip_len = sizeof (struct ip) + sizeof (struct udphdr) + strlen(data);

    char recv_buffer[4096];

    
    // sending to secret ports
    for (int i=shift_over_headers ; i<c ;i=i+5)  // jump every 5th byte in the received buffer
    {

        char char_arr[] = {buffer[i], buffer[i+1], buffer[i+2], buffer[i+3]};  // assamble 4 bytes to get one port
        int port;
        sscanf (char_arr,"%d",&port);		// char array -> integer

	    std::cout << "sending to port: " << port << std::endl;

		// set ports
        dest.sin_port = htons(port);	
	    udph->uh_dport= htons(port);


		// Send secret phrase to port
        if (sendto (s_raw, datagram, iph->ip_len ,	0, (struct sockaddr *) &dest, sizeof (dest)) < 0)
        {
            perror("sendto failed");
			exit(0);
        }

        socklen = sizeof(dest);
        int cc = recvfrom(s_raw, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *) &dest, &socklen);
        if (cc<0){
            printf("recvfrom failed");
			exit(0);
        } 
        else{            
            print_buffer((char*) recv_buffer, shift_over_headers, cc);
			printf("\n");
        }
    }    

    return 0;
}




















