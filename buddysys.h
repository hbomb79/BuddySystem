//////////////////////////////////////////////////////////////////////////////////
//
//   Program Name:  Buddy System Algorithm
//                  
//   Description:  Buddy System Algorithm
//                  
//   Student name: Harry Felton, 18032692
//
//
//////////////////////////////////////////////////////////////////////////////////


#ifndef __BUDDYSYS_H__
#define __BUDDYSYS_H__


#include "auxiliary.h"
// The size of the free list, which uses 'k' as it's index
// where 2^k should be the size of the initial free block
// As we're using stack based arrays, this means the
// value must be hardcoded - adjust this value to match 
// the size of the *wholememory Node provided to the
// constructor.
#define SIZE_OF_FREE_LIST 26

#define BUDDY_SYS_DEBUG 1

extern long long int MEMORYSIZE;


// shorter, replace cast to (char *) with cast to (byte *)
// Allows us to use (byte *) as a typecast rather than (unsigned char *)
// Nice quality of life improvement
typedef unsigned char byte;

typedef struct Node {
    //size of the block (ONLY for data, this size does not consider the Node size! (so it is same as s[k])
    long long int size;

    // 0 is free, 1 means allocated - We use this variable here so that
    // when searching for buddy-blocks to consolidate in to one we can
    // tell if it's allocated without having to go and check
    // the free list for the buddy blocks node presence
    int alloc;

    // Pointer to the next node; NULL if none
    struct Node * next;

    // Pointer to the next node; NULL if none.
    struct Node * previous;
} Node;

// Decalre the wholememory pointer as an extern(ally) defined variable.
extern Node *wholememory;

///////////////////////////////////////////////////////////////////////////////////

class BuddySystem {
    Node* freeList[SIZE_OF_FREE_LIST];
    uintptr_t baseMemoryAddress;
    int upperK;
    int lowerK;
public:
    BuddySystem();
    void init(Node* wholememory);
    void* malloc(int request_memory); 
    int free(void *p);
protected:
    Node* splitNode(Node* node);
    Node* coalesceFree(Node* node);
    Node* cascadeSplit(int startingBinSize, int desiredBinSize);

    uintptr_t findBuddyBlock(Node* node);

    void insertToFree(Node* node);
    void ejectFromFree(Node* node);

    int determineBinK(int request_size);
    int determineBinK(Node* node);

    bool binHasNode(int binK);
    int findFirstBin(int binK);
    Node* getFromBin(int binK);

    void debugNodeStructure();
    template<typename... Args>
    void debugPrintF(const char* fmt, Args... args);
};


#endif