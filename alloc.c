#define _DEFAULT_SOURCE

#include "alloc.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HEADER_SIZE (sizeof(struct header))

// keep track of the list of free blocks
struct header *list_of_free_chunks = NULL;

// allocation algo to tell alloc() how to allocate memory in the heap
enum algs current_allocation_algorithm = FIRST_FIT;

// set the size ceiling of the heap to the param @size
static int heap_total_size = 0;

// the current heap size to make sure it is under heap_total_size
static int current_heap_size = 0;

// initial program break before ANY alloc is called
static void *initial_program_break = NULL;

struct header *find_first_fit(struct header *list_of_free_chunks, int size) {
  while (list_of_free_chunks != NULL) {
    if (list_of_free_chunks->size >= (uint64_t)size) {
      return list_of_free_chunks;
    }
    list_of_free_chunks = list_of_free_chunks->next;
  }
  return NULL;
}

struct header *find_best_fit(struct header *list_of_free_chunks, int size) {
  struct header *current_best_chunk = NULL;
  struct header *cur = list_of_free_chunks;

  while (cur != NULL) {
    if (cur->size >= (uint64_t)size) {
      if (current_best_chunk == NULL ||
          cur->size < (uint64_t)current_best_chunk->size) {
        current_best_chunk = cur;
      }
    }
    cur = cur->next;
  }

  if (current_best_chunk != NULL) {
    return current_best_chunk;
  }

  return NULL;
}

struct header *find_worst_fit(struct header *list_of_free_chunks, int size) {
  struct header *current_best_chunk = NULL;
  struct header *cur = list_of_free_chunks;

  while (cur != NULL) {
    if (cur->size >= (uint64_t)size) {
      if (current_best_chunk == NULL ||
          cur->size > (uint64_t)current_best_chunk->size) {
        current_best_chunk = cur;
      }
    }
    cur = cur->next;
  }

  if (current_best_chunk != NULL) {
    return current_best_chunk;
  }

  return NULL;
}

void coalesce_free_chunks1() {}
void coalesce_free_chunks() {
  struct header *current = list_of_free_chunks;

  while (current != NULL && current->next != NULL) {
    // check if memory address of free blocks are adjacent
    if ((char *)current - current->size < (char *)current->next) {
      // merge adjacent free blocks
      current->size += current->next->size;
      current->next = current->next->next;
    } else { // no adjacent free block, go to next free block
      current = current->next;
    }
  }
}

void remove_chunk_selected_from_list(struct header **list_of_free_chunks,
                                     struct header *chunk_to_delete) {
  if (list_of_free_chunks == NULL || *list_of_free_chunks == NULL ||
      chunk_to_delete == NULL) {
    return;
  }

  struct header *current = *list_of_free_chunks;
  struct header *prev = NULL;

  while (current != NULL) {
    if (current == chunk_to_delete) {
      if (prev == NULL) {
        *list_of_free_chunks = current->next;

      } else {
        prev->next = current->next;
      }
      return;
    }
    prev = current;
    current = current->next;
  }
}
void *alloc(int size) {

  // for the very first call of alloc
  //  keep the current address of sbrk(0) so that when allocopt() is called in
  //  the future. Future calls of alloc will use the initial_program_break as if
  //  no allocs were used
  if (initial_program_break == NULL) {
    initial_program_break = sbrk(0);
  }

  // check for valid size
  if (size <= 0) {
    return NULL;
  }

  struct header *chunk_selected = NULL;

  int allocation_size = size + HEADER_SIZE;

  // find chunk to insert
  while (chunk_selected == NULL) {
    if (current_allocation_algorithm == FIRST_FIT) {
      chunk_selected = find_first_fit(list_of_free_chunks, allocation_size);
    } else if (current_allocation_algorithm == BEST_FIT) {
      chunk_selected = find_best_fit(list_of_free_chunks, allocation_size);
    } else if (current_allocation_algorithm == WORST_FIT) {
      chunk_selected = find_worst_fit(list_of_free_chunks, allocation_size);
    }
    void *heap_expanded = NULL;
    // no chunk found
    if (chunk_selected == NULL) {
      // expand heap
      if (current_heap_size + INCREMENT > heap_total_size) {
        return NULL;
      }

      heap_expanded = sbrk(INCREMENT);
      //  check for error from sbrk
      if (heap_expanded == (void *)-1) {
        return NULL;
      }
      // update current_heap_size
      current_heap_size += INCREMENT;

      // cast explanded heap memory to header
      struct header *new_chunk = (struct header *)heap_expanded;

      //  set new chunk size
      new_chunk->size = INCREMENT;
      //  add to list of frees
      new_chunk->next = list_of_free_chunks;
      list_of_free_chunks = new_chunk;
      coalesce_free_chunks();
    }
  }
  // found chunk that fits allocation size
  remove_chunk_selected_from_list(&list_of_free_chunks, chunk_selected);
  //// chunk found that fits allocation size
  // extra space so split into two: allocated and free
  if (chunk_selected->size > (uint64_t)allocation_size + HEADER_SIZE) {
    // split free
    struct header *left_over_free_chunk =
        (struct header *)((char *)chunk_selected + allocation_size);
    left_over_free_chunk->size = chunk_selected->size - allocation_size;
    left_over_free_chunk->next = list_of_free_chunks;
    list_of_free_chunks = left_over_free_chunk;

    chunk_selected->size = allocation_size;
    return (void *)((char *)chunk_selected + HEADER_SIZE);
  }

  return (void *)((char *)chunk_selected + HEADER_SIZE);
}

void dealloc(void *pointer) {
  if (pointer == NULL) {
    return;
  }

  struct header *chunk_to_free =
      (struct header *)((char *)pointer - HEADER_SIZE);

  chunk_to_free->next = list_of_free_chunks;
  list_of_free_chunks = chunk_to_free;

  coalesce_free_chunks();
}

void allocopt(enum algs algo, int size) {

  // select an allocation algorithm
  current_allocation_algorithm = algo;

  // set the size ceiling of the heap to the param @size
  heap_total_size = size;

  // clear the list of free chuncks
  list_of_free_chunks = NULL;

  current_heap_size = 0;

  // reset the program break
  // void *current_program_break = sbrk(0);
  // brk(current_program_break);
  //
  if (initial_program_break != NULL) {
    brk(initial_program_break);
  }
  return;
}

struct allocinfo allocinfo(void) {

  struct allocinfo info;
  info.free_size = 0;
  info.free_chunks = 0;
  info.largest_free_chunk_size = 0;
  info.smallest_free_chunk_size = 0;
  struct header *current_chunk = list_of_free_chunks;

  if (current_chunk != NULL) {
    info.smallest_free_chunk_size = current_chunk->size;
  }
  if (current_chunk != NULL && current_chunk->next == NULL) {
    // count total size of free blocks
    info.free_size += current_chunk->size - HEADER_SIZE;
    // count number of free chunks
    info.free_chunks++;
    // find largest chunk
    if (current_chunk->size - HEADER_SIZE >
        (uint64_t)info.largest_free_chunk_size) {
      info.largest_free_chunk_size = current_chunk->size - HEADER_SIZE;
    }

    // make sure smallest chunck size is accurate
    // if (current_chunk->size == 0) {
    //  info.smallest_free_chunk_size = current_chunk->size;
    //}
    // find smallest chunk
    if (current_chunk->size - HEADER_SIZE <
        (uint64_t)info.smallest_free_chunk_size) {
      info.smallest_free_chunk_size = current_chunk->size - HEADER_SIZE;
    }

  } else if (current_chunk != NULL) {
    // check each free block
    while (current_chunk != NULL) {
      // count total size of free blocks
      info.free_size += current_chunk->size;
      // count number of free chunks
      info.free_chunks++;
      // find largest chunk
      if (current_chunk->size - HEADER_SIZE >
          (uint64_t)info.largest_free_chunk_size) {
        info.largest_free_chunk_size = current_chunk->size - HEADER_SIZE;
      }

      // make sure smallest chunck size is accurate
      // if (current_chunk->size == 0) {
      //  info.smallest_free_chunk_size = current_chunk->size;
      //}
      // find smallest chunk
      if (current_chunk->size - HEADER_SIZE <
          (uint64_t)info.smallest_free_chunk_size) {
        info.smallest_free_chunk_size = current_chunk->size - HEADER_SIZE;
      }

      current_chunk = current_chunk->next;
    }
    info.free_size -= HEADER_SIZE;
  }
  // printf("total_size %d\n", total_size);

  // printf("free_size %d\n", info.free_size);
  // printf("free_chunks %d\n", info.free_chunks);
  // printf("largest %d\n", info.largest_free_chunk_size);
  // printf("smallest %d\n", info.smallest_free_chunk_size);
  //  return the allocinfo struct
  return info;
}
