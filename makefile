server.exe		: 	server.o 
	g++ server.cpp -lws2_32 -o server.exe 
			
server.o		:	server.cpp 
	g++ -c -Wall -O2 -fconserve-space server.cpp

clean:
	del *.o
	del *.exe
