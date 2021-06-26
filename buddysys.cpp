//////////////////////////////////////////////////////////////////////////////////
//
//   Program Name:  Buddy System Algorithm
//                  
//   Description:  Buddy System Algorithm
//                  
//   Student name: Harry Felton, 18032692
//
// TODO:
// [x] - Create doubly linked list for the free list
// [x] - Write method (splitNode) to take a node from the free list and split it in to two equally sized nodes
// [x] - Write method (coalesceFree) that finds the buddy block of a node being free and coalesces it if so
// [x] - Write method (determineBinK) to determine the best k-value to use given a certain request size. This will also
//      be used to find out how large our free list needs to be when initialising it.
// [x] - Implement an initialisation function (buddyInit()) for the buddy memory manager which will
//      create our doubly-linked list and populate it with the first and only (wholememory) node
// [x] - Implement the buddyMalloc function, which will determine the bin size so we can split the nodes if
//      needed, before returning a pointer to the nodes data (the Node* address + sizeof(node))
// [x] - Implement the buddyFree function, which accepts a pointer to the data and then walks back the size of the Node structure
//      before adding that node to the free list and then running coalesceFree to check if the existence of a new free block
//      will allow us to combine it with it's buddy block.
// [x] - Done?
//
// Notes:
// * The initial Node we get starts at the start address of the _entire memory block_
// * This assignment is going to involve lots of pointer arithmetic. Ensure we're factoring
//   in that that the address of a node is NOT the address of the data component.
// * There is a minimum size we can allocate, due to the sizeof(Node).
// * There is a maximum size we can allocate, dictated by Node* wholememory->size;
//
//
//////////////////////////////////////////////////////////////////////////////////

#include "buddysys.h"

using namespace std;

/////////////////////////////////////////////////////////////////////////////////

#include <stdarg.h>
#include <stdio.h>

/**
 * Base constructor - left blank as no work can be done until
 * the startup code has reserved the process memory (*wholememory).
 * 
 * We have to initialise this class on the stack, to avoid
 * using the `new` keyword, so using an 'init' method
 * seemed the best option.
 */
BuddySystem::BuddySystem() {}

/**
 * init, when provided with a Node* pointing to the 'wholememory' block this buddy system has to work
 * with, will initialise the `freeList` and insert this Node in to it at the appropiatte position in the
 * free list (determineBinK is used here to find the bin).
 */
void BuddySystem::init(Node *wholememory) {
    this->freeList[SIZE_OF_FREE_LIST] = { NULL };
    this->baseMemoryAddress = (uintptr_t)wholememory;
    this->upperK = determineBinK(wholememory);

    // This log operation will find the bin size for exactly 32bytes,
    // this leaves 0 bytes for data, so only the bin size above this
    // is usable (this is why we have a +1 on the end of the statement).
    this->lowerK = std::log2(sizeof(Node)) + 1;

    // Ensure the system is initialised with sane values and insert the first node in to the free list
    if(this->upperK <= this->lowerK) {
        throw std::logic_error("BuddySystem::init has failed - upperK and lowerK values are illogical!");
    } else {
        printf("[init]:: Successfully initialised buddy system with upperK/lowerK %d/%d\n", this->upperK, this->lowerK);
    }

    wholememory->alloc = 0;
    this->insertToFree(wholememory);
}

/**
 * Given a request size, malloc will attempt to find a node that
 * can satisfy the request. This may require splitting nodes of larger
 * sizes many times so that the request can be satisfied.
 * 
 * The pointer to the usable data section of the node is returned, or NULL
 * if the request could not be granted for any reason (e.g. insufficient memory space)
 */
void* BuddySystem::malloc(int request_memory) {
    // Find what bin we need to satisfy this request
    int binK = this->determineBinK(request_memory + sizeof(Node));
    this->debugPrintF("[malloc]:: Attempting to malloc %d where sizeof(Node) = %d, bin k = %d\n*** Free list before malloc: ***\n", request_memory, sizeof(Node), binK);
    this->debugNodeStructure();
    if(binK > this->upperK || binK < 0) {
        this->debugPrintF("[malloc]:: Failed to determine bin for request\n");
        return NULL;
    }

    // Find the closest bin that we have available to accomodate this request
    int foundBinK = this->findFirstBin(binK);
    this->debugPrintF("[malloc]:: Searching for bin size large enough for this request, found to be: %d\n", foundBinK);
    if(foundBinK < 0) {
        // -1 return means we are unable to satisfy this request as we have no free bins available
        // for this request size
        return NULL;
    }

    // If the foundBinK is bigger than what we want, perform a series of splits
    // to make it the right size.
    Node* binNode = NULL;
    if(foundBinK > binK) {
        this->debugPrintF("[malloc]:: Attempting cascade split %d - %d\n", foundBinK, binK);
        binNode = this->cascadeSplit(foundBinK, binK);

        // NULL return here means failure to perform the split.
        if(binNode == NULL) {
            this->debugPrintF("[malloc]:: Error, no bin could be split\n");
            return NULL;
        }
    } else if(foundBinK < binK) {
        return NULL;
    }
    
    // If the node is NULL (i.e. cascadeSplit wasn't used), and a free node exists (it should)
    // then grab it from the free list. If not, return NULL as this memory request cannot
    // be satisfied.
    if(binNode == NULL) {
        if(!this->binHasNode(foundBinK)) {
            this->debugPrintF("[malloc]:: Error, insufficient memory available!\n");
            return NULL;
        }

        binNode = this->getFromBin(foundBinK);
    }

    // Free the node (marking as allocated too)
    ejectFromFree(binNode);
    binNode->alloc = 1;

    this->debugPrintF("[malloc]:: Success, freeing node (with size of %d) and returning data pointer\n*** Free list after malloc: ***\n", binNode->size);
    this->debugNodeStructure();

    // Return data pointer for use by memory requester
    return (void *)((uintptr_t)binNode + (uintptr_t)sizeof(Node));
} 

/**
 * free accepts a pointer (*p) to a data section previously allocated by this system,
 * and will reassemble the Node structure associatted with it, and insert this Node
 * back in to the free list for allocation in the future.
 * 
 * If this request succeeds, the system will attempt to consolidate any free-buddy blocks
 * together to allow larger memory requests to be satisfied.
 */
int BuddySystem::free(void *p){
    Node* nodeToFree = (Node*)((uintptr_t)p - (uintptr_t)sizeof(Node));

    this->debugPrintF("[free]:: Memory free request node size = %d\n*** Free list before free: ***\n", nodeToFree->size + sizeof(Node));
    this->debugNodeStructure();
    nodeToFree->alloc = 0;
    while(true) {
        // Try to coalesce, if failure, NULL is returned
        nodeToFree = this->coalesceFree(nodeToFree);
        if(nodeToFree == NULL) {
            // Done
            break;
        }
    }
    
    this->debugPrintF("[free]::*** Free list after free: ***\n");
    this->debugNodeStructure();

    return 1;
}

/**
 * Given a node, split it in to two equal sized nodes taking care of ejecting
 * the node being split from the freelist, as well as adding the two
 * output nodes back to the free list at the correct bin size
 * 
 * Returns the Node pointer for the same address as the node input
 */
Node* BuddySystem::splitNode(Node* node) {
    this->debugPrintF("[splitNode]:: Attempting to split node with reported size of %d with k=%d\n", node->size, determineBinK(node));
    if(determineBinK(node) == this->lowerK) {
        throw std::invalid_argument("BuddySystem::splitNode(Node* node) failed to split 'node', doing so will breach the lower bin limit (lowerK)!\nThis node is as small as possible already.");
    }

    // Remove the node from the free list
    ejectFromFree(node);

    // Split the node address in half, this will give us the middle of the data
    uintptr_t base = (uintptr_t)node;
    uintptr_t offset = (uintptr_t)(node->size + sizeof(Node));

    Node* nodeA = (Node*)((uintptr_t)node);
    Node* nodeB = (Node*)((uintptr_t)node + (offset / (uintptr_t)2));

    // Node size is representative of the data only - add back the size for the node so that
    // our division calculation is actually acting on the entire block of memory concerning the
    // nodes.. including the struct data itself.
    long long int newSize = ((node->size + sizeof(Node))/2)-sizeof(Node);

    // Fill in the new split node
    nodeA->size = newSize;
    nodeA->alloc = 0;
    nodeB->size = newSize;
    nodeB->alloc = 0;
    
    // Insert both nodes in to the free list
    insertToFree(nodeB);
    insertToFree(nodeA);

    return nodeA;
}

/**
 * cascadeSplit will return a pointer to a Node structure, matching the
 * desiredBinK, by splitting nodes starting from startingBinK and
 * working down the bin sizes until desiredBinK is met.
 * 
 * Note, the Node* returned exists inside the free list and must be ejected
 * prior to allocation.
 * 
 * Returns NULL if fails, as is possible if:
 * - the startingBinK in the free list contains no nodes to split.
 * - the startingBinK exceeds the upperK limit
 * - the desiredBinK exceeds the lowerK limit
 */
Node* BuddySystem::cascadeSplit(int startingBinK, int desiredBinK) {
    if(startingBinK > this->upperK || desiredBinK < this->lowerK || !this->binHasNode(startingBinK)) {
        return NULL;
    }

    // For each iteration, take a node from the free list at 'k', split it
    // and then focus on that node for the next iteration
    Node* focus = this->getFromBin(startingBinK);
    for(int k = startingBinK; k > desiredBinK; k--) {
        this->debugPrintF("[cascadeSplit]:: Splitting node inside binK = %d of size %d\n", k, focus->size);
        focus = this->splitNode(focus);
    }

    return focus;
}

/**
 * Given a node, coalesceFree will search for it's buddy block and coalesce them if the buddy
 * block is free.
 * 
 * Returns the new coalesced block, or NULL if the buddy block was already allocated.
 */
Node* BuddySystem::coalesceFree(Node* node) {
    Node* buddy = (Node*)this->findBuddyBlock(node);
    this->ejectFromFree(node);

    this->debugPrintF("[coalesceFree]:: Attempting to merge buddy of node with size=%d, alloc=%d\n", node->size, node->alloc);
    this->debugPrintF("[coalesceFree]:: Buddy block for node has size=%d, alloc=%d\n", buddy->size, buddy->alloc);
    if(buddy->alloc == 1 || buddy->size != node->size) {
        this->debugPrintF("[coalesceFree]:: (!!) Buddy block already allocated, or is currently of the wrong size (is split).\n");
        
        node->alloc = 0;
        this->insertToFree(node);
        return NULL;
    }
    
    // Coalesce the memory blocks. First remove the buddy block from the free list
    this->ejectFromFree(buddy);

    // As these blocks form contiguous memory, find the node that exists first so we can
    // form one large node with them.
    uintptr_t nodeAddr = (uintptr_t)node;
    uintptr_t buddyAddr = (uintptr_t)buddy;
    Node* coalesced = (Node*)(nodeAddr < buddyAddr ? nodeAddr : buddyAddr);

    // The size is the two nodes data size, plus the size of one Node structure. As now
    // two nodes exist as one, meaning that one of the node structure is not needed anymore...
    // by adding this to the size, we're effectively reclaiming that memory as available data storage.
    coalesced->size = node->size + buddy->size + sizeof(Node);
    coalesced->alloc = 0;
    coalesced->next = NULL;
    coalesced->previous = NULL;

    this->insertToFree(coalesced);
    return coalesced;
}

/**
 * findBuddyBlock will return the address for the buddy block
 * of the Node provided as it's sole argument.
 */
uintptr_t BuddySystem::findBuddyBlock(Node* node) {
    uintptr_t address = (uintptr_t)node;
    uintptr_t start = this->baseMemoryAddress;
    uintptr_t size = (uintptr_t)(sizeof(Node)) + (uintptr_t)(node->size);

    return start + ((address - start) ^ size);
}

/**
 * Given a Node pointer, inserts it in to the free list at the correct bin
 */
void BuddySystem::insertToFree(Node* node) {
    // Given a node, find which bin we want to store it in!
    int k = determineBinK(node);
    if(k < 0) {
        throw std::domain_error("BuddySystem::insertToFree(Node* node) failed to insert 'node' in to free list, bin size (k) determined for this size is out of domain");
    }
    
    this->debugPrintF("[insertToFree]:: Attempting to insert a free node of size = %d into freelist @ k=%d\n", node->size, k);
    // Get first node. This may be NULL
    Node* start = freeList[k];

    // Point our next to this node and previous to NULL (as we're the first Node), also set it to free (alloc=0)
    node->next = start;
    node->previous = NULL;

    // If the bin already had a node in it, point it's previous to us now instead of NULL.
    if(start != NULL) {
        start->previous = node;
    }

    // Tell the list to point to us as the beginning of the linked list now
    freeList[k] = node;
}

/**
 * Given a Node pointer, finds it inside the free list and ejects it, fixing up the existing pointers in
 * the free list (if any) for that bin size
 */
void BuddySystem::ejectFromFree(Node* node) {
    int k = determineBinK(node);
    if(k < 0) {
        throw std::domain_error("BuddySystem::ejectFromFree(Node* node) failed to eject 'node' from free list, bin size (k) determined for this size is out of domain");
    }

    this->debugPrintF("[ejectFromFree]:: Attempting to eject a free node of size = %d from freelist @ k=%d\n", node->size, k);
    // If current has neither a next or previous, check if it's an orphan in the list
    if(node->next == NULL && node->previous == NULL) {
        if(freeList[k] == node) {
            freeList[k] = NULL;
        }

        return;
    }

    Node* left = node->previous;
    Node* right = node->next;
    
    // Take care of pointers for the adjacent nodes, including when one or both of these nodes don't exist.
    if(right == NULL) {
        // There is no node to the right, let the node to the left know that there is no next
        // In the case there is no left node either, the next if clause will take care of that and
        // tell freelist to point to right (NULL)
        if(left != NULL) {
            left->next = NULL;
        }
    } else {
        // Tell the next node that it's previous node is now the one before the node we're ejecting.
        right->previous = left;
    }

    if(left == NULL) {
        // The node we found was the first in the list. Point the free list to the node to the right
        freeList[k] = right;
    } else {
        // Point the left node's next, to the node to the right of the node we're ejecting
        left->next = right;
    }

    // Mark node as allocated, and set the linked list paramaters to NULL as it's no longer inside the list.
    node->next = NULL;
    node->previous = NULL;
}

/**
 * determineBinK will, given a request size (int size), find which bin in the freelist this
 * request belongs to, using the formula found in our lecture slides. The k value
 * associatted with this bin is returned.
 * 
 * 2^(k-1) < request_size < 2^k
 */
int BuddySystem::determineBinK(int request_size) {
    for(int k = this->lowerK; k <= this->upperK; k++) {
        if(std::pow(2, k - 1) < request_size && request_size <= std::pow(2, k)) {
            return k;
        }
    }

    return -1;
}

/**
 * Overloaded version of determineBinK where we provide a Node* instead of an integer size
 * This means we can assume the node is a perfect size for the free list, and can use
 * logarithms to quickly calculate the 'k' value associatted with the node
 * 
 * 2^k = size, so k = log2(size).
 */
int BuddySystem::determineBinK(Node* node) {
    return std::log2(node->size + sizeof(Node));
}

/**
 * binHasNode, when provided with a bin size, will return true if
 * a free node exists inside the bin size. binK here is the bin, where bin size
 * can be found using 2^binSize.
 */
bool BuddySystem::binHasNode(int binK) {
    return this->freeList[binK] != NULL;
}

/**
 * Returns a Node from the bin provided with the smallest address (i.e. the
 * earliest node in the memory block).
 */
Node* BuddySystem::getFromBin(int binK) {
    Node* current = this->freeList[binK];
    if(current == NULL) {
        return NULL;
    }

    uintptr_t min = (uintptr_t)current;
    current = current->next;
    while(current != NULL) {
        if((uintptr_t)current < min) {
            min = (uintptr_t)current;
        };

        current = current->next;
    }

    return (Node*)min;
}

/**
 * findFirstBin accepts a single parameter dictating the size of
 * the bin we're looking for - this method will work *up* the free
 * list, starting at the binK provided. It will return the index of
 * the first bin it finds with a free node available for
 * allocation
 */
int BuddySystem::findFirstBin(int binK) {
    for(binK; binK <= this->upperK; binK++) {
        if(this->binHasNode(binK)) {
            return binK;
        }
    }

    return -1;
}

/**
 * This debug function will print out the structure of the free list using
 * a graphical representation of each doubly-linked-list in the free list.
 * 
 * Only functions if BUDDY_SYS_DEBUG preprocessor define is set to 1
 */
void BuddySystem::debugNodeStructure() {
    this->debugPrintF("===== NODE DEBUG =====\n");
    for(int k = this->upperK; k >= this->lowerK; k--) {
        this->debugPrintF("k = %d contains: ", k);
        
        Node* n = this->freeList[k];
        while(n != NULL) {
            this->debugPrintF("%d <-> ", n->size + sizeof(Node));
            n = n->next;
        }
        this->debugPrintF("NULL;\n");
    }
    this->debugPrintF("======================\n");
}

/**
 * This debug function is used as a wrapper around `std::printf` to
 * stop debug output being displayed when debug is disabled...
 * 
 * Only functions if BUDDY_SYS_DEBUG preprocessor define is set to 1
 */
template<typename... Args>
void BuddySystem::debugPrintF(const char* fmt, Args... args) {
    if(!BUDDY_SYS_DEBUG) {
        return;
    }

    std::printf( fmt, args... );
}