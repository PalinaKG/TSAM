#include<stdio.h>	//for printf
#include<string.h> //memset
#include<sys/socket.h>	//for socket ofcourse
#include<stdlib.h> //for exit(0);
#include<errno.h> //For errno - the error number
#include<netinet/udp.h>	//Provides declarations for udp header
#include<netinet/ip.h>	//Provides declarations for ip header
#include <arpa/inet.h> // inet_addr
#include <netinet/in.h>


#include <stdio.h>    
#include <stdlib.h>





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


struct pseudo_header
{
	u_int32_t source_address;
	u_int32_t dest_address;
	u_int8_t placeholder;
	u_int8_t protocol;
	u_int16_t udp_length;
};


typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;


// unsigned short csum(unsigned short number)
// {
//     int number1=number>>8;
//     number=number&0b11111111;
//     number1=number1&0b11111111;
//     short finalNumber=number+number1;
//     return ~finalNumber;
// }

// unsigned short csum(unsigned short *ptr,int nbytes) 
// {
// 	register long sum;
// 	unsigned short oddbyte;
// 	register short answer;

// 	sum=0;
// 	while(nbytes>1) {
// 		sum+=*ptr++;
// 		nbytes-=2;
// 	}
// 	if(nbytes==1) {
// 		oddbyte=0;
// 		*((u_char*)&oddbyte)=*(u_char*)ptr;
// 		sum+=oddbyte;
// 	}
// 	sum = (sum>>16)+(sum & 0xffff);
// 	sum = sum + (sum>>16);
// 	answer=(short)~sum;
	
// 	return(answer);
// }




// uint8_t csum(unsigned int addr)

// {

// 	register long sum = 0;

// 	for (int i=0; i<8; i++)
// 	{
//            /*  This is the inner loop */
//         sum += (addr & 0xFF);

// 		addr=addr>>8;
//     }

// 	std::cout << "sum: " << sum << std::endl;
// 	while ((sum&0xFF00)!=0)
// 	//for (int i=0;i<4;i++)
// 	{
// 		//std::cout << (sum&0xFFFF0000) << std::endl;
// 		//std::cout << ((sum&0xFFFF0000)!=0) << std::endl;
		
// 		int overFlow=(sum&0xFF00)>>8;
// 		sum=(sum&0x00FF)+overFlow;
// 		std::cout << "sum: " << sum << std::endl;
// 	}
//            /*  Add left-over byte, if any */
//     //    if( count > 0 )
//     //            sum += (unsigned char ) addr;

//     //        /*  Fold 32-bit sum to 16 bits */
//     //    while (sum>>16)
//     //        sum = (sum & 0xffff) + (sum >> 16);
// 	uint8_t checksum = (uint8_t) sum;
//       return checksum;
// }





u16 getCheckSum(void *buffer, int nbytes)
{
    u16 *ptr = (u16 *)(buffer);
    unsigned long sum;
    unsigned short oddbyte;
    unsigned short answer;

    sum = 0;
    while (nbytes > 1)
    {
        sum += *ptr++;
        nbytes -= 2;
		//decToBinary(sum);
		//std::cout << std::endl;
		//std::cout << "sum: " << sum << std::endl;
    }
    if (nbytes == 1)
    {
        oddbyte = 0;
        *((u_char *)&oddbyte) = *(u_char *)ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum = sum + (sum >> 16);
	std::cout << "sum-before: " << sum << std::endl;
	
    answer = (u16)~sum;

	std::cout << "sum-after: " << answer << std::endl;


    return answer + 80; //Adding 0101-0000 
}





// unsigned short csum(int number)
// {
// 	int finalNumber=~number;


// }



int main (int argc, char* argv[]) 
{


 int s_1 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
	
	if(s_1== -1)
	{
		//socket creation failed, may be because of non-root privileges
		perror("Failed to create socket");
		exit(1);
	}




	//Create a raw socket of type IPPROTO
	int s = socket (AF_INET, SOCK_RAW, IPPROTO_UDP);
	if(s == -1)
	{
		//socket creation failed, may be because of non-root privileges
		perror("Failed to create raw socket");
		exit(1);
	}

	int one = 1;
	const int *val = &one;

	if (setsockopt (s, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)							// header included
	{
	printf ("Error setting IP_HDRINCL. Error number : %d . Error message : %s \n" , errno , strerror(errno));
	exit(0);
	}
	
	//Datagram to represent the packet
	char datagram[4096] , source_ip[32] , *data, *pseudogram;;
	
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
	strcpy(data , "ABC");

	

	// destionation
	dest.sin_family = AF_INET;
	dest.sin_port =  htons(atoi(argv[1])); // htons(atoi(argv[1]));
	dest.sin_addr.s_addr = inet_addr ("130.208.243.61");
	

	// source
	src.sin_family = AF_INET;
	src.sin_port =  5000; //htons(0);
	src.sin_addr.s_addr = inet_addr ("192.168.1.209");
	


	if(bind(s, (struct sockaddr *)&src, sizeof(src)) < 0)
   	{
      perror("Failed to bind to socket:");
      return(-1);
   	}

	//some address resolution
	strcpy(source_ip , "192.168.1.209");
	
	
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
	
	udph->uh_sport= htons(5000);
	udph->uh_dport= htons(atoi(argv[1]));
	udph->uh_ulen= htons(8 + strlen(data));	//tcp header size
	//udph->uh_ulen=4000;
	

	
    std::cout << datagram << std::endl;
    //std::cout << data << std::endl;
    //std::cout << iph << std::endl;
    //std::cout << &iph << std::endl;
    //std::cout << iph->iph_protocol << std::endl;
    //std::cout << iph->iph_ttl << std::endl;
    //std::cout << udph <<std::endl;
    //std::cout << &udph <<std::endl;


	char buffer[512];
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


    // if (sendto (s, datagram, iph->iph_len ,	0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
    //if (sendto (s, datagram, strlen(datagram),	0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
   if (sendto (s_1, datasending, strlen(datasending),	0, (struct sockaddr *) &dest, sizeof (dest)) < 0)
    {
        perror("sendto failed");
    }
    //Data send successfully
    else
    {
        printf ("Packet Send. Length : %d \n" , iph->ip_len);
    }

	ssize_t count=recvmsg(s_1,&message,0);
	if (count==-1) {
        printf("%s",strerror(errno));
    } else if (message.msg_flags&MSG_TRUNC) {
        printf("datagram too large for buffer: truncated");
    } else {
        // handle_datagram(buffer,count);
        printf("%s",buffer);
        // printf("%d",count);
    }


	std::vector<std::string> tokens;   // List of tokens in command from client
  	std::string token;                  // individual token being parsed
  // Split command from client into tokens for parsing
  	std::stringstream stream(buffer);

  // By storing them as a vector - tokens[0] is first word in string


  while(stream >> token)
      tokens.push_back(token);
    
	std::cout << std::endl << std::endl;
    std::cout << tokens[17] << std::endl;

	
	//udph->uh_sum = csum(stoi(tokens[17]));

	

	 //Now the UDP checksum using the pseudo header
	//int tok=tokens[17];
	std::cout << tokens[17].substr(2,5) << std::endl;
	int num = (int)stoi(tokens[17].substr(2,5), NULL, 16); 
	std::cout<<num <<std::endl;

	//strcpy(headerData, "12");
	//int headerData=1;
	// struct pseudo_header psh;
	// psh.source_address = inet_addr( source_ip );
	// psh.dest_address = dest.sin_addr.s_addr;
	// psh.protocol = IPPROTO_UDP;
	// psh.placeholder = 0;
	// psh.udp_length = htons(sizeof(struct udphdr) + strlen(data));
	
	// int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(data);
	// pseudogram = (char *)malloc(psize);
	
	// memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
	// memcpy(pseudogram + sizeof(struct pseudo_header) , udph , sizeof(struct udphdr) + strlen(data));
	

	struct pseudo_header psh;
	psh.source_address = inet_addr( source_ip );
	psh.dest_address = dest.sin_addr.s_addr;
	psh.protocol = IPPROTO_UDP;
	psh.udp_length = htons(sizeof(struct udphdr) + strlen(data));
	
	int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(data);
	pseudogram = (char *)malloc(psize);
	
	memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
	memcpy(pseudogram + sizeof(struct pseudo_header) , udph , sizeof(struct udphdr) + strlen(data));
	
	

	
	
	// udph->uh_sum=csum((unsigned short*) pseudogram , psize);
	// std::cout << udph->uh_sum << std::endl;
	// int diff=num-(udph->uh_sum);
	// std::cout << "DIF: " << diff << std::endl;
	// data=std::to_string(diff);
	// //strcpy(data,  itoa(diff));



	// iph->ip_len = sizeof (struct ip) + sizeof (struct udphdr) + strlen(data);
	// udph->uh_ulen= htons(8 + strlen(data));	//tcp header size

	// psh.udp_length = htons(sizeof(struct udphdr) + strlen(data));
	
	// int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(data);
	// pseudogram = (char *)malloc(psize);
	
	// memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
	// memcpy(pseudogram + sizeof(struct pseudo_header) , udph , sizeof(struct udphdr) + strlen(data));
	
	
	// psh.placeholder = diff;
	// psh.udp_length = htons(sizeof(struct udphdr) + strlen(data));
	// //psh.udp_length = htons(sizeof(struct udphdr) + strlen(data) );
	// psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(data);
	// pseudogram = (char *)malloc(psize);
	

	// memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
	// memcpy(pseudogram + sizeof(struct pseudo_header) , udph , sizeof(struct udphdr) + strlen(data));


	std::cout << "Pseudogram: " << &pseudogram << std::endl;
	//unsigned short gram = (unsigned short) &pseudogram;
	//udph->uh_sum= csum(pseudogram, psize);
	udph->uh_sum = getCheckSum(pseudogram, psize);
	//dph->uh_sum=0;
	std::cout << udph->uh_sum << std::endl;
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