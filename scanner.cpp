#include <stdio.h>       // for printf
#include <stdlib.h>      // for exit()
#include <string.h>      // for strlen()

#include <sys/socket.h>  // for udp socket
#include <netinet/in.h>  // --//--
#include <netinet/udp.h> // --//--
#include <sys/types.h>   // recvfrom

#include <arpa/inet.h>   // inet_addr

#include <vector> 
#include <iostream>

void arr_resize(int* & arr );
bool not_in_arr( int * arr, int n_foundports ,int port_poss);
void print_arr(int * arr, int num_ports);
void print_msg(char* buf, unsigned int msg_size);

int main(int argc, char* argv[]){

    if (argc != 4){
		printf("Usage: scanner <ip address> <low port> <high port> \n");
        exit(0);
	}

    printf("Starting scan on IP:%s from port %s to %s\n\n", argv[1],argv[2],argv[3]);

    freopen("output.txt","w",stdout); // Write results i.e. port numbers and messages to output.txt 



    int udp_socket;                // create socket 
    char buffer_tx[64];            // transmission buffer
    int len_buffer_tx;             // length of transmission message   
    short int port_lo = atoi(argv[2]);
    short int port_hi = atoi(argv[3]);
    struct sockaddr_in destaddr;   // Destination address

    // initializing receive buffer 
    char buffer_rx[1300];
	struct sockaddr_storage src_addr;

	struct iovec iov[1];
	iov[0].iov_base=buffer_rx;
	iov[0].iov_len=sizeof(buffer_rx);

	struct msghdr message;
	message.msg_name=&src_addr;
	message.msg_namelen=sizeof(src_addr);
	message.msg_iov=iov;
	message.msg_iovlen=1;
	message.msg_control=0;
	message.msg_controllen=0;

    int n_reruns = 10;                  // number of reruns of the port range

    int size = 8; 
    int* port_numbers = new int[size];  // initialize array containing open ports numbers
    int n_ports = 0;                    // number of current found open ports



    strcpy(buffer_tx, "Are you open?"); // message to send
    len_buffer_tx = strlen(buffer_tx) +1;   // length of message +1, to not send the whole buffer
  

    // socket setup
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket<0){
        perror("ERROR - Failed to create socket\n");
        exit(0);
    }
    // initializing timeout for socket
	// https://stackoverflow.com/questions/13547721/udp-socket-set-timeout
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 10000;	  // 0.01s timeout 
	if (setsockopt (udp_socket, SOL_SOCKET, SO_RCVTIMEO ,&tv,sizeof(tv)) < 0)   // set timelimit on each socket
	{
		perror("Error socket setup options");
		exit(0);
	}

    // setting destination address
    destaddr.sin_family = AF_INET;
    destaddr.sin_addr.s_addr = inet_addr (argv[1]);  // inet_addr ("130.208.243.61");  
    

    for (int i = 0; i<n_reruns; i++){
        // printf("run: %d\n",i);
        for(int port = port_lo; port <= port_hi; port++){

            destaddr.sin_port = htons(port); // update port number

            if (sendto (udp_socket, buffer_tx, len_buffer_tx , 0, (struct sockaddr *) &destaddr, sizeof (destaddr)) < 0){
                perror("Error - Sendto failed");
            }
            // else{
            //     printf("Package sent to %d\n",port);
            // }      
            
            ssize_t count=recvmsg(udp_socket,&message,0);  
            
            
            if (count==-1) {
                // timout or package drop
            } else if (message.msg_flags&MSG_TRUNC) {
                printf("datagram too large for buffer: truncated\n");
            } else {
                // printf("%d -- %s\n",port,buffer_rx);
                
                if(not_in_arr(port_numbers, n_ports, port)){    // if port not already in port_numbers
                    
                    if (n_ports == size){                      // resize array if to big
                        arr_resize(*& port_numbers);
                        size *= 2; 
                    }

                    port_numbers[n_ports] = port;              // add port to port_numbers
                    n_ports++;

                    //printf("%d -- %s\n",port,buffer_rx);
                    printf("\nport %d\n",port);
                    print_msg(buffer_rx,count);                 
                }


            }

            
        
         }

    }
    // print_arr(port_numbers, n_ports);


    

    delete [] port_numbers; // free memory 
    port_numbers = NULL; 


    fclose (stdout); // close file
    freopen ("/dev/tty", "a", stdout);  // send printing back to console

    printf("Scan finished! \nResults can be seen in current working directory in output.txt\n");

    
    

}


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

void print_msg(char* buf, unsigned int msg_size){
    // prints out the messages from the buf
    for (unsigned int i = 0; i<msg_size; i++){
        std::cout<<buf[i];
    }
    std::cout<<"\n";
}


















