#define M61_DISABLE 1
#include "m61.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>

// push on 9-17 to bry

/* variables from m61.h for getstats */
static unsigned long long nactive, active_size, ntotal, total_size, nfail, fail_size;
char* heap_min;
char* heap_max;

/* metadata structure to get active_size */
typedef struct {
    size_t block_size;
    const char* file;
    int line;
} metadata;
metadata main_metadata; 

/// m61_malloc(sz, file, line)
///    Return a pointer to `sz` bytes of newly-allocated dynamic memory.
///    The memory is not initialized. If `sz == 0`, then m61_malloc may
///    either return NULL or a unique, newly-allocated pointer value.
///    The allocation request was at location `file`:`line`.

void* m61_malloc(size_t sz, const char* file, int line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
    // Your code here.
    void *ptr = NULL;
    /* allocating more space than the user requested for metadata */
    ptr = base_malloc(sz+sizeof(metadata));  
    /* if base_malloc fails or succeeds */
    if (!ptr){
	nfail++;
	fail_size+=sz;
	return ptr;	
    }
    else{
	/* updating statistics */
	ntotal++;
	total_size+=sz;
	nactive++;
        active_size+=sz;    
	/* initializing metadata */
	metadata *main_metadata = (metadata*) ptr;
	main_metadata->block_size=sz;
	main_metadata->file=file;
	main_metadata->line=line;
	/* updating more statistics */
	/*
	if(heap_max==NULL || heap_max<ptr)
		heap_max=ptr;

	return ptr;
	*/
	// return ptr + sizeof(metadata);
    }
}


///    m61_free(ptr, file, line)
///    Free the memory space pointed to by `ptr`, which must have been
///    returned by a previous call to m61_malloc and friends. If
///    `ptr == NULL`, does nothing. The free was called at location
///    `file`:`line`.

void m61_free(void *ptr, const char *file, int line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
    // Your code here.
    metadata *main_metadata = (metadata*) ptr;
  
    /* updating statistics */
    nactive--;
    active_size-=main_metadata->block_size;
    /* initializing metadata */
    main_metadata->file=file; 
    main_metadata->line=line;
    base_free((metadata*) ptr);
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
		size_t asize = main_metadata.block_size;
		if(asize <= sz){
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

