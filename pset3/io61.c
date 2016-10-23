#include "io61.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>


// io61_file
//    Data structure for io61 file wrappers. Add your own stuff.

#define BUFSZ 16384

struct io61_file {	
    int fd;
    int mode;
	unsigned char* memory;
	size_t file_size;
	size_t first;
	size_t last;
    unsigned char cbuf[BUFSZ];
	size_t cache_size;
    off_t tag; // file offset of first character in cache
	off_t prev_tag; // offset of previous next
    off_t end_tag; // file offset one past last valid char in cache
    off_t pos_tag; // file offset of next char to read in cache
};


// io61_fdopen(fd, mode)
//    Return a new io61_file for file descriptor `fd`. `mode` is
//    either O_RDONLY for a read-only file or O_WRONLY for a
//    write-only file. You need not support read/write files.

io61_file* io61_fdopen(int fd, int mode) {
    assert(fd >= 0);
    io61_file* f = (io61_file*) malloc(sizeof(io61_file));
    f->fd = fd;
    f->mode = mode;
	f->file_size = io61_filesize(f);
	f->memory = calloc(BUFSZ, sizeof(char));
    f->tag = f->end_tag = f->pos_tag = f->cache_size = f->first = f->last = f->prev_tag = 0;

    return f;
}


// io61_close(f)
//    Close the io61_file `f` and release all its resources.

int io61_close(io61_file* f) {
    if((f->mode & O_ACCMODE) != O_RDONLY)
	io61_flush(f);
    // io61_flush(f);
    int r = close(f->fd);
    free(f);
    return r;
}


// io61_readc(f)
//    Read a single (unsigned) character from `f` and return it. Returns EOF
//    (which is -1) on error or end-of-file.

int io61_readc(io61_file* f) {
	if (f->mode != O_RDONLY)
        return -1;
	if (f->pos_tag < f->end_tag) {
        f->pos_tag++;
        return *(f->memory + f->pos_tag - f->tag - 1);
    }
	// cache is empty
    else {
        f->tag = f->end_tag;
		// Read from the file
        ssize_t size = read(f->fd, f->memory, BUFSZ);
		// If read successfully
        if (size > 0) {
            f->end_tag += size;
            f->pos_tag++;
			// Read the next char from cache
            return *(f->memory + f->pos_tag - f->tag - 1);
        }
        else
            return EOF;
    }



    //if(f->pos_tag < f->end_tag){
	//char c = f->cbuf[f->pos_tag];
	//++f->pos_tag;
       // return c;
   // }
   // else {
	//return EOF;
 //   }
/* Original Implementation Provided...
    unsigned char buf[1];
    if (read(f->fd, buf, 1) == 1)
        return buf[0];
    else
        return EOF;
*/
}


// io61_read(f, buf, sz)
//    Read up to `sz` characters from `f` into `buf`. Returns the number of
//    characters read on success; normally this is `sz`. Returns a short
//    count, which might be zero, if the file ended before `sz` characters
//    could be read. Returns -1 if an error occurred before any characters
//    were read.

ssize_t io61_read(io61_file* f, char* buf, size_t sz) { 
    size_t nread = 0;
    while (nread != sz) {
    	if(f->pos_tag < f->end_tag) {
        	ssize_t n = sz - nread;
                if (n > f->end_tag - f->pos_tag)
                	n = f->end_tag - f->pos_tag;
                memcpy(&buf[nread], &f->cbuf[f->pos_tag - f->tag], n);
                f->pos_tag += n;
                nread += n;
         }else{
		f->tag = f->end_tag; // mark cache as empty
            ssize_t n = read(f->fd, f->cbuf, BUFSZ);
            if(n > 0)
            	f->end_tag += n;
            else
                return nread ? (ssize_t) nread : (ssize_t) n;
 		}        
    }  
    return nread;
}	


// io61_writec(f)
//    Write a single character `ch` to `f`. Returns 0 on success or
//    -1 on error.

int io61_writec(io61_file* f, int ch) {
	if (f->mode != O_WRONLY)
        return -1;
	const char* buf = (const char*) &ch;
	// how much space is available in cache
	size_t available = BUFSZ - f->cache_size;
	if (!available) {
		//write the cache to the file.
        ssize_t nwritten = write(f->fd, f->memory + f->first, BUFSZ - f->first);
		// If able to write
        if (nwritten >= 0) {
            f->first += nwritten;
            f->cache_size -= nwritten;
			// Check if at the end of cache 
            f->first = (BUFSZ == f->first) ? 0 : f->first;
            f->last = (BUFSZ == f->last) ? 0 : f->last;
        }
		//write not succcessful
		else
            return -1;   
    }
	// Write to cache
	 *(f->memory + f->last) = *buf;
    f->last++;
    f->cache_size++;
    return 0;
         
/* Original Implementation Provided...  
    unsigned char buf[1];
    buf[0] = ch;
    if (write(f->fd, buf, 1) == 1)
        return 0;
    else
        return -1;
*/
}


// io61_write(f, buf, sz)
//    Write `sz` characters from `buf` to `f`. Returns the number of
//    characters written on success; normally this is `sz`. Returns -1 if
//    an error occurred before any characters were written.

ssize_t io61_write(io61_file* f, const char* buf, size_t sz) {
   size_t nwritten = 0;
   if((f->mode & O_ACCMODE) != O_RDONLY){
   	while (nwritten != sz) {
       		if (f->pos_tag - f->tag < BUFSZ) { // If there is space in buffer
           	     ssize_t n = sz - nwritten; 
           	if (BUFSZ - (f->pos_tag - f->tag) < n)
               	     n = BUFSZ - (f->pos_tag - f->tag);
           	memcpy(&f->cbuf[f->pos_tag - f->tag], &buf[nwritten], n);
           	f->pos_tag += n;
           	if (f->pos_tag > f->end_tag)
                     f->end_tag = f->pos_tag;
           nwritten += n;
       }
       // The position should never exceed the end tag.
       assert(f->pos_tag <= f->end_tag);

       // Check if we've filled the buffer and if so, call flush to write data.
       if (f->pos_tag - f->tag == BUFSZ) // Indicates that the buffer is full
           io61_flush(f);
	}
   }
   return nwritten;
}


// io61_flush(f)
//    Forces a write of all buffered data written to `f`.
//    If `f` was opened read-only, io61_flush(f) may either drop all
//    data buffered for reading, or do nothing.

int io61_flush(io61_file* f) {

	// If f was opened read-only
    if (f->mode == O_RDONLY)
        return 0;

	if(f->end_tag != f->tag || (f->mode & O_ACCMODE) != O_RDONLY) {
		ssize_t n = write(f->fd, f->cbuf, f->end_tag - f->tag);
		assert(n == f->end_tag - f->tag);
    }
   	f->pos_tag = f->tag = f->end_tag;

    while (f->cache_size) {
        size_t size = f->first ? BUFSZ - f->first : f->last;
        //write the cache to the file.
        ssize_t cleared = write(f->fd, f->memory + f->first, size);
        // If able to write to file
        if (cleared > 0) {
            // update cache position and size
            f->first += cleared;
            f->cache_size -= cleared;
        }
        // Else if nothing was written
        else if (cleared == 0) {
            f->first = cleared;
        }
        // Else not able to write successfully       
        else
            return -1;
    }
    if(f->end_tag != f->tag){
	ssize_t n = write(f->fd, f->cbuf, f->end_tag - f->tag);
    	assert(n == f->end_tag - f->tag);
    }
    f->pos_tag = f->tag = f->end_tag;

    return 0;


    //if(f->end_tag != f->tag || (f->mode & O_ACCMODE) != O_RDONLY) {
	//ssize_t n = write(f->fd, f->cbuf, f->end_tag - f->tag);
	//assert(n == f->end_tag - f->tag);
    //}
   // f->pos_tag = f->tag = f->end_tag;
   // return 0;
}


// io61_seek(f, pos)
//    Change the file pointer for file `f` to `pos` bytes into the file.
//    Returns 0 on success and -1 on failure.


int io61_seek(io61_file* f, off_t pos) {
	if((f->mode & O_ACCMODE) != O_RDONLY)
		io61_flush(f);
   	if(pos < f->tag || pos > f->end_tag || (f->mode & O_ACCMODE) != O_RDONLY) {
        off_t aligned = pos - (pos % BUFSZ);
        off_t r = lseek(f->fd, aligned, SEEK_SET);
        if (r != aligned)
            return -1;
        f->tag = f->end_tag = aligned;
    }
    f->prev_tag = f->pos_tag;
    f->pos_tag = pos;
    return 0;
}


// You shouldn't need to change these functions.

// io61_open_check(filename, mode)
//    Open the file corresponding to `filename` and return its io61_file.
//    If `filename == NULL`, returns either the standard input or the
//    standard output, depending on `mode`. Exits with an error message if
//    `filename != NULL` and the named file cannot be opened.

io61_file* io61_open_check(const char* filename, int mode) {
    int fd;
    if (filename)
        fd = open(filename, mode, 0666);
    else if ((mode & O_ACCMODE) == O_RDONLY)
        fd = STDIN_FILENO;
    else
        fd = STDOUT_FILENO;
    if (fd < 0) {
        fprintf(stderr, "%s: %s\n", filename, strerror(errno));
        exit(1);
    }
    return io61_fdopen(fd, mode & O_ACCMODE);
}


// io61_filesize(f)
//    Return the size of `f` in bytes. Returns -1 if `f` does not have a
//    well-defined size (for instance, if it is a pipe).

off_t io61_filesize(io61_file* f) {
    struct stat s;
    int r = fstat(f->fd, &s);
    if (r >= 0 && S_ISREG(s.st_mode))
        return s.st_size;
    else
        return -1;
}


// io61_eof(f)
//    Test if readable file `f` is at end-of-file. Should only be called
//    immediately after a `read` call that returned 0 or -1.

int io61_eof(io61_file* f) {
    char x;
    ssize_t nread = read(f->fd, &x, 1);
    if (nread == 1) {
        fprintf(stderr, "Error: io61_eof called improperly\n\
  (Only call immediately after a read() that returned 0 or -1.)\n");
        abort();
    }
    return nread == 0;
}
