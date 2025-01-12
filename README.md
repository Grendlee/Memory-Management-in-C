# Memory-Management-in-C

There are two main functions in `alloc.c`.

```c
/*
 * alloc() allocates memory from the heap. The first argument indicates the
 * size. It returns the pointer to the newly-allocated memory. It returns NULL
 * if there is not enough space.
 */
void *alloc(int);

/*
 * dealloc() frees the memory pointed to by the first argument.
 */
void dealloc(void *);
```

They correspond to `malloc()` and `free()`. These two functions that
allocate and deallocate memory for a user program.

In addition, there are two other functions that you need to implement that sets the parameters for
your allocator and gathers the statistics.

```c
/*
 * allocopt() sets the options for the memory allocator.
 *
 * The first argument sets the algorithm. The second argument sets the size
 * limit.
 */
void allocopt(enum algs, int);

/*
 * allocinfo() returns the current statistics.
 */
struct allocinfo allocinfo(void);
```

These functions are inspired by `mallopt()` and `mallinfo()` for `malloc()` that allow you to set
the parameters for `malloc()` and gather statistics.

## `alloc()`

A user program invokes `alloc()` to request a chunk of memory. It expects a single argument that
indicates the memory block size being requested. It returns the pointer to a newly-allocated
chunk. This is basically `malloc()`.

* Internally,  `alloc()` uses `sbrk()` (which increases the program break) to increase the size of
  the heap. 


## `dealloc()`

A user program frees a previously-allocated block with `dealloc()`. It expects a single argument
that points to the block to be freed. This is basically `free()`.

