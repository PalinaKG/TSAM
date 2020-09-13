//#include <sys/socket.h>
// #include<stdio.h>	//for printf
//#include<string.h> //memset
// #include<stdlib.h> //for exit(0);
// #include<errno.h> //For errno - the error number
#include<netinet/udp.h>	//Provides declarations for udp header
//#include<netinet/ip.h>	//Provides declarations for ip header

#include <stdio.h>	//for printf
#include <string.h> //memset
#include <sys/socket.h>	//for socket ofcourse
#include <stdlib.h> //for exit(0);
#include <errno.h> //For errno - the error number
// #include <netinet/tcp.h>	//Provides declarations for tcp header
#include <netinet/ip.h>	//Provides declarations for ip header
#include <arpa/inet.h> // inet_addr
#include <unistd.h> // sleep()
#include <typeinfo>
//https://www.binarytides.com/raw-sockets-c-code-linux/
http://www.cis.syr.edu/~wedu/seed/Labs_12.04/Networking/DNS_Remote/udp.c 


// struct ipheader {
//     //IP header format
//     unsigned char      iph_ihl:4; //IHL
//     unsigned char      iph_ver:4; //Version
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

unsigned short csum(unsigned short *ptr, int nbytes) {
    register long sum;
	unsigned short oddbyte;
	register short answer;

	sum=0;
	while(nbytes>1) {
		sum+=*ptr++;
		nbytes-=2;
	}
	if(nbytes==1) {
		oddbyte=0;
		*((u_char*)&oddbyte)=*(u_char*)ptr;
		sum+=oddbyte;
	}

	sum = (sum>>16)+(sum & 0xffff);
	sum = sum + (sum>>16);
	answer=(short)~sum;
	
	return(answer);
}

struct pseudo_header
{
	u_int32_t source_address;
	u_int32_t dest_address;
	u_int8_t placeholder;
	u_int8_t protocol;
	u_int16_t tcp_length;
};

int main () {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //raw UDP socket created using the socket function
    //std::string ip="127.0.0.1";
   //Create a raw socket
	int s = socket (PF_INET, SOCK_RAW, IPPROTO_TCP);
	
	if(s == -1)
	{
		//socket creation failed, may be because of non-root privileges
		perror("Failed to create socket");
		exit(1);
	}
	
	//Datagram to represent the packet
	char datagram[512] , source_ip[32] , *data , *pseudogram;
	
	//zero out the packet buffer
	memset (datagram, 0, 512);
	
	//IP header
	struct ip *iph = (struct ip *) datagram;

    //TCP header
	struct udphdr *udph = (struct udphdr *) (datagram + sizeof (struct ip));
	struct sockaddr_in sin;
	struct pseudo_header psh;

    //Data part
	data = datagram + sizeof(struct ip) + sizeof(struct udphdr);
	strcpy(data , "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	
	//some address resolution
	strcpy(source_ip , "192.168.1.1");
	sin.sin_family = AF_INET;
	sin.sin_port = htons(80);
	sin.sin_addr.s_addr = inet_addr ("130.208.243.61");
 

	struct in_addr * src_add = (struct in_addr *) inet_addr (source_ip);
	iph->ip_src=src_add;
    iph->ip_tos = 0; //Type of service
    iph->ip_len = sizeof (struct ip) + sizeof (struct udphdr) + strlen(data); //Total length
    iph->ip_id = htonl(11); //Identification er 16 bitar þannig kannski nota htnos??
    iph->ip_off = 0; //Fragment offset
    iph->ip_ttl = 255; //Maximum number (perhaps to large?), often recommended to use minimum of 64
    iph->ip_p = IPPROTO_UDP; //Protocol
	//typeid(iph->ip_src).name();
	//typeid(iph->ip_dst)=typeid(inet_addr ("130.208.243.61"));
	//ip_dst=inet_addr ("130.208.243.61");
  //iph->ip_src = inet_addr (source_ip); //Checksum
    //iph->ip_dst = inet_addr ("130.208.243.61");  //Destination address


   //Ip checksum
	iph->check = sum ((unsigned short *) datagram, iph->iph_len);

    udph->udph_srcport = htons(0);
    udph->udph_destport = htons("4007");
    udph->udph_len = sizeof (struct udphr) + strlen(data);
    udph->udph_csum = 0;

    //Now the TCP checksum
	psh.source_address = inet_addr( source_ip );
	psh.dest_address = sin.sin_addr.s_addr;
	psh.placeholder = 0;
	psh.protocol = IPPROTO_UDP;
	psh.tcp_length = htons(sizeof(struct udphdr) + strlen(data) );
	
	int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(data);
	pseudogram = malloc(psize);
	
	memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
	memcpy(pseudogram + sizeof(struct pseudo_header) , tcph , sizeof(struct udphdr) + strlen(data));
	
	udp->udph_csum = csum( (unsigned short*) pseudogram , psize);
	
    //IP_HDRINCL to tell the kernel that headers are included in the packet kannski óþarfi??
	int one = 1;
	const int *val = &one;
	
	if (setsockopt (s, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)
	{
		perror("Error setting IP_HDRINCL");
		exit(0);
	}

    while (1)
	{
		//Send the packet
		if (sendto (sock, datagram, udph->udph_len ,	0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
		{
			perror("sendto failed");
		}
		//Data send successfully
		else
		{
			printf ("Packet Send. Length : %d \n" , udph->udph_len);
		}
        // sleep for 1 seconds
        sleep(1);
	}
	
	return 0;