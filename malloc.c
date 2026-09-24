#include <stddef.h>
#include <unistd.h>
#include <sys/mman.h>

struct block_meta {
    size_t size;
    int free;                  //check if block of memory is freed yet
    int is_mmap;               //for whether to use sbrk or mmap
    struct block_meta* next;    //for the free list/linked list of memory blocks  
};

struct block_meta *head = NULL;
struct block_meta *tail = NULL;

size_t META_SIZE = sizeof(struct block_meta);
size_t MMAP_LIMIT = (128 * 1024);

struct block_meta*  find_free_block(struct block_meta *head, size_t size) {
    struct block_meta * current = head;
    while (current != NULL) {
        if (current->free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void * my_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    size = (size + sizeof(void*) - 1) & ~(sizeof(void*) - 1); // because some cpu architectures want size up to a certain multiple for allocating in chunk sizes of words.

    struct block_meta * block;

    if (size >= MMAP_LIMIT) {
        

        void *ptr = mmap(NULL, size + META_SIZE, 
                  PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (ptr == MAP_FAILED) {
            return NULL;
        }

        block = (struct block_meta *)ptr;

        block->size = size;
        block->free = 0;
        block->is_mmap = 1;
        block->next = NULL;
        return block + 1;
    } else {


        block = find_free_block(head, size);

        if (block == NULL) {
            void *ptr = sbrk(size + META_SIZE);
            if (ptr == (void *)-1) {
                return NULL;
            }
            block = (struct block_meta *)ptr;
            block->size = size;
            block->free = 0;
            block->is_mmap = 0;
            block->next = NULL;
            
            if (tail) {
                tail->next = block;
            } else {
                head = block;
            }

            tail = block;
            

        } else {
            block->free = 0;
            return block + 1;
        }
        
    }
    return block + 1;
}

void my_free(void* ptr) {

    if (ptr == NULL) {
        return;
    }

    struct block_meta *block = (struct block_meta*)ptr - 1;

    if (block->is_mmap) {
        munmap(block, block->size + META_SIZE);
        return;
    }

    block->free = 1;

}

int main(void) {
    void *a = my_malloc(100);
    void *b = my_malloc(100);
    my_free(a);
    void *c = my_malloc(100);
    // c should equal a (reused block), and b should differ from both
}