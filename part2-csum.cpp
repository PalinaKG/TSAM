#include <stdio.h>	//for printf
#include <string.h> //memset
#include <sys/socket.h>	//for socket ofcourse
#include <stdlib.h> //for exit(0);
#include <errno.h> //For errno - the error number
#include <netinet/udp.h>	//Provides declarations for udp header
#include <netinet/ip.h>	//Provides declarations for ip header
#include <arpa/inet.h> // inet_addr
#include <netinet/in.h>


#include <unistd.h>   // gethostbyname
#include <netdb.h> 
#include <sys/types.h> 


#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>




void print_buffer(char* buf, int start, int finish){
	// prints out the message in buffer
	for (int i = start; i<finish; i++){
		char c = buf[i];	
		std::cout<<c;	
	}
	printf("\n");
}



int return_hex_in_buffer(char* buf, int start, int finish){
	// finds and returns the hex number in buffer
	int hex_num = 0;
	char x;
	for(int i = start; i<finish; i++){

		x = buf[i];
		if (x == 'x'){
			
		    char h []= {'0','x',buf[i+1], buf[i+2], buf[i+3], buf[i+4]};	

			hex_num = (int)strtol(h, NULL, 0);

			return (hex_num);
		}
	}
	
}


unsigned short checkSum(void *buffer, int nbytes)
{
    unsigned short *ptr = (unsigned short *)(buffer);
    unsigned long sum;
    unsigned short oddbyte;
    unsigned short answer;

    sum = 0;
    while (nbytes > 1)
    {
        sum += *ptr++;
        nbytes -= 2;
    }
    if (nbytes == 1)
    {
        oddbyte = 0;
        *((unsigned char *)&oddbyte) = *(unsigned char *)ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum = sum + (sum >> 16);
	
    answer = (unsigned short)~sum;

    return answer; // have to add 80 (0101 0000) on macOS but not for Linux
}


struct pseudo_header   
// declare pseudo header for the check sum
{
	u_int32_t source_address;
	u_int32_t dest_address;
	u_int8_t placeholder;
	u_int8_t protocol;
	u_int16_t udp_length;
};




int main(int argc, char* argv[]){

	if (argc != 2){
		printf("Usage: p2csum <port> \n");
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

	if (setsockopt (s_raw, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)	     	// header included
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
	
	// address structures
	struct sockaddr_in dest;
	struct sockaddr_in src;
	
	//Data part
	data = datagram + sizeof(struct ip) + sizeof(struct udphdr);
	strcpy(data , "$group_6$");


	// destionation address
	dest.sin_family = AF_INET;
	dest.sin_port =  htons(atoi(argv[1])); 
	dest.sin_addr.s_addr = inet_addr ("130.208.243.61");  // IP for skel.ru.is

	// source
	int const src_port = 5000;   // hard coded open port 
	src.sin_family = AF_INET;
	src.sin_port =  src_port; 

	std::ifstream file("IPv4address.txt");  // get host IPv4 address from text file created with make file
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
	udph->uh_dport= htons(4000);                                                    // Destination port
	udph->uh_ulen= htons(8 + strlen(data));	                                        // Total length of header
	udph->uh_sum = 0;                                                               // checksum



	// fill in pseudo_header
	struct pseudo_header psh;
	psh.source_address = src.sin_addr.s_addr;
	psh.dest_address = dest.sin_addr.s_addr;
	psh.protocol = IPPROTO_UDP;
	psh.udp_length = htons(sizeof(struct udphdr) + strlen(data));
	
	int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(data);
	char *pseudogram = (char *)malloc(psize);
	
	memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
	memcpy(pseudogram + sizeof(struct pseudo_header) , udph , sizeof(struct udphdr) + strlen(data));


	udph->uh_sum = checkSum(pseudogram, psize);  // update check sum

	


	printf("Sending group number to port %s  (if it takes longer than 2sek, kill program (Ctrl+C) and retry)\n",argv[1]);

	// sending first message to get desired check sum
    if (sendto (s_raw, datagram, iph->ip_len , 0, (struct sockaddr *) &dest, sizeof (dest)) < 0)
    {
        perror("sendto failed");
		exit(0);
    }


	// receive 
    char buffer[4096];
	socklen_t socklen = sizeof(dest);
    int c = recvfrom(s_raw, buffer, sizeof(buffer), 0, (struct sockaddr *) &dest, &socklen);

    if (c<0){
        // socket timed out or received ICMP package   
		perror("recvfrom failed");
		exit(0);
    } 
    else
    {
        int index = sizeof(struct ip) + sizeof(struct udphdr);
        
        print_buffer((char*) buffer, index, c);

		unsigned int hex_numb = return_hex_in_buffer((char *)buffer, sizeof(struct ip), c);  // find hex number in buffer
		unsigned int hex_tmp1 = (hex_numb << 8) & 0xFF00;
		unsigned int hex_tmp2 = (hex_numb >> 8) & 0x00FF; 
		hex_numb = hex_tmp1 | hex_tmp2;   // hex in little endian

		std::string str = " ";
		unsigned char stafur = 0;
		int index_str = 0;
		
		// Creates every possible payload to match the desired check sum
		while (checkSum(pseudogram, psize)!= hex_numb)
		{

			data = datagram + sizeof(struct ip) + sizeof(struct udphdr); 			
			
			if (stafur == 255){				// If ascii char is max value 
				str.push_back(stafur);		// Pushes it to the back of the payload
				stafur = 0;					// Resets the char
				index_str++; 			    // Goes to the next index
			}
			else{							// Otherwise increment last letter in the payload 					
				str[index_str] = stafur;	
				stafur++; 
			}		
			
			strcpy(data ,  str.c_str());



			// Reset UDP header, IP header length and Pseudo header for this payload
			struct udphdr *udph = (struct udphdr *) (datagram + sizeof (struct ip));

			udph->uh_sport= htons(src_port);
			udph->uh_dport= htons(atoi(argv[1]));
			udph->uh_ulen= htons(8 + strlen(data));	
			udph->uh_sum = 0;
			

			iph->ip_len = sizeof (struct ip) + sizeof (struct udphdr) + strlen(data);


			// struct pseudo_header psh;
			psh.source_address = src.sin_addr.s_addr;
			psh.dest_address = dest.sin_addr.s_addr;
			psh.protocol = IPPROTO_UDP;
			psh.udp_length = htons(sizeof(struct udphdr) + strlen(data));
		
			psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(data);
			pseudogram = (char *)malloc(psize);
		
			memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
			memcpy(pseudogram + sizeof(struct pseudo_header) , udph , sizeof(struct udphdr) + strlen(data));


			if (index_str >= 1000000){   // if bigger than size of payload then try again
				perror("Unsuccessful -- try again\n");
				exit(0);
			}

		}
		
    }
	

	udph->uh_sum = checkSum(pseudogram, psize);  // update the checksum


	printf("sending:  %s\n", datagram );         


    // Sending datagram
    if (sendto (s_raw, datagram, iph->ip_len , 0, (struct sockaddr *) &dest, sizeof (dest)) < 0)
    {
        perror("sendto failed");
		exit(0);
    }

    c = recvfrom(s_raw, buffer, sizeof(buffer), 0, (struct sockaddr *) &dest, &socklen);



    if (c<0){
        // socket timed out or received ICMP package   
		perror("recvfrom failed");
		exit(0);
    } 
    else
    {
        int index = sizeof(struct ip) + sizeof(struct udphdr);
        
        print_buffer((char*) buffer, index, c);
        

    }
        
                


    

	return 0;
    

}




















