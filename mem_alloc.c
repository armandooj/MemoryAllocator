#include "mem_alloc.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

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
	first_free = (void *)memory;
  printf("memory: %d\n", (int)first_free);
	first_free->size = MEMORY_SIZE - sizeof(free_block_s);
	first_free->next = NULL;
}

/*
This method allocates a block of size size. It returns a pointer to the allocated memory or NULL if the allocation failed.
*/
char *memory_alloc(int size) {
  // the size needed for both memory and header
	int real_size = size + sizeof(busy_block_s);  
	free_block_t current, previous;  

  // printf("busy_block size: %lu, real_size: %d\n", sizeof(busy_block_s), real_size);

  for (current = first_free; current != NULL; current = current->next) {
    // We found an empty block with enough space
    // First fit criteria
		if (current->size > real_size) {                    
        // It was the first free block
        if (current == first_free) {
          first_free = first_free + real_size; // We moove the pointer!
          first_free->size = first_free->size - real_size;  
        }
        // Somewhere else, link the previous to the next one 
        else {          
          previous->next = current->next; 
        }

        // Create a busy block
        busy_block_t new_busy_block = (busy_block_t)current;
        new_busy_block->size = (current->size + sizeof(free_block_s)) - sizeof(busy_block_s);
			             
       printf("current: %d\n", (int)current); // NOTE this looks weird, why? (maybe virtual address)
      return (char *)((int)current + sizeof(busy_block_s));	    
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
  // print_free_info(p); 


  /* ... */
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
  // printf("malloc: %d\n", (int)memory_alloc(1));

  int i ; 
  for( i = 0; i < 1; i++){
    char *b = memory_alloc(16);//memory_alloc(rand()%8);
    printf("b: %d\n", (int)b);
    memory_free(b); 
    print_free_blocks();
  }

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
