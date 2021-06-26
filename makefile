#g++ or direct path to it
CC = g++
#Mingw or Unix
CompilerVersion = Mingw

main.exe : main.o auxiliary.o buddysys.o 
	$(CC) -O2 -Wl,-s -o main.exe main.o auxiliary.o buddysys.o 
			
main.o : main.cpp auxiliary.h buddysys.h
	$(CC) -O2 -c main.cpp 


buddysys.o : buddysys.cpp buddysys.h auxiliary.h	 
	g++ -O2  -std=c++11  -c buddysys.cpp
		

auxiliary.o : auxiliary.cpp auxiliary.h	 
	g++ -O2  -std=c++11  -c auxiliary.cpp

clean:
	del *.o
	del *.exe
