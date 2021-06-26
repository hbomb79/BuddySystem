
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Program Name:  Buddy System
//                  
// Description:   Memory Management using the Buddy System Algorithm.
//                This work is based largely on the previous assignments made by Martin Johnson 
//                and Andre Barczak.
// 
//
// Submitted by: 
//      Harry Felton, ID number: 18032692
//
//
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//
// Author of Start-up code:   Napoleon Reyes
//
// References:
//
//  Martin Johnson's codes and assignment design
//  Andre Barczak's codes and assignment design
//  https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocesstimes
//  https://docs.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-filetime
//
//  VIRTUAL ADDRESS SPACE AND PHYSICAL STORAGE: https://docs.microsoft.com/en-us/windows/win32/memory/virtual-address-space-and-physical-storage
//  MEMORY MANAGEMENT:  https://docs.microsoft.com/en-us/windows/win32/memory/about-memory-management
//  MEMORY PROTECTION CONSTANTS:
//     https://docs.microsoft.com/en-nz/windows/win32/memory/memory-protection-constants
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////



#include "auxiliary.h"
#include "buddysys.h"

using namespace std;

unsigned seed;

Node *wholememory;

long long int MEMORYSIZE;

// Honestly.. no idea how to adjust this
// It has to be a factor of 2^k, so
// the next lowest if 4096 which is
// WAY too small. I have no idea
// how to make it slightly smaller
// while making sure the initial memory
// block is still in terms of 2^k...
// * Please let me know how I was supposed to do this.
#define NUMBEROFPAGES  8192//7200  


///////////////////////////////////////////////////////////
//-------------------------------------
// WHICH RUNNING TIME ESTIMATOR?
//-------------------------------------
// - enable one
// (1) QUERY_PERFORMANCE_COUNTER 
     #define USE_QUERY_PERFORMANCE_COUNTER true
//
// (2) GETPROCESSTIMES
       // #define USE_GETPROCESSTIMES true //Google Benchmark codes: //https://github.com/google/benchmark/blob/master/src/timers.cc#L67-L75
                                //https://docs.microsoft.com/en-nz/windows/win32/sysinfo/acquiring-high-resolution-time-stamps?redirectedfrom=MSDN
//--------------------------------------
///////////////////////////////////////////////////////////

// #define DEBUG_MODE
// #define DEBUGGINGPRT

///////////////////////////////////////////////////////////
//---------------------------------------
// WHICH MEMORY MANAGEMENT STRATEGY?
//---------------------------------------

//---------------------------------------
//(1) use built-in C functions
// change the following lines to test the real malloc and free
// const string strategy = "malloc"; //enable this to test the system MALLOC & FREE
// #define MALLOC malloc //enable this to test the system MALLOC & FREE
// #define FREE free //enable this to test the system MALLOC & FREE

//---------------------------------------
//(2) use user-defined functions
// const string strategy = "mymalloc";
// #define MALLOC mymalloc
// #define FREE myfree

//---------------------------------------
//Enable the following compiler directives to test your implementation of buddy system strategy
//(3) use Buddy System
const string strategy = "Buddy System"; //enable this to test the Buddy System
#define USE_BUDDY_SYSTEM  //enable this to test the Buddy System
#define MALLOC buddySystem.malloc //enable this to test the Buddy System
#define FREE buddySystem.free //enable this to test the Buddy System
//---------------------------------------
///////////////////////////////////////////////////////////

/* Globals for the BuddySystem class instance */
// Class is used here to avoid cluttering the global
// namespace with all the additional methods implemented
// in buddysys.cpp
BuddySystem buddySystem;

//////////////////////////////////////////////////////////////////////////////////////////////
// MAIN FUNCTION
////////////////////////////////////////////////////////////////////////////////////////////////////
int main() {
   int i,k;
   unsigned char *n[NO_OF_POINTERS]; // used to store pointers to allocated memory
   int size;
      
   unsigned int s[NO_OF_POINTERS]; // size of memory allocated - for testing

   double  start_time;

   DWORDLONG start_mem;

   seed=7652; //DO NOT CHANGE THIS SEED FOR RANDOM NUMBER GENERATION

   for(i=0;i<NO_OF_POINTERS;i++) {
      n[i]=0;     // initially nothing is allocated
   }
   
///////////////////////////////////////////////////////////
//---------------------------------------
// WHICH TEST ROUTINE?
//---------------------------------------
#ifdef RUN_SIMPLE_TEST
   cout << "=========================================" << endl;
   cout << "          << RUN SIMPLE TEST >>" << endl;
   cout << "=========================================" << endl;
#else
   #ifdef RUN_COMPLETE_TEST 
   cout << "=========================================" << endl;
   cout << "          << RUN COMPLETE TEST >>" << endl;

   #ifdef USE_SIMULATION_2
   cout << "          << SIMULATION 2 >>" << endl;
   #else
   cout << "          << SIMULATION 1 >>" << endl;
   #endif 

   cout << "=========================================" << endl;

   #endif

#endif

//---------------------------------------
//Record start time
//---------------------------------------
#ifdef USE_GETPROCESSTIMES   
   start_time=cputime();
#endif   

#ifdef USE_QUERY_PERFORMANCE_COUNTER   
   //---
   LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
   LARGE_INTEGER Frequency;

   QueryPerformanceFrequency(&Frequency); 
   QueryPerformanceCounter(&StartingTime);
   //---
   cout << "QueryPerformanceCounter started." << endl;
#endif

   // getSystemInfo();

 
   
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------------------
#ifdef USE_BUDDY_SYSTEM
   ////acquire one wholememory block
   if (wholememory==NULL) {


        #ifndef RUN_SIMPLE_TEST
              //MEMORYSIZE = (long long int) ((long long int)NUMBEROFPAGES * (long long int)PAGESIZE);
              MEMORYSIZE = (long long int) ((long long int)NUMBEROFPAGES * (long long int)PAGESIZE);
        #else
              MEMORYSIZE = 512; //bytes  -  RUN_SIMPLE_TEST  
        #endif

         //---  
         //VirtualAlloc - Reserves, commits, or changes the state of a region of pages in the virtual address space of the calling process. Memory allocated by this function is automatically initialized to zero.
         //  the return value is the base address of the allocated region of pages.
         wholememory=(Node*) VirtualAlloc(NULL, MEMORYSIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE); //works!
         //---

         wholememory->size=(long long int)(MEMORYSIZE-(long long int)sizeof(Node));//Data size only          
         wholememory->next=NULL;
         wholememory->previous=NULL;
         printf("\n\n\nwhole memory address: %ld, size: %lld bytes or %lld Megabytes.\n", wholememory, MEMORYSIZE, MEMORYSIZE/1000000);
         printf("The size of the structure for the Nodes is: %d, and single large node size is: %lld bytes or %lld Megabytes.\n",sizeof(Node), wholememory->size, wholememory->size/1000000);
         // printf("SIZE_BUDDY_LIST is %d \n",SIZE_BUDDY_LIST);

         // Instatiate our memory manager with the initial wholememory node
         // as it's baseline
         buddySystem.init(wholememory);
   }
   printf("Init complete\n");

#endif   
//-------------------------------------------------------------------------------------  
////////////////////////////////////////////////////////////////////////////////////////   

   ///////////////////////////////////////////////////////////////////////////////////////////////////////// 
   start_mem=memory();

   cout << "start memory = " << start_mem << " bytes" << ", or " << start_mem/1024 << " KiloBytes, or " <<  start_mem/1048576 << " MegaBytes, or " <<  start_mem/(1073741824) << " GigaBytes" << endl;

   /////////////////////////////////////////////////////////////////////////////////////////////////
   cout << "\n\n<<<<<<<<<<<<<<<<< Simulation starts... " << ", max iterations = " << NO_OF_ITERATIONS << " >>>>>>>>>>>>>>>\n";
   
//---------------------------------------
//Test routine
//---------------------------------------
long long int totalAllocatedBytes=0;
long long int totalFreeSpace=0;
long long int totalSizeOfNodes=0;

#ifdef RUN_SIMPLE_TEST


  int NUM_OF_REQUESTS=5; //sequence of requests
  char actions[] =      {'m' ,  'm', 'm', 'f', 'f'};  //1 = MALLOC, 0 = FREE
  int requests[] =      {13, 3, 110, 3, 13};  //if NUMBEROFPAGES_BUDDY is set to 64
  
  int pointer_index[] = { 0,  1, 2, 1, 0};
//======================================================================================= 
    
  char selectedAction;
  cout << "\n\n executing " << NUM_OF_REQUESTS << " rounds of combinations of memory allocation and deallocation..." << endl;

  for(int r=0; r < NUM_OF_REQUESTS; r++){
     
     #ifdef DEBUGGINGPRT
        cout << "---[Iteration : " << r << "]" << endl;
      #endif
      selectedAction = actions[r];

      size = (int)requests[r];
      k = pointer_index[r];

      
      switch(selectedAction){
        case 'm':
                   // #ifdef DEBUGGINGPRT
                       cout << "\n======>REQUEST: MALLOC(" << size << ") =======\n\n";
                   // #endif
                      
                    n[k]=(unsigned char *)MALLOC(size); // do the allocation
                    if(n[k] != NULL){
                       s[k]=size; // remember the size
                       totalAllocatedBytes = totalAllocatedBytes + s[k];
                       totalSizeOfNodes = totalSizeOfNodes + sizeof(Node);
                       #ifdef DEBUGGINGPRT
                          cout << "\t\t\t\tsuccessfully allocated memory of size: " << size << endl;   
                          //// printf("\t\tRETALLOC size %d at address %ld (block size %d at Nodeaddress %ld)\n",s[k], (Node*) ((uintptr_t)n[k]-(uintptr_t)wholememory),  s[k]+sizeof(Node), (Node*)( (uintptr_t)n[k]-(uintptr_t)sizeof(Node)-(uintptr_t)wholememory) );
                          
                          printf("\n\t\t\t\t << MALLOC() >>  relative address: %8ld size: %8d  (Node: %8ld Nodesize: %8d)\n", (Node*)((uintptr_t)n[k]-(uintptr_t)wholememory),  s[k], (Node*)((uintptr_t)n[k]-(uintptr_t)sizeof(Node)-(uintptr_t)wholememory) , s[k]+sizeof(Node) );
                       #endif   
                       

                       n[k][0]=(unsigned char) k;  // put some data in the first and 
               
                       if(s[k]>1) 
                          n[k][s[k]-1]=(unsigned char) k; // last byte

                    } else {
                       cout << "\tFailed to allocate memory of size: " << size << endl;   
                    }
                   break;
        case 'f':
                   // #ifdef DEBUGGINGPRT
                       cout << "\n======>REQUEST: FREE(n[" << pointer_index[r] << "]) =======\n\n";
                   // #endif
                   if(n[k]) { // if it was allocated then free it
                       // check that the stuff we wrote has not changed
                       
                       if ( (n[k][0]) != (unsigned char) k)//(n[k]+s[k]+k) )
                          printf("\t\t==>Error when checking first byte! in block %d \n",k);
                       if(s[k]>1 && (n[k][s[k]-1])!=(unsigned char) k )//(n[k]-s[k]-k))
                          printf("\t\t==>Error when checking last byte! in block %d \n",k);

                       #ifdef DEBUGGINGPRT
                         cout << "\n======>REQUEST: FREE(" << hex << n[k] << ") =======\n\n";
                         printf("\n\t\t\t\t << FREE() >>  relative address: %8ld size: %8d  (Node: %8ld Nodesize: %8d)\n", (Node*)((uintptr_t)n[k]-(uintptr_t)wholememory),  s[k], (Node*)((uintptr_t)n[k]-(uintptr_t)sizeof(Node)-(uintptr_t)wholememory) , sizeof(Node) );
                       #endif
                       FREE(n[k]);
                       totalFreeSpace = totalFreeSpace + s[k];      

                    } 
                   break; 
      }



  }

  
#endif


////////////////////////////////////////////////////////

#ifdef RUN_COMPLETE_TEST  
   cout << "\n\n executing " << NO_OF_ITERATIONS << " rounds of combinations of memory allocation and deallocation..." << endl;
    
   for(i=0;i<NO_OF_ITERATIONS;i++) {

    #ifdef DEBUG_MODE
      cout << "iteration: " << i << endl;
    #endif
      k=myrand() % NO_OF_POINTERS; // pick a pointer

      if(n[k]) { // if it was allocated then free it
         // check that the stuff we wrote has not changed       
         if ( (n[k][0]) != (unsigned char) k)//(n[k]+s[k]+k) )
            printf("Error when checking first byte! in block %d \n",k);
         if(s[k]>1 && (n[k][s[k]-1])!=(unsigned char) k )//(n[k]-s[k]-k))
            printf("Error when checking last byte! in block %d \n",k);

         FREE(n[k]);         
      }
      size=randomsize(); // pick a random size
      #ifdef DEBUG_MODE
        cout << "\tPick random size to allocate: " << size << endl;
      #endif
        
      n[k]=(unsigned char *)MALLOC(size); // do the allocation
      if(n[k] != NULL){
         #ifdef DEBUG_MODE
            cout << "\tallocated memory of size: " << size << endl;   
         #endif   
         s[k]=size; // remember the size

         n[k][0]=(unsigned char) k;  // put some data in the first and 
 
         if(s[k]>1) n[k][s[k]-1]=(unsigned char) k; // last byte

      } else {
         cout << "\tFailed to allocate memory of size: " << size << endl;   
      }    
      
   }
#endif

   cout << "\n<<<<<<<<<<<<< End of simulation >>>>>>>>>>>>\n\n"; 
   /////////////////////////////////////////////////////////////////////////////////////////////////


//---------------------------------------
// Performance Report
//---------------------------------------

   cout << "========================================================" << endl;
   cout << "\n----------<< " << strategy.c_str() << " PERFORMANCE REPORT >>---------\n" << endl;

   

#ifdef  USE_GETPROCESSTIMES   
   //printf("That took %.3f seconds and used %d bytes\n", ((double)(cputime()-start_time))/1000, start_mem-memory());
   printf("That took %.3f seconds and used %d bytes\n", ((double)(cputime()-start_time)), start_mem-memory());
#endif

#ifdef USE_QUERY_PERFORMANCE_COUNTER
   //---
   QueryPerformanceCounter(&EndingTime);
   ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;

   //
   // We now have the elapsed number of ticks, along with the
   // number of ticks-per-second. We use these values
   // to convert to the number of elapsed microseconds.
   // To guard against loss-of-precision, we convert
   // to microseconds *before* dividing by ticks-per-second.
   //

   ElapsedMicroseconds.QuadPart *= 1000000;
   ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
   cout << "Elapsed Time = " << ElapsedMicroseconds.QuadPart << " microsec(s)." << endl;
#endif
   
#ifdef USE_BUDDY_SYSTEM
   cout << " " << endl;
   cout << "whole memory address: " << wholememory << ", size: " << MEMORYSIZE << " bytes or " << MEMORYSIZE/1000000 << " Megabytes.\n";
   printf("The size of the structure for the Nodes is: %d, and single large node size is: %lld bytes or %lld Megabytes.\n",sizeof(Node), wholememory->size, wholememory->size/1000000);
   // printf("SIZE_BUDDY_LIST is %d \n",SIZE_BUDDY_LIST);
   cout << "-------------------------------------------------------- " << endl;      
#endif   

#ifndef USE_BUDDY_SYSTEM
   cout << "Memory Left: " << memory() << ", or " << memory()/1024 << " KiloBytes, or " <<  memory()/1048576 << " MegaBytes, or " <<   memory()/(1073741824) << " GigaBytes" << endl;
   cout << "Memory Used " << (start_mem - memory()) << " bytes, or " << (start_mem - memory())/1024 << " Kilobytes, or " << (start_mem - memory())/1048576 << " Megabytes" << endl;
   cout << "--------------------------------------------------------" << endl;
   cout << "========================================================" << endl;
#endif   


   return 0;
}

