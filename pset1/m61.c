#define M61_DISABLE 1
#include "m61.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>

// push on 9-17 to bry

static unsigned long long nactive, active_size, ntotal, total_size, nfail, fail_size;
char* heap_min, heap_max;


/* per-allocation metadata */
typedef struct {
    size_t block_size;
    /* payload */
    const char* file;
    int line;
    /* padding */
    	// Not Applicable Yet
    /* Temp Address Holder */
    char *address;
} metadata;
// metadata main_metadata; 

/// m61_malloc(sz, file, line)
///    Return a pointer to `sz` bytes of newly-allocated dynamic memory.
///    The memory is not initialized. If `sz == 0`, then m61_malloc may
///    either return NULL or a unique, newly-allocated pointer value.
///    The allocation request was at location `file`:`line`.

void* m61_malloc(size_t sz, const char* file, int line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
    // Your code here.

   
    // Checking to see if conditions are met & defining Null pointer
    
    void *ptr = NULL;
    ptr = base_malloc(sz);  
    /* base malloc failed */
    if (!ptr){
	nfail++;
	fail_size+=sz;
	return NULL;	
    }
    else{
	/* get statistics */
	ntotal++;
	total_size+=sz;
	nactive++;
        active_size+=sz;    
	/* initialize metadata */
	metadata *main_metadata;
	main_metadata->block_size=sz;
	main_metadata->file=file;
	main_metadata->line=line;

	/* setting up min heap stuff to check min address - doesn't work 
		int ptr2 = base_malloc(sz);
		int value = strcmp(&ptr2, main_metadata.address);
		if(value < 0){
			heap_min = (long long) &ptr2;
		//memcpy(&heap_min, &ptr2, sizeof(unsigned long long );
		}
		main_metadata.address = &ptr2;
		return ptr2;
	*/
	return base_malloc(sz);
    }
}


/// m61_free(ptr, file, line)
///    Free the memory space pointed to by `ptr`, which must have been
///    returned by a previous call to m61_malloc and friends. If
///    `ptr == NULL`, does nothing. The free was called at location
///    `file`:`line`.

void m61_free(void *ptr, const char *file, int line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
    // Your code here.

    // active_size isn't defined right, I just free 3x the amt of ptr size for now
    // active_size-=sizeof(*ptr)*3;
    /* New version of active_size, if fail, version above is good */
    active_size-=sizeof(*ptr)*3;
// metadata *metadata1 = ((metadata*) ptr-1);	
// active_size-=metadata1->block_size;
    /* Just like in realloc, I checked size_allocation to know
	how much mem to remove from active_size */
    
    	//size_t asize = (*(metadata*) ptr).block_size;
	//size_t asize = (*(metadata*) ptr).block_size; active_size-=asize;
    nactive--;
    base_free(ptr);
}


/// m61_realloc(ptr, sz, file, line)
///    Reallocate the dynamic memory pointed to by `ptr` to hold at least
///    `sz` bytes, returning a pointer to the new block. If `ptr` is NULL,
///    behaves like `m61_malloc(sz, file, line)`. If `sz` is 0, behaves
///    like `m61_free(ptr, file, line)`. The allocation request was at
///    location `file`:`line`.

void* m61_realloc(void* ptr, size_t sz, const char* file, int line) {
    void* new_ptr = NULL;
    if (sz)
        new_ptr = m61_malloc(sz, file, line);
    if (ptr && new_ptr) {
        // Copy the data from `ptr` into `new_ptr`.
        // To do that, we must figure out the size of allocation `ptr`.
        // Your code here (to fix test012).
	
	/* Casts ptr to Metadata structure type, and then checks block_size */
	size_t asize = (*(metadata*) ptr).block_size;
	if(asize<sz){
		memcpy(new_ptr,ptr,asize);
	}
	else{
		memcpy(new_ptr,ptr,sz);
	}

    }
    m61_free(ptr, file, line);
    return new_ptr;
}


/// m61_calloc(nmemb, sz, file, line)
///    Return a pointer to newly-allocated dynamic memory big enough to
///    hold an array of `nmemb` elements of `sz` bytes each. The memory
///    is initialized to zero. If `sz == 0`, then m61_malloc may
///    either return NULL or a unique, newly-allocated pointer value.
///    The allocation request was at location `file`:`line`.

void* m61_calloc(size_t nmemb, size_t sz, const char* file, int line) {
    // Your code here (to fix test014).
   if (nmemb * sz < sz || nmemb * sz  < nmemb) {
        nfail++;
        return NULL;
    }
    void* ptr = m61_malloc(nmemb * sz, file, line);
    if (ptr)
        memset(ptr, 0, nmemb * sz);
    return ptr;
}


/// m61_getstatistics(stats)
///    Store the current memory statistics in `*stats`.

void m61_getstatistics(struct m61_statistics* stats) {
    // Stub: set all statistics to enormous numbers
    memset(stats, 255, sizeof(struct m61_statistics));
	stats->nactive=nactive;
	stats->active_size=active_size;
	stats->ntotal=ntotal;
	stats->total_size=total_size;
	stats->nfail=nfail;
	stats->fail_size=fail_size;
	stats->heap_min=heap_min;
	// stats->heap_max=heap_max;
}


/// m61_printstatistics()
///    Print the current memory statistics.

void m61_printstatistics(void) {
    struct m61_statistics stats;
    m61_getstatistics(&stats);

    printf("malloc count: active %10llu   total %10llu   fail %10llu\n",
           stats.nactive, stats.ntotal, stats.nfail);
    printf("malloc size:  active %10llu   total %10llu   fail %10llu\n",
           stats.active_size, stats.total_size, stats.fail_size);
}


/// m61_printleakreport()
///    Print a report of all currently-active allocated blocks of dynamic
///    memory.

void m61_printleakreport(void) {
    // Your code here.
}

