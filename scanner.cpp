#include <sys/socket.h>
// #include<stdio.h>	//for printf
//#include<string.h> //memset
// #include<stdlib.h> //for exit(0);
// #include<errno.h> //For errno - the error number
#include<netinet/udp.h>	//Provides declarations for udp header
#include<netinet/ip.h>	//Provides declarations for ip header

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
// http://www.cis.syr.edu/~wedu/seed/Labs_12.04/Networking/DNS_Remote/udp.c 


// my adding imports 
#include <sys/types.h>
#include <sys/time.h>



struct ipheader {
    //IP header format
    unsigned char      iph_ihl:4; //IHL
    unsigned char      iph_ver:4; //Version
    unsigned char      iph_tos; //Type of service
    unsigned short     iph_len; //Total length
    unsigned short     iph_ident; //Identification
    unsigned short     iph_offset; //Fragment offset
    unsigned char      iph_ttl; //Time to live
    unsigned char      iph_protocol; //Protocol
    unsigned short     iph_csum; //Checksum
    unsigned int       iph_source; //Source address
    unsigned int       iph_dest; //Destination address
};  

struct udpheader {
    unsigned short      udph_srcport; //Source port (2 bytes)
    unsigned short      udph_destport; //Destination port (2 bytes)
    unsigned short      udph_len; //UDP length (2 bytes)
    unsigned short      udph_csum; //UDP checksum (2 bytes)
};

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



////// main ////////
int main (int argc, char* argv[]) {

	if (argc != 4){
		printf("Usage: scanner <ip address> <low port> <high port> \n");
        exit(0);
	}
    
	//Datagram to represent the packet
	char datagram[512] , source_ip[32] , *data , *pseudogram;
	
	//zero out the packet buffer
	memset (datagram, 0, 512);
	
    //UDP header
	struct udpheader *udph = (struct udpheader *) (datagram + sizeof (struct ipheader));
	struct sockaddr_in sin;
	struct pseudo_header psh;

    //Data part
	data = datagram + sizeof(struct ipheader) + sizeof(struct udpheader);
	strcpy(data , "Are you open?");
	
	//some address resolution
	strcpy(source_ip , "192.168.1.12");
	sin.sin_family = AF_INET;
	sin.sin_port = htons(4000);
	// sin.sin_addr.s_addr = inet_addr ("130.208.243.61");
	sin.sin_addr.s_addr = inet_addr (argv[1]);


	// Creating ip header
	struct ipheader *iph = (struct ipheader *) datagram;
     
    iph->iph_tos = 0; //Type of service
    iph->iph_len = sizeof (struct ipheader) + sizeof (struct udpheader) + strlen(data);  //Total length
    iph->iph_ident = htonl(11); //Identification
    iph->iph_offset = 0; //Fragment offset
    iph->iph_ttl = 255; //Time to live
    iph->iph_protocol = IPPROTO_UDP; //Protocol
    
    iph->iph_source = inet_addr (source_ip); //Source address
    // iph->iph_dest = inet_addr ("130.208.243.61"); //Destination address
	iph->iph_dest = inet_addr (argv[1]); //Destination address


   //Ip checksum
	iph->iph_csum = csum ((unsigned short *) datagram, iph->iph_len);

    // UDP header
    int size_udph = strlen(datagram) + sizeof (struct ipheader);

    udph->udph_srcport = htons(0);
    udph->udph_destport = htons(4000);
    udph->udph_len = size_udph + strlen(data);
    udph->udph_csum = 0;

    
    //Now the UDP checksum
	psh.source_address = inet_addr( source_ip );
	psh.dest_address = sin.sin_addr.s_addr;
	psh.placeholder = 0;
	psh.protocol = IPPROTO_UDP;
	psh.tcp_length = htons(size_udph + strlen(data) );
	
	int psize = sizeof(struct pseudo_header) + size_udph + strlen(data);
	pseudogram = (char *) malloc(psize);
	
	memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
	memcpy(pseudogram + sizeof(struct pseudo_header) , udph , size_udph + strlen(data));
	
	udph->udph_csum = csum( (unsigned short*) pseudogram , psize);
	

    //Create a raw UDP socket
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
	
	if(s== -1)
	{
		//socket creation failed, may be because of non-root privileges
		perror("Failed to create socket");
		exit(1);
	}

	
	// http://www.microhowto.info/howto/listen_for_and_receive_udp_datagrams_in_c.html
	char buffer[512];
	struct sockaddr_storage src_addr;

	struct iovec iov[1];
	iov[0].iov_base=buffer;
	iov[0].iov_len=sizeof(buffer);

	struct msghdr message;
	message.msg_name=&src_addr;
	message.msg_namelen=sizeof(src_addr);
	message.msg_iov=iov;
	message.msg_iovlen=1;
	message.msg_control=0;
	message.msg_controllen=0;


    // initializing timeout for socket
	// https://stackoverflow.com/questions/13547721/udp-socket-set-timeout
	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;	
	if (setsockopt (s, SOL_SOCKET, SO_RCVTIMEO ,&tv,sizeof(tv)) < 0)
	{
		perror("Error socket setup options");
		exit(0);
	}

	int i = atoi(argv[2]);
	int i_stop = atoi(argv[3])+1;
	while (i<i_stop){
		sin.sin_port = htons(i);

		//Send the packet
		if (sendto (s, data, udph->udph_len , 0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
		{
			perror("sendto failed");
		}
		//Data send successfully
		else
		{
			printf ("Packet Sent to port %d \n" , i); 
		}

		ssize_t count=recvmsg(s,&message,0);
		
		if (count==-1) {
			printf("%s",strerror(errno));
		} else if (message.msg_flags&MSG_TRUNC) {
			printf("datagram too large for buffer: truncated");
		} else {
			printf("%s",buffer);
	
		}

		printf("\n\n");

		i++; 

	} 


	return 0;
}