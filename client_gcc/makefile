CC = g++
CFLAGS = -Wall -lws2_32 -lgmp -lgmpxx

clientWindows.exe		: 	clientWindows.o rsa.o
	$(CC) clientWindows.o rsa.o $(CFLAGS) -o clientWindows.exe 
			
clientWindows.o		:	clientWindows.cpp 
	$(CC) -c -O2 -Wall clientWindows.cpp

rsa.o:	../rsa.cpp
	$(CC) -c -Wall -O2 -fconserve-space ../rsa.cpp

clean:
	cmd //C del *.o
	cmd //C del *.exe
