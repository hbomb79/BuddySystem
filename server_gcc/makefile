CC = g++
CFLAGS = -Wall -lws2_32 -lgmp -lgmpxx

serverWindows.exe:	serverWindows.o rsa.o
	$(CC) serverWindows.o rsa.o $(CFLAGS) -o serverWindows.exe 
			
serverWindows.o:	serverWindows.cpp 
	$(CC) -c -Wall -O2 -fconserve-space serverWindows.cpp

rsa.o:	../rsa.cpp
	$(CC) -c -Wall -O2 -fconserve-space ../rsa.cpp

clean:
	cmd //C del *.o
	cmd //C del *.exe
