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
/* 
	96 bit (12 bytes) pseudo header needed for udp header checksum calculation 
*/


struct ipheader {
    //IP header format
    unsigned char      iph_ihl; //IHL
    unsigned char      iph_ver; //Version
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



struct pseudo_header
{
	u_int32_t source_address;
	u_int32_t dest_address;
	u_int8_t placeholder;
	u_int8_t protocol;
	u_int16_t udp_length;
};

/*
	Generic checksum calculation function
*/
unsigned short csum_IP(unsigned short *ptr,int nbytes) 
{
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

unsigned short csum(int number)
{
    int number1=number>>8;
    number=number&0b11111111;
    number1=number1&0b11111111;
    short finalNumber=number+number1;
    return ~finalNumber;
}

int main (int argc, char* argv[]) 
{

    // int s_1 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
	
	// if(s_1== -1)
	// {
	// 	//socket creation failed, may be because of non-root privileges
	// 	perror("Failed to create socket");
	// 	exit(1);
	// }







	//Create a raw socket of type IPPROTO
	
	int s = socket (AF_INET, SOCK_RAW, IPPROTO_RAW);
	if(s == -1)
	{
		//socket creation failed, may be because of non-root privileges
		perror("Failed to create raw socket");
		exit(1);
	}
	
	//Datagram to represent the packet
	char datagram[4096] , source_ip[32] , *data , *pseudogram;
	
	//zero out the packet buffer
	memset (datagram, 0, 4096);
	
	//IP header
	struct ipheader *iph = (struct ipheader *) datagram;
	
	//UDP header
	struct udpheader *udph = (struct udpheader *) (datagram + sizeof (struct ip));
	
	struct sockaddr_in sin;
	struct pseudo_header psh;
	
	//Data part
    //strcpy(datagram , "$group_6$");
	data = datagram + sizeof(struct ipheader) + sizeof(struct udpheader);
	strcpy(data , "$group_6$");


    //IP header
	// struct ipheader *iph = (struct ipheader *) datagram;
	
	// //UDP header
	// struct udpheader *udph = (struct udpheader *) (datagram + sizeof (struct ip));



	
	//some address resolution
	strcpy(source_ip , "10.3.26.122");
	
	sin.sin_family = AF_INET;
	sin.sin_port = htons(atoi(argv[1]));
	sin.sin_addr.s_addr = inet_addr ("130.208.243.61");
	
	//Fill in the IP Header
	iph->iph_ihl = 4;
	iph->iph_ver = 4;
	iph->iph_tos = 0;
	iph->iph_len = sizeof (struct ipheader) + sizeof (struct udpheader) + strlen(data);
	iph->iph_ident = htons (11);	//Id of this packet
	iph->iph_offset = 0;
	iph->iph_ttl = 255;
	iph->iph_protocol = 17;
	iph->iph_csum = 0;		//Set to 0 before calculating checksum
	iph->iph_source = inet_addr ( source_ip );	//Spoof the source ip address
	iph->iph_dest = sin.sin_addr.s_addr;
	
	//Ip checksum
	iph->iph_csum = csum_IP ((unsigned short *) datagram, iph->iph_len);
	
	//UDP header
	udph->udph_srcport = htons(0);
	udph->udph_destport = htons(atoi(argv[1]));
	udph->udph_len  = htons(8 + strlen(data));	//tcp header size
	udph->udph_csum = 0;	//leave checksum 0 now, filled later by pseudo header
	
	
	
	//loop if you want to flood :)
	//while (1)
    // int one = 1;
    // const int *val = &one;
    // if(setsockopt(s, IPPROTO_IP, IP_HDRINCL, val, sizeof(one))<0 )
    // {
	//     printf("Error - Socket options\n");	
	//     exit(-1);
    // }
    char buffer[300];
    char datasending[10];
    strcpy(datasending, "$group_6$");
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


    // int s_1 = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    // if (sendto (s_1, datasending, udph->udph_len , 0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
    // {
	// 		perror("sendto failed");
	// 	}
	// 	//Data send successfully
	// 	else
	// 	{
	// 		printf ("Packet Send. Length : %d \n" , iph->iph_len);
	// 	}
    // std::cout << "HAA" << std::endl;
    // ssize_t count=recvmsg(s_1,&message,0);
    // std::cout << "NIIII" << std::endl;

    
       
    // if (count==-1) {
    //     printf("%s",strerror(errno));
    // } else if (message.msg_flags&MSG_TRUNC) {
    //     printf("datagram too large for buffer: truncated");
    // } else {
    //     // handle_datagram(buffer,count);
    //     printf("%s",buffer);
    //     // printf("%d",count);
    // }



    std::vector<std::string> tokens;   // List of tokens in command from client
  std::string token;                  // individual token being parsed

  // Split command from client into tokens for parsing


  std::stringstream stream(buffer);

  // By storing them as a vector - tokens[0] is first word in string


  while(stream >> token)
      tokens.push_back(token);
    

    // std::cout << tokens[17] << std::endl;
    // std::string num=tokens[17];
    // int number= stoi(num);
   //udph->udph_csum = csum(number);
    udph->udph_csum=0;

    // //Now the UDP checksum using the pseudo header
	// psh.source_address = inet_addr( source_ip );
	// psh.dest_address = sin.sin_addr.s_addr;
	// psh.placeholder = 0;
	// psh.protocol = IPPROTO_UDP;
	// psh.udp_length = htons(sizeof(struct udphdr) + strlen(data) );
	
	// int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(data);
	// pseudogram = (char *)malloc(psize);
	
	// memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
	// memcpy(pseudogram + sizeof(struct pseudo_header) , udph , sizeof(struct udphdr) + strlen(data));
	
	// udph->udph_csum= csum( (unsigned short*) pseudogram , psize);


		//Send the packet
       

        
       
        std::cout << datagram << std::endl;
        std::cout << data << std::endl;
        std::cout << iph << std::endl;
        std::cout << &iph << std::endl;
        std::cout << iph->iph_protocol << std::endl;
        std::cout << iph->iph_ttl << std::endl;
        std::cout << udph <<std::endl;
        std::cout << &udph <<std::endl;
		if (sendto (s, (struct ipheader *)iph, iph->iph_len ,	0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
		//if (sendto (s, datagram, strlen(datagram),	0, (struct sockaddr *) &sin, sizeof (sin)) < 0)

        {

			perror("sendto failed");
		}
		//Data send successfully
		else
		{
			printf ("Packet Send. Length : %d \n" , iph->iph_len);
		}

    
    // char buffer[548];
    // struct sockaddr_storage src_addr;

    // struct iovec iov[1];
    // iov[0].iov_base=buffer;
    // iov[0].iov_len=sizeof(buffer);

    // struct msghdr message;
    // message.msg_name=&src_addr;
    // message.msg_namelen=sizeof(src_addr);
    // message.msg_iov=iov;
    // message.msg_iovlen=1;
    // message.msg_control=0;
    // message.msg_controllen=0;

    // socklen_t len = sizeof sin;

    // if(recvfrom(s, buffer,sizeof(buffer), 0, (struct sockaddr *) &sin, &len) < 0){
    //  //timeout reached
    //  perror("Received from failed");
    // }
    // else{
    //  printf("%s",buffer);
    // }




    // if (sendto (s_1, data, udph->udph_len , 0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
	// 	{
	// 		perror("sendto failed");
	// 	}
	// 	//Data send successfully
	// 	else
	// 	{
	// 		printf ("Packet Sent to port %d \n"); 
	// 	}

	// 	ssize_t count=recvmsg(s_1,&message,0);
		
	// 	if (count==-1) {
	// 		printf("%s",strerror(errno));
	// 	} else if (message.msg_flags&MSG_TRUNC) {
	// 		printf("datagram too large for buffer: truncated");
	// 	} else {
	// 		printf("%s",buffer);
	
	// 	}




	
	return 0;
}