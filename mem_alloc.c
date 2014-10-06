#include "mem_alloc.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

#define ALIGNMENT_CONSTANT 4
#define MINIMUM_SIZE 12

/* memory */
char memory[MEMORY_SIZE]; 

/* 
Structure declaration for free blocks
Size: 16 bytes
*/
typedef struct free_block{
  int size; 
  struct free_block *next; 
} free_block_s, *free_block_t;

/* 
Structure declaration for occupied blocks 
Size: 4 bytes
*/
typedef struct{
  int size; 
} busy_block_s, *busy_block_t; 


/* Pointer to the first free block in the memory */
free_block_t first_free; 


#define ULONG(x)((long unsigned int)(x))
#define max(x,y) (x>y?x:y)


/*
Initialize the list of free blocks with a single free block corresponding to the full array.
*/
void memory_init(void) {
	first_free = (free_block_t)((uintptr_t)memory);
	first_free->size = MEMORY_SIZE;
	first_free->next = NULL;
}

/*
This method allocates a block of size size. It returns a pointer to the allocated memory or NULL if the allocation failed.
*/
char *memory_alloc(int size) {
  free_block_t current, previous;

  if (size < MINIMUM_SIZE) {
    size = MINIMUM_SIZE;
  }

  /*
  // Memory alignment
  int reminder = size % ALIGNMENT_CONSTANT;
  if (reminder != 0) {    
    size = size + reminder;
  } 
  */

  // the size needed for both memory and header
	int real_size = size + sizeof(busy_block_s); 

  for (current = first_free; current != NULL; current = current->next) {
    // We found an empty block with enough space
    // First fit criteria
		if (current->size >= real_size) {         // TODO should be >= ???

      // TODO what's happening to the empty spaces??

      // Make sure that when we allocate something in a free block, the remaining empty space can hold at least another free block
      if (current->size - real_size < sizeof(free_block_s)) {
        // Use all the block
        size = current->size - sizeof(busy_block_s);
        real_size = size + sizeof(busy_block_s);
      }

      // It was the first free block
      if (current == first_free) {
        // If there's another free block after, just make it the first free
        if (first_free->next != NULL) {
          if(first_free->size - real_size >= sizeof(free_block_s)){

            int old_first_free_size = first_free->size;
            free_block_t old_next = first_free->next;

            first_free = (free_block_t)((uintptr_t)first_free + real_size);
            first_free->size = old_first_free_size - real_size; 
            first_free->next = old_next;

          }
          else first_free = first_free->next;
        } else {

          // TODO there may be busy blocks here, can't just move it like that

          // Check if there's enough space to move first_free                              
          if (((uintptr_t)first_free + real_size) >= ((uintptr_t)memory + MEMORY_SIZE)) {     // check out buddy allocation
            first_free = NULL;
          } else {
            // Enough space, just move first_free 
            int old_first_free_size = first_free->size;
            free_block_t old_next = first_free->next; //TODO this is always null ?!?

            first_free = (free_block_t)((uintptr_t)first_free + real_size);

            first_free->size = old_first_free_size - real_size; 
            first_free->next = old_next;
          }
        }
      }
      // Somewhere else
      else {
        // If we're using the whole block
        if (size == current->size - sizeof(busy_block_s)) {
          // Two scenarios here. If there's another free block after, link the previous to that one, if not, set next to NULL
          if (current->next != NULL) {
            previous->next = current->next;
          } else {
            // We ran out of space!
            previous->next = NULL;
          }
        } 
        // There's empty space. Create a new free block there
        else {
          int old_free_block_size = current->size;
          free_block_t new_free_block = (free_block_t)((uintptr_t)current + real_size);
          new_free_block->size = old_free_block_size - real_size;
          // And link it, as usual
          new_free_block->next = current->next;
          previous->next = new_free_block;
        }
      }

      // Create a new busy block
      busy_block_t new_busy_block = (busy_block_t)current;
      new_busy_block->size = size;

      char *allocated_memory = (char *)((uintptr_t)current + sizeof(busy_block_s));
      print_alloc_info(allocated_memory, size);
			             
      return allocated_memory;	    
		}

    previous = current;
	}

  return NULL;
}

/*
This method frees the zone adressed by zone. whose address is given receives an address to an occupied block. 
It updates the list of the free blocks and merge contiguous blocks.
*/
void memory_free(char *p) {

  print_free_info(p);

  // Find the busy block headear  
  busy_block_t occupied_block = (busy_block_t)((uintptr_t)p - sizeof(busy_block_s));
  int old_occupied_size = occupied_block->size;
  // printf("\noccupied block size: %d\n", occupied_block->size);  

  // Make it a free block
  free_block_t free_block = (free_block_t)((uintptr_t)occupied_block);
  free_block->size = (old_occupied_size + sizeof(busy_block_s));

  // If we ran out of free blocks
  if (first_free == NULL) {
    first_free = free_block;
    first_free->next = NULL;
  }
  // Update the list of free blocks
  else {
    // First free is located after free_block
    if ((uintptr_t)first_free > (uintptr_t)free_block) {
      free_block->next = first_free;      
      first_free = free_block;
    } 
    // There's at least one empty free block before free_block
    else {
      free_block_t current; 
      for (current = first_free; (uintptr_t)current < (uintptr_t)free_block; current = current->next) {
        // We need to find the closest free block before free_block
        if ((uintptr_t)current->next > (uintptr_t)free_block) {
          // just link free_block
          free_block->next = current->next;
          current->next = free_block;
        }
        else if (current->next == NULL) {
          current->next = free_block;
          free_block->next = NULL;
        }
      }
    }
  }

  // Merge contiguous blocks
  // Note. Another apporoach would be to not merge at all here but wait until there's not enough memory for an allocation and then try
  // to merge free blocks  
  int merged_blocks;
  do {
    merged_blocks = 0;
    free_block_t current;

    for (current = first_free; current != NULL; current = current->next) {
      if (current->next != NULL) {
        // Find two contiguous free blocks and merge them
        if ((uintptr_t)current->next == ((uintptr_t)current + current->size)) {
          current->size = current->size + current->next->size;
          current->next = current->next->next;
          // Start again - Unefficient but works for now
          merged_blocks = 1;
          current = first_free;
          break;
        }
      }
    }
  }
  while (merged_blocks);
}

void print_info(void) {
  fprintf(stderr, "Memory : [%lu %lu] (%lu bytes)\n", (long unsigned int) 0, (long unsigned int) (memory+MEMORY_SIZE), (long unsigned int) (MEMORY_SIZE));
  fprintf(stderr, "Free block : %lu bytes; busy block : %lu bytes.\n", ULONG(sizeof(free_block_s)), ULONG(sizeof(busy_block_s))); 
}

void print_free_info(char *addr){
  if(addr)
    fprintf(stderr, "FREE  at : %lu \n", ULONG(addr - memory)); 
  else
    fprintf(stderr, "FREE  at : %lu \n", ULONG(0)); 
}

void print_alloc_info(char *addr, int size){
  if(addr){
    fprintf(stderr, "ALLOC at : %lu (%d byte(s))\n", 
	    ULONG(addr - memory), size);
  }

  else{
    fprintf(stderr, "Warning, system is out of memory\n"); 
  }
}

void print_free_blocks(void) {
  free_block_t current; 
  fprintf(stderr, "Begin of free block list :\n"); 
  for(current = first_free; current != NULL; current = current->next)
    fprintf(stderr, "Free block at address %lu, size %u\n", ULONG((char*)current-memory ), current->size);
}

char *heap_base(void) {
  return memory;
}


void *malloc(size_t size){
  static int init_flag = 0; 
  if(!init_flag){
    init_flag = 1; 
    memory_init(); 
    //print_info(); 
  }      
  return (void*)memory_alloc((size_t)size); 
}

void free(void *p){
  if (p == NULL) return;
  memory_free((char*)p); 
  print_free_blocks();
}

void *realloc(void *ptr, size_t size){
  if(ptr == NULL)
    return memory_alloc(size); 
  busy_block_t bb = ((busy_block_t)ptr) - 1; 
  printf("Reallocating %d bytes to %d\n", bb->size - (int)sizeof(busy_block_s), (int)size); 
  if(size <= bb->size - sizeof(busy_block_s))
    return ptr; 

  char *new = memory_alloc(size);
 
  memcpy(new, (void*)(bb+1), bb->size - sizeof(busy_block_s) ); 
  memory_free((char*)(bb+1)); 
  return (void*)(new); 
}


#ifdef MAIN
int main(int argc, char **argv){

  /* The main can be changed, it is *not* involved in tests */
  memory_init();

  /*
  memory_alloc(20);
  char *b = memory_alloc(20);
  char *d = memory_alloc(10);
  char *c = memory_alloc(15);
  memory_alloc(20);

  memory_free(b);
  print_free_blocks();
  memory_free(c);
  print_free_blocks();
  memory_free(d);    
  print_free_blocks();
  */

  /*
  char *b = memory_alloc(30);
  printf("malloc returned: %lu\n", (uintptr_t)b);
  printf("first_free: %lu\n", (uintptr_t)first_free);
  if (b != NULL) {
    memory_free(b);
    printf("first_free after free: %lu\n", (uintptr_t)first_free);
    printf("first_free->size after free: %d\n", first_free->size);

    printf("second free: %lu\n", (uintptr_t)first_free->next);
    printf("second free size: %d\n", first_free->next->size); // TODO wrong size
  }
  
  printf("\n\n\n");

  int i ; 
  for (i = 0; i < 2; i++) {
    char *c = memory_alloc(rand()%8);
    printf("malloc b: %lu\n", (uintptr_t)c);
    printf("first_free: %lu\n", (uintptr_t)first_free);
    memory_free(c); 
    printf("first_free after free: %lu\n", (uintptr_t)first_free);
    // print_free_blocks();
  } 
  */



  /*
  char * a = memory_alloc(15);
  a=realloc(a, 20); 
  memory_free(a);


  a = memory_alloc(10);
  memory_free(a);

  printf("%lu\n",(long unsigned int) (memory_alloc(9)));
  */
  return EXIT_SUCCESS;
}
#endif 
  
