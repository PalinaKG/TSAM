Project 2 - Tölvusamskipti TSAM - Group 6 (Jónas Már Kristjánsson and Pálína Kröyer Guðmundsdóttir)



To compile the code 
- Enter "make" into the command line while being in the directory with the codes.

  -- Note: Along with compiling the codes this will make a text file called IPv4address.txt
     containing the host IPv4 address which will be used in all of the executeable files. 


General about the codes:
- All runs require root privileges because they all create raw sockets.
- If the any run takes more than 2 seconds to run (except the scanner in part 1) 
  then kill the process and start over. As well as when a non-english message appears
  then run it again. 
  This is usually due to the raw socket accidentally taking in a wrong UDP packet or 
  the UDP packet was dropped. 
- Part 2 and 3 have skel.ru.is IP address coded into them.



Part 1 - Scanner 
- To start the UDP port scanner run the command in terminal

	"sudo ./scanner <IP address> <port high> <port low>" 
	
  f.x. to search ports 4000-4100 on skel.ru.is this would be: 
  "sudo ./scanner 130.208.243.61 4000 4100"
  
  Notes: 
  -- The output will be printed out on the screen. 
  -- This takes around 0.7s per port so in the skel.ru.is example would be approx. 70s.
  
  
  
Part 2 - Puzzle port solver
- To start the solver run the command

	"sudo ./solver"
	
  Then follow the instruction printed out on the display. 
  
  Notes:
  -- This code needs to be ran on a machine connected to the RU internet 
     via VPN, Wifi or line. 
  


Part 3 - Secret knocker 
- To start the secret knocker run the command

	"sudo ./knocker <Oracle port> <hidden port 1>,<hidden port 2>" 
	
  f.x. let's say that the Oracle port found in part 1 is 4042, and one of the hidden 
  port is found in part 1 to be 4008 and the other is found behind the evil bit port
  to be 4015, then the command would be:
  "sudo ./knocker 4042 4008,4015" 
  
  Note:
  -- The secret phrase from part 2 is already in the message.


 















