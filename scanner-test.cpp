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

void print_arr(int * arr, int num_ports){
    // prints out arr
    for (int i = 0; i < num_ports; i++){
        std::cout<<arr[i]<<" ";
    }
    std::cout<<"\n";
}


void print_buffer(char* buf, int start, int finish){
	for (int i = start; i<finish; i++){
		char c = buf[i];	
		std::cout<<c;	
	}
	printf("\n");
}




int main(int argc, char* argv[]){

	//Create a raw socket of type IPPROTO
	int s_raw = socket (AF_INET, SOCK_RAW, IPPROTO_UDP);
	if(s_raw == -1)
	{
		//socket creation failed, may be because of non-root privileges
		perror("Failed to create raw socket");
		exit(1);
	}

	int one = 1;
	const int *val = &one;

	if (setsockopt (s_raw, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)							// header included
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
	dest.sin_port =  htons(4000); // atoi(argv[1]);
	dest.sin_addr.s_addr = inet_addr ("130.208.243.61");  // IP for skel.ru.is
	

	int const src_port = 5000;

	// source
	src.sin_family = AF_INET;
	src.sin_port =  src_port; 
	// src.sin_addr.s_addr = inet_addr ("10.3.15.42");
	//src.sin_addr.s_addr = inet_addr ("192.168.1.48");

	// get current ip address
    char hostbuffer[256]; 
	int hostname; 
	char *IPbuffer; 
    struct hostent *host_entry; 
	hostname = gethostname(hostbuffer, sizeof(hostbuffer)); 
    host_entry = gethostbyname(hostbuffer); 
	IPbuffer = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0])); 
  	
	src.sin_addr.s_addr = inet_addr ((const char *)IPbuffer);


	printf("Ber saman hex á ip address:\ngethostbyname= 0x%x \nHardkodud= 0x%x\n",src.sin_addr.s_addr, inet_addr ("192.168.1.48"));
	// exit(0);
	


	if(bind(s_raw, (struct sockaddr *)&src, sizeof(src)) < 0)   // bind raw socket with source port
   	{
      perror("Failed to bind to socket:");
      return(-1);
   	}
	   
	// printf("%d -- %x\n", s_dgram,s_dgram);
	
	//Fill in the IP Header
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
	udph->uh_sport= htons(src_port);
	udph->uh_dport= htons(4000);
	udph->uh_ulen= htons(8 + strlen(data));	
	udph->uh_sum = 0;



	char buffer[4096];


	int n_reruns = 10;
	int port_lo = 4000;
	int port_hi = 4100;
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
				int index = sizeof(struct ip);
				// printf("%d\n",index); 

				//printf("%x -- %x\n",buffer[index],buffer[index+1]);
				//std::cout<<buffer[index]<<buffer[index+1]<<"\n";

				int p_numb = ((buffer[20] & 0xFF)<<8);
				p_numb += (buffer[21] & 0xFF);

				printf("\nAllur Buffer I hex:\n");
				for (int j = 0; j<c; j++){
					printf("%x ", buffer[j]);
				}
				

				if(not_in_arr(port_numbers, n_ports, p_numb)){    // if port not already in port_numbers
                    
                    if (n_ports == size){                      // resize array if to big
                        arr_resize(*& port_numbers);
                        size *= 2; 
                    }

                    port_numbers[n_ports] = p_numb;              // add port to port_numbers
                    n_ports++;

                    //printf("%d -- %s\n",port,buffer_rx);
                    printf("\n\nport %d\n",p_numb);
					printf("\nmessage í buffer:");
                    print_buffer((char*) buffer, sizeof(struct ip) + sizeof(struct udphdr), c);         

					       
                }
				exit(0);
				

			}
                
                


            }

            
        
         }


    

    delete [] port_numbers; // free memory 
    port_numbers = NULL; 

	
    
    

}




















