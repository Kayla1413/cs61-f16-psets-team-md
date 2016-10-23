#include "io61.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <sys/mman.h>
#include <string.h>

// io61.c
//    YOUR CODE HERE!

size_t cache_size = 64000;

typedef struct io61_cache {
    unsigned char* memory;
    size_t size;     // Current size of cache
    size_t first;    // Position of the first char in writing cache
    size_t last;     // Position of the last char in writing cache
    size_t start;     // offset of first character in cache
    size_t next;      // offset of next char to read in cache
    size_t previous_next; // offset of previous next
    size_t end;       // offset of last valid char in cache
} io61_cache;

// io61_file
//    Data structure for io61 file wrappers. Add your own stuff.

struct io61_file {
    int fd;
	int mode;
    size_t size;
	io61_cache* cache;
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
	f->size = io61_filesize(f);
	f->cache = malloc(sizeof(io61_cache));
	f->cache->memory = calloc(cache_size, sizeof(char));
	f->cache->start = 0;
	f->cache->next = 0;
	f->cache->end = 0;
	f->cache->size = 0;
	f->cache->first = 0;
	f->cache->last = 0;
	f->cache->previous_next = 0;
	
    return f;
}


// io61_close(f)
//    Close the io61_file `f` and release all its resources.

int io61_close(io61_file* f) {
    io61_flush(f);
    int r = close(f->fd);
	free(f->cache);
    free(f);
    return r;
}


// io61_readc(f)
//    Read a single (unsigned) character from `f` and return it. Returns EOF
//    (which is -1) on error or end-of-file.

int io61_readc(io61_file* f) {
    if (f->mode != O_RDONLY)
        return -1;
    if (f->cache->next < f->cache->end) {
        f->cache->next++;
        return *(f->cache->memory + f->cache->next - f->cache->start - 1);
    }
	// cache is empty
    else {
        f->cache->start = f->cache->end;
		// Read from the file
        ssize_t size = read(f->fd, f->cache->memory, cache_size);
		// If read successfully
        if (size > 0) {
            f->cache->end += size;
            f->cache->next++;
			// Read the next char from cache
            return *(f->cache->memory + f->cache->next - f->cache->start - 1);
        }
        else
            return EOF;
    }
}

// io61_read(f, buf, sz)
//    Read up to `sz` characters from `f` into `buf`. Returns the number of
//    characters read on success; normally this is `sz`. Returns a short
//    count, which might be zero, if the file ended before `sz` characters
//    could be read. Returns -1 if an error occurred before any characters
//    were read.

ssize_t io61_read(io61_file* f, char* buf, size_t sz) {
	if (f->mode != O_RDONLY)
        return -1;
	size_t nread = 0;
	size_t size;
	while (nread != sz) {
        // if not at end of cache
        if (f->cache->next < f->cache->end) {
			if ((int) (f->cache->end - f->cache->next) < (int) (sz - nread)) {
				size = f->cache->end - f->cache->next;
			} 
			else {
				size = sz - nread;
			}
			//copy from cache to buffer
			memcpy(buf + nread, f->cache->memory +  f->cache->next - f->cache->start, size);
			//update the next cache position
			f->cache->next += size;
			//update the number of chars read from cache
			nread += size;
		}
		else {
            f->cache->start = f->cache->end;
			//read from file
            size = read(f->fd, f->cache->memory, cache_size);
			//if successfully read more than 0 chars
            if (size > 0) {
				//update the end of cache
                f->cache->end += size; 
			}
            else {
                return (ssize_t) nread ? (ssize_t) nread : (ssize_t) size;
			}
        }
	}
	return nread;
	//read sz characters
	//if (read(f->fd, buf, sz) == (int) sz)
		//return sz;
	//else
		//return -1;
}


// io61_writec(f)
//    Write a single character `ch` to `f`. Returns 0 on success or
//    -1 on error.

int io61_writec(io61_file* f, int ch) {
	if (f->mode != O_WRONLY)
        return -1;
	const char* buf = (const char*) &ch;
	// how much space is available in cache
	size_t available = cache_size - f->cache->size;
	if (!available) {
		//write the cache to the file.
        ssize_t nwritten = write(f->fd, f->cache->memory + f->cache->first, cache_size - f->cache->first);
		// If able to write
        if (nwritten >= 0) {
            f->cache->first += cleared;
            f->cache->size -= cleared;
			// Check if at the end of cache
            f->cache->first = (cache_size == f->cache->first) ? 0 : f->cache->first;
            f->cache->last = (cache_size == f->cache->last) ? 0 : f->cache->last;
        }
		//write not succcessful
		else
            return -1;   
    }
	// Write to cache
	 *(f->cache->memory + f->cache->last) = *buf;
    f->cache->last++;
    f->cache->size++;
    return 0;

    //buf[0] = ch;
   // if (write(f->fd, buf, 1) == 1)
       // return 0;
    //else
       // return -1;
}


// io61_write(f, buf, sz)
//    Write `sz` characters from `buf` to `f`. Returns the number of
//    characters written on success; normally this is `sz`. Returns -1 if
//    an error ohttps://github.com/cs61/cs61-f16-psets-team-mdccurred before any characters were written.

ssize_t io61_write(io61_file* f, const char* buf, size_t sz) {
	 if (f->mode != O_WRONLY)
        return -1;
	size_t nwritten = 0;
	size_t size;
	while (nwritten != sz) {
		size_t available = cache_size - f->cache->size;
		if (f->cache->previous_next != f->cache->next) {
			size_t cleared = write(f->fd, f->cache->first, f->cache->size);
			if (cleared >= 0) {
				if ((size_t) cleared != f->cache->size) {
					f->cache->previous_next = (f->cache->previous_next + cleared);
				}
				else {
					f->cache->previous_next = f->cache->next;
				}
				f->cache->first += cleared;
                f->cache->size -= cleared;
				f->cache->first = (cache_size == f->cache->first) ? 0 : f->cache->first;
                f->cache->last = (cache_size == f->cache->last) ? 0 : f->cache->last;
			}
			else {
                return (ssize_t) nwritten ? (ssize_t) nwritten : cleared;
			}
		}
		else if (!available) {
			ssize_t cleared = write(f->fd, f->cache->memory + f->cache->first, cache_size - f->cache->first);
			if (cleared >= 0) {
                f->cache->first += cleared;
                f->cache->size -= cleared;
                f->cache->first = (cache_size == f->cache->first) ? 0 : f->cache->first;
                f->cache->last = (cache_size == f->cache->last) ? 0 : f->cache->last;
            }
            else {
                return (size_t) nwritten ? (size_t) nwritten : (size_t) cleared;
			}
		}
		else {
			if (sz - nwritten < available) {
				size = sz - nwritten;
			} 
			else {
				size = available;
			}
            memcpy(f->cache->memory + f->cache->last, buf + nwritten, size);
            f->cache->last += size;
            f->cache->size += size;
            nwritten += size;
        }
	}
	return nwritten;
	//write sz characters
    //if (write(f->fd, buf, sz))
		//return sz;
   // else
       // return -1;
}


// io61_flush(f)
//    Forces a write of all buffered data written to `f`.
//    If `f` was opened read-only, io61_flush(f) may either drop all
//    data buffered for reading, or do nothing.

int io61_flush(io61_file* f) {
    // If f was opened read-only
    if (f->mode == O_RDONLY)
        return 0;
    // While the cache is not empty...
    while (f->cache->size) {
        size_t size = f->cache->first ? cache_size - f->cache->first : f->cache->last;
        //write the cache to the file.
        ssize_t cleared = write(f->fd, f->cache->memory + f->cache->first, size);
        // If able to write to file
        if (cleared > 0) {
            // update cache position and size
            f->cache->first += cleared;
            f->cache->size -= cleared;
        }
        // Else if nothing was written
        else if (cleared == 0) {
            f->cache->first = cleared;
        }
        // Else not able to write successfully       
        else
            return -1;
    }
    return 0;
}


// io61_seek(f, pos)
//    Change the file pointer for file `f` to `pos` bytes into the file.
//    Returns 0 on success and -1 on failure.

int io61_seek(io61_file* f, off_t pos) {
    off_t r = lseek(f->fd, (off_t) pos, SEEK_SET);
    if (r == (off_t) pos)
        return 0;
    else
        return -1;
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
