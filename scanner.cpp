#include <stdio.h>			//for printf
#include <string.h> 		//memset
#include <sys/socket.h>		//for socket ofcourse
#include <stdlib.h> 		//for exit(0);
#include <errno.h> 			//For errno - the error number
#include <netinet/udp.h>	//Provides declarations for udp header
#include <netinet/ip.h>		//Provides declarations for ip header
#include <arpa/inet.h> 		//inet_addr

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>


void arr_resize(int* & arr ){
    // double size of arr
    int* resize = new int[sizeof(arr)*2];

    for (unsigned int i = 0; i<sizeof(arr); i++){
        resize[i] = arr[i];
    }

    delete[] arr;  // delete old array
    arr = resize;  
}

bool not_in_arr(int* arr, int n_foundports ,int port_poss){
    // returns false if port_poss is in arr
    for (int i = 0; i<n_foundports; i++){
        if (arr[i] == port_poss){
            return false;
        }
    }
    return true;
}


void print_buffer(char* buf, int start, int finish){
	// prints out the message in the buffer
	for (int i = start; i<finish; i++){
		char c = buf[i];	
		std::cout<<c;	
	}
	printf("\n");
}




int main(int argc, char* argv[]){

	if (argc != 4){
		printf("Usage: scanner <ip address> <low port> <high port> \n");
        exit(0);
	}

    printf("Starting scan on IP:%s from port %s to %s  (ETA:~%ds)\n\n", argv[1],argv[2],argv[3],(int)((atoi(argv[3])-atoi(argv[2]))*0.7));



	//Create a raw UDP socket
	int s_raw = socket (AF_INET, SOCK_RAW, IPPROTO_UDP);
	if(s_raw == -1)
	{
		perror("Failed to create raw socket -- (Remember to run with root privileges)");
		exit(1);
	}

	int one = 1;
	const int *val = &one;

	if (setsockopt (s_raw, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)	    // header included
	{
		printf ("Error setting IP_HDRINCL. Error number : %d . Error message : %s \n" , errno , strerror(errno));
		exit(0);
	}
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 100000;	  // 0.01s timeout 
	if (setsockopt (s_raw, SOL_SOCKET, SO_RCVTIMEO ,&tv,sizeof(tv)) < 0)   // set timelimit on each socket
	{
		perror("Error socket setup options");
		exit(0);
	}

	
	
	//Datagram to represent the packet
	char datagram[4096] , *data;
	
	//zero out the packet buffer
	memset (datagram, 0, 4096);
	
	//IP header
	struct ip *iph = (struct ip *) datagram;
	
	//UDP header
	struct udphdr *udph = (struct udphdr *) (datagram + sizeof (struct ip));
	
	//Socket addresses structures
	struct sockaddr_in dest;
	struct sockaddr_in src;
	
	//Data part
	data = datagram + sizeof(struct ip) + sizeof(struct udphdr);
    strcpy(data , "$group_6$");                 // payload message


	// destionation
	dest.sin_family = AF_INET;
	dest.sin_port =  htons(4000); 
	dest.sin_addr.s_addr = inet_addr (argv[1]);  
	
	// source
	int const src_port = 5000;
	src.sin_family = AF_INET;
	src.sin_port =  src_port; 

	std::ifstream file("IPv4address.txt");	// get host IPv4 address from text file created with make file
    std::string str; 
    std::getline(file, str);
    
	src.sin_addr.s_addr = inet_addr (str.c_str());


	if(bind(s_raw, (struct sockaddr *)&src, sizeof(src)) < 0)   // bind raw socket with source port
   	{
      perror("Failed to bind to socket:");
      exit(0);
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

    

	char buffer[4096];


	int n_reruns = 7;					// number of reruns
	int port_lo = atoi(argv[2]);		// lowest number of port		
	int port_hi = atoi(argv[3]);		// highest number of port

	int size = 8;
	int* port_numbers = new int[size];  // initialize array containing open ports numbers
    int n_ports = 0;                    // number of current found open ports


	for (int i = 0; i<n_reruns; i++){
        // printf("run: %d\n",i);
        for(int port = port_lo; port <= port_hi; port++){

            dest.sin_port = htons(port); // update port number
			udph->uh_dport= htons(port);

			if (sendto (s_raw, datagram, iph->ip_len , 0, (struct sockaddr *) &dest, sizeof (dest)) < 0)
			{
				perror("sendto failed");
			}
		

			socklen_t socklen = sizeof(dest);
			int c = recvfrom(s_raw, buffer, sizeof(buffer), 0, (struct sockaddr *) &dest, &socklen);

			if (c<0){
				// socket timed out or received ICMP package
			} 
			else
			{
				int p_numb = ((buffer[20] & 0xFF)<<8);     // source port of this package is stored in byte 20 and 21
				p_numb += (buffer[21] & 0xFF);


				if ((port_lo <= p_numb) & (p_numb <= port_hi)){
					if(not_in_arr(port_numbers, n_ports, p_numb)){    // if p_numb is not already in port_numbers

						if (n_ports == size){                         // resize array if to big
							arr_resize(*& port_numbers);
							size *= 2; 
						}

						port_numbers[n_ports] = p_numb;               // add port to port_numbers
						n_ports++;

						printf("\n\nport %d\n",p_numb);
						printf("message : ");
						print_buffer((char*) buffer, sizeof(struct ip) + sizeof(struct udphdr), c);         

					       
               		}
				}	
			}
        }   
    }

    delete [] port_numbers; // free memory 
    port_numbers = NULL; 


    printf("Scan finished!\n");

        
    return 0;
    

}




















