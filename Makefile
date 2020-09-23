default: 
	g++ -Wall -std=c++11 scanner.cpp -o scanner
	g++ -Wall -std=c++11 part2-solver.cpp -o solver
	g++ -Wall -std=c++11 part3.cpp -o knocker
	hostname -I > IPv4address.txt
        
