#include<stdio.h>	//for printf
#include<string.h> //memset
#include<sys/socket.h>	//for socket ofcourse
#include<stdlib.h> //for exit(0);
#include<errno.h> //For errno - the error number
#include<netinet/udp.h>	//Provides declarations for udp header
#include<netinet/ip.h>	//Provides declarations for ip header
#include <arpa/inet.h> // inet_addr
#include <netinet/in.h>



#include <iostream>
#include <vector>
#include <string>
#include <sstream>



// struct ipheader {
//     //IP header format
//     unsigned char      iph_ihl; //IHL
//     unsigned char      iph_ver; //Version
//     unsigned char      iph_tos; //Type of service
//     unsigned short     iph_len; //Total length
//     unsigned short     iph_ident; //Identification
//     unsigned short     iph_offset; //Fragment offset
//     unsigned char      iph_ttl; //Time to live
//     unsigned char      iph_protocol; //Protocol
//     unsigned short     iph_csum; //Checksum
//     unsigned int       iph_source; //Source address
//     unsigned int       iph_dest; //Destination address
// };  

// struct udpheader {
//     unsigned short      udph_srcport; //Source port (2 bytes)
//     unsigned short      udph_destport; //Destination port (2 bytes)
//     unsigned short      udph_len; //UDP length (2 bytes)
//     unsigned short      udph_csum; //UDP checksum (2 bytes)
// };



int main (int argc, char* argv[]) 
{
	//Create a raw socket of type IPPROTO
	int s = socket (AF_INET, SOCK_RAW, IPPROTO_RAW);
	if(s == -1)
	{
		//socket creation failed, may be because of non-root privileges
		perror("Failed to create raw socket");
		exit(1);
	}
	
	//Datagram to represent the packet
	char datagram[4096] , source_ip[32] , *data;
	
	//zero out the packet buffer
	memset (datagram, 0, 4096);
	
	//IP header
	// struct ipheader *iph = (struct ipheader *) datagram;
	struct ip *iph = (struct ip *) datagram;
	
	//UDP header
	//struct udpheader *udph = (struct udpheader *) (datagram + sizeof (struct ip));
	struct udphdr *udph = (struct udphdr *) (datagram + sizeof (struct ip));
	
	struct sockaddr_in dest;
	struct sockaddr_in src;
	
	//Data part
	//data = datagram + sizeof(struct ipheader) + sizeof(struct udpheader);
	data = datagram + sizeof(struct ip) + sizeof(struct udphdr);
	strcpy(data , "$group_6$");


	// destionation
	dest.sin_family = AF_INET;
	dest.sin_port =  atoi(argv[1]); // htons(atoi(argv[1]));
	dest.sin_addr.s_addr = inet_addr ("130.208.243.61");
	

	// source
	src.sin_family = AF_INET;
	src.sin_port =  0; //htons(0);
	src.sin_addr.s_addr = inet_addr ("10.3.15.42");
	


	//some address resolution
	// strcpy(source_ip , "10.3.26.122");
	
	
	//Fill in the IP Header
	// iph->iph_ihl = 4;
	// iph->iph_ver = 4;
	// iph->iph_tos = 0;
	// iph->iph_len = sizeof (struct ipheader) + sizeof (struct udpheader) + strlen(data);
	// iph->iph_ident = htons (11);	//Id of this packet
	// iph->iph_offset = 0;
	// iph->iph_ttl = 255;
	// iph->iph_protocol = IPPROTO_UDP;
	// iph->iph_csum = 0;		//Set to 0 before calculating checksum
	// iph->iph_source = inet_addr ( source_ip );	//Spoof the source ip address
	// iph->iph_dest = sin.sin_addr.s_addr;


	iph->ip_hl = 5;
	iph->ip_v = 4;
	iph->ip_tos = 0;
	iph->ip_len = sizeof (struct ip) + sizeof (struct udphdr) + strlen(data);
	iph->ip_id =  11; // htons(11);
	iph->ip_off = 0;
	iph->ip_ttl = 255;
	iph->ip_p = IPPROTO_UDP;
	iph->ip_sum = 0;
	iph->ip_src = src.sin_addr;
	iph->ip_dst = dest.sin_addr;
	
	//Ip checksum


	//UDP header

	// udph->udph_srcport = htons(0);
	// udph->udph_destport = htons(atoi(argv[1]));
	// udph->udph_len  = htons(8 + strlen(data));	//tcp header size
	// udph->udph_csum = htons(0);	//leave checksum 0 now
	
	udph->uh_sport= 0;  // htons(0);
	udph->uh_dport= atoi(argv[1]);  // htons(atoi(argv[1]));
	udph->uh_ulen= 8 + strlen(data);   // htons(8 + strlen(data));	//tcp header size
	udph->uh_sum = 0;

	
    std::cout << datagram << std::endl;
    //std::cout << data << std::endl;
    //std::cout << iph << std::endl;
    //std::cout << &iph << std::endl;
    //std::cout << iph->iph_protocol << std::endl;
    //std::cout << iph->iph_ttl << std::endl;
    //std::cout << udph <<std::endl;
    //std::cout << &udph <<std::endl;


    // if (sendto (s, datagram, iph->iph_len ,	0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
    //if (sendto (s, datagram, strlen(datagram),	0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
    if (sendto (s, datagram, iph->ip_len ,	0, (struct sockaddr *) &dest, sizeof (dest)) < 0)
    {
        perror("sendto failed");
    }
    //Data send successfully
    else
    {
        printf ("Packet Send. Length : %d \n" , iph->ip_len);
    }
	return 0;
}