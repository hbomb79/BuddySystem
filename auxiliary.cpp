//////////////////////////////////////////////////////////////////////////////////
//
//   Program Name:  Auxiliary
//                  
//   Description:  Auxiliary functions, functions and constants
//                  
//   * you are not allowed to modify this file (auxiliary.cpp) 
//
//   Author: Napoleon Reyes
//
//   References:  
//
//     Martin Johnson's codes
//     Andre Barczak's codes
//     https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocesstimes
//     https://docs.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-filetime
//
//     VIRTUAL ADDRESS SPACE AND PHYSICAL STORAGE: https://docs.microsoft.com/en-us/windows/win32/memory/virtual-address-space-and-physical-storage
//     MEMORY MANAGEMENT:  https://docs.microsoft.com/en-us/windows/win32/memory/about-memory-management
//     MEMORY PROTECTION CONSTANTS:
//          https://docs.microsoft.com/en-nz/windows/win32/memory/memory-protection-constants

//
//////////////////////////////////////////////////////////////////////////////////

#include "auxiliary.h"


////////////////////////////////////////////////////////////////////////////////////

//To determine the size of a page and the allocation granularity on the host computer
void getSystemInfo(){
    SYSTEM_INFO sSysInfo;         // Useful information about the system

    GetSystemInfo(&sSysInfo);     // Initialize the structure.

    cout << "This computer has page size: " << sSysInfo.dwPageSize << endl;

    cout << "Reserved pages allocated: " << MEM_RESERVE << endl;  
    cout << "Constants: PAGE_NOACCESS: " << PAGE_NOACCESS << ", PAGE_READWRITE: " << PAGE_READWRITE << ", MEM_RELEASE:" << MEM_RELEASE << endl;     
}

//---
double MakeTime(FILETIME const& kernel_time, FILETIME const& user_time) {

  ULARGE_INTEGER kernel;
  ULARGE_INTEGER user;
  kernel.HighPart = kernel_time.dwHighDateTime;
  kernel.LowPart = kernel_time.dwLowDateTime;
  user.HighPart = user_time.dwHighDateTime;
  user.LowPart = user_time.dwLowDateTime;

  return (static_cast<double>(kernel.QuadPart) +
          static_cast<double>(user.QuadPart)) * 1e-7;

}
//---
double cputime(void) { // return cpu time used by current process
    HANDLE proc = GetCurrentProcess();

    FILETIME creation_time;
    FILETIME exit_time;
    FILETIME kernel_time;
    FILETIME user_time;

    if (GetProcessTimes(proc, &creation_time, &exit_time, &kernel_time, &user_time))
      return MakeTime(kernel_time, user_time);

    return 0; //napoleon
}
//---

DWORDLONG  memory(void) { // return memory available to current process
   ms m;

   m.dwLength = sizeof(m);
   //On computers with more than 4 GB of memory, the GlobalMemoryStatus function can return incorrect information, reporting a value of –1 to indicate an overflow. 
   // For this reason, applications should use the GlobalMemoryStatusEx function instead.
   //Reference: https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-globalmemorystatus

   GlobalMemoryStatusEx(&m); //Contains information about the current state of both physical and virtual memory, including extended memory. The GlobalMemoryStatusEx function stores information in this structure.
 
   return m.ullAvailVirtual; //The amount of unreserved and uncommitted memory currently in the user-mode portion of the virtual address space of the calling process, in bytes.
}

// you are not allowed to change the following function
void  *allocpages(int n) { // allocate n pages and return start address
   // VirtualAlloc reserves a block of pages with NULL specified as the base address parameter, forcing the system to determine the location of the block
   // VirtualAlloc is called whenever it is necessary to commit a page from this reserved region, and the base address of the next page to be committed is specified
   // Extra: To allocate memory in the address space of another process, use the VirtualAllocEx function.
   // return VirtualAlloc(NULL,n * PAGESIZE,4096+8192,4); //original
   return VirtualAlloc(NULL, n * PAGESIZE, PAGESIZE + MEM_RESERVE, PAGE_READWRITE); 
                     //Parameters: 
   
                     // NULL          = system selects address (where to allocate the region);
                     // n*dwPageSize = number of pages requested * PAGESIZE = Size of allocation
                     // MEM_RESERVE  = Allocate reserved pages
                     // PAGE_READWRITE = Enables read-only or read/write access to the committed region of pages. 
                     //                  If Data Execution Prevention is enabled, attempting to execute code in the committed region results in an access violation.
}
//---
// you are not allowed to change the following function
int freepages(void *p) { // free previously allocated pages.
  //return VirtualFree(p,0,32768); //original
  return VirtualFree(p,0, MEM_RELEASE); //// Release the block of pages
                       //p - A pointer to the base address of the region of pages to be freed.
                       //0 - Bytes of committed pages; If the dwFreeType parameter is MEM_RELEASE, this parameter must be 0 (zero).
                       //MEM_RELEASE - Decommit the pages and release the entire region of reserved and committed pages. constant is equivalent to 32768
}

//---
void *mymalloc(int n) { // very simple memory allocation
   void *p;
   p=allocpages((n/PAGESIZE)+1);
   if(!p) puts("Failed");
   return p;
}
//---
int myfree(void *p) { // very simple free
   int n;
   n=freepages(p);
   if(!n) puts("Failed");
   return n;
}


////////////////////////////////////////////////////////////////////////
//---
// SIMULATION 2
//2020 version
#ifdef USE_SIMULATION_2
  int myrand() { // pick a random number

     //seed=(seed*2416+374441) % 4095976;
     seed=(seed*2416+374441) % 1095976;
     return seed;
  }

  int randomsize() { // choose the size of memory to allocate
    int j,k;
    int n=0;
    j=0; //new
    k=0; //new
     
    k=myrand();
       
    j=(k&3)+(k>>2 &3)+(k>>4 &3)+(k>>6 &3)+(k>>4 &3)+(k>>4 &3);
    j=1<<j;
    //n = 2500 + (myrand() % (j<<8));
    n = 500 + (myrand() % (j<<5));
    
    return n;
  }

#endif
//---
// SIMULATION 1
//2010

////////////////////////////////////////////////////////////////////////
#ifdef USE_SIMULATION_1

  int myrand() { // pick a random number
     
     seed=(seed*2416+374441)%1771875;
     return seed;
  }

  int randomsize() { // choose the size of memory to allocate
     int j,k;
     j=0; //new
     k=0; //new

     k=myrand();
     j=(k&3)+(k>>2 &3)+(k>>4 &3)+(k>>6 &3)+(k>>8 &3)+(k>>10 &3);
     j=1<<j;
     return (myrand() % j) +1;
  }
#endif
////////////////////////////////////////////////////////////////////////
//---






