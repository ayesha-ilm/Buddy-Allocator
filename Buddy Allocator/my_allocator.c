#include <stdlib.h>
#include "my_allocator.h"
#include <stdio.h>
#include <math.h>
#include <stdint.h> //For uintptr_t

typedef void * Addr;

struct node 
{
    Addr data; 
    struct node *next;
    struct node *prev;
};

Addr buffer_start;
struct node *arr_ptr = NULL;
int arr_length;
unsigned int bblock = 0;
unsigned int total_mem = 0;

//Helper function to compute integer log2
unsigned int int_log2(unsigned int x) 
{
    unsigned int res = 0;
    while (x >>= 1) res++;
    return res;
}

//helper function to compute the next power of two
unsigned int next_power_of_two(unsigned int x) 
{
    if (x == 0) return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
    return x;
}

//function to flip the buddy bit
Addr buddy_flip(Addr buddy_of, unsigned int size) 
{
    uintptr_t offset = (uintptr_t)buddy_of - (uintptr_t)buffer_start;  
    unsigned int bitflip = int_log2(size);
    uintptr_t buddy_virtual = offset ^ (1 << (bitflip - 1));
    Addr buddy_addr = (Addr)(buffer_start + buddy_virtual);
    return buddy_addr;
}

//initialize the allocator
unsigned int init_allocator(unsigned int _basic_block_size, unsigned int _length)
{
    bblock = _basic_block_size;
    unsigned int twos = next_power_of_two(_length);
    total_mem = twos; 
    buffer_start = malloc(total_mem); 
    if (buffer_start == NULL) {
        fprintf(stderr, "Failed to allocate memory.\n");
        exit(EXIT_FAILURE);
    }

    arr_length = int_log2(twos) - int_log2(bblock) + 1; 
    arr_ptr = (struct node*)malloc(arr_length * sizeof(struct node));
    if (arr_ptr == NULL) {
        fprintf(stderr, "Failed to allocate memory for arr_ptr.\n");
        free(buffer_start);
        exit(EXIT_FAILURE);
    }

    //initialize all nodes in arr_ptr
    for (int i = 0; i < arr_length; i++) 
    {
        arr_ptr[i].data = NULL; //Not used as head
        arr_ptr[i].next = NULL;
        arr_ptr[i].prev = NULL;
    }

    //add the initial large block to the highest-order free list
    struct node* initial_node = (struct node*) malloc(sizeof(struct node));
    if (initial_node == NULL) {
        fprintf(stderr, "Failed to allocate initial_node.\n");
        free(arr_ptr);
        free(buffer_start);
        exit(EXIT_FAILURE);
    }
    initial_node->data = buffer_start;
    initial_node->next = NULL;
    initial_node->prev = NULL;

    arr_ptr[arr_length - 1].next = initial_node; //link to free list

    return 0;
}

int release_allocator()
{
    //free all nodes in each linked list
    for (int i = 0; i < arr_length; i++) 
    {
        struct node* current = arr_ptr[i].next;
        while (current != NULL) 
        {
            struct node* temp = current;
            current = current->next;
            free(temp);
        }
    }

    free(arr_ptr);
    free(buffer_start);
    return 0;
}

//add a block to the free list
void add_block(Addr sentin, unsigned int size) 
{
    int arr_idx = int_log2(size) - int_log2(bblock);
    if (arr_idx < 0 || arr_idx >= arr_length) {
        fprintf(stderr, "add_block: Invalid array index %d for size %u.\n", arr_idx, size);
        return;
    }

    struct node* new_node = (struct node*) malloc(sizeof(struct node));
    if (new_node == NULL) {
        fprintf(stderr, "Failed to allocate memory for new_node.\n");
        exit(EXIT_FAILURE);
    }
    new_node->data = sentin;
    new_node->next = NULL;
    new_node->prev = NULL;

    struct node* current = &arr_ptr[arr_idx];
    while (current->next != NULL)
    {
        current = current->next; 
    }
    current->next = new_node;
    new_node->prev = current;
}

//remove a block from the free list
void remove_block(Addr sentin, unsigned int size) 
{
    int arr_idx = int_log2(size) - int_log2(bblock);
    if (arr_idx < 0 || arr_idx >= arr_length) {
        fprintf(stderr, "remove_block: Invalid array index %d for size %u.\n", arr_idx, size);
        return;
    }

    struct node* current = arr_ptr[arr_idx].next;
    struct node* previous = &arr_ptr[arr_idx];

    while(current != NULL) 
    {   
        if(current->data == sentin) 
        {   
            //unlink the node
            previous->next = current->next; 
            if (current->next != NULL)
            {
                current->next->prev = previous;
            }
            free(current);
            return;
        }
        previous = current;
        current = current->next;
    }
    //if not found, do nothing
}

//find a free block of at least the given size
Addr find_free_block_in_size(unsigned int size) 
{
    int arr_idx = int_log2(size) - int_log2(bblock);
    if (arr_idx < 0 || arr_idx >= arr_length) {
        return NULL;
    }

    struct node* current = arr_ptr[arr_idx].next;
    if (current == NULL) 
    { 
        return NULL;
    } 
    return current->data;
}

//split a block until the desired size is reached
Addr split_block(unsigned int desired_size)
{
    unsigned int current_size = next_power_of_two(desired_size);
    unsigned int arr_idx = int_log2(current_size) - int_log2(bblock);
    Addr block = find_free_block_in_size(current_size);

    if (block == NULL) 
    {
        //search for larger blocks
        unsigned int size = current_size * 2;
        while (size <= total_mem) 
        { 
            block = find_free_block_in_size(size);
            if (block != NULL) 
            {
                break;
            }
            size *= 2;
        }

        if (block == NULL)
        {
            //No suitable block found
            return NULL;
        }

        //split blocks until the desired size is reached
        while (size > current_size)
        {
            remove_block(block, size);
            size /= 2;
            Addr buddy = (Addr)((uintptr_t)block + size);
            add_block(buddy, size);
            add_block(block, size);
        }
    }
    else
    {
        remove_block(block, current_size);
    }

    return block;
}

//join buddies if possible
int buddy_join(Addr strt, struct node *ptr, int arr_idx, unsigned int size)
{
    Addr buddy = buddy_flip(strt, size);
    struct node* current = ptr->next;

    while (current != NULL)
    {
        if (current->data == buddy)
        {
            //buddy found, remove it from free list
            remove_block(current->data, size);
            //no need to free here since remove_block already frees the node

            //determine the lower address
            Addr merged = (strt < buddy) ? strt : buddy;
            add_block(merged, size * 2);
            return 1;
        }
        current = current->next;
    }
    return 0;
}

//coalesce free blocks
void coalesce(Addr strt, unsigned int size)
{
    unsigned int current_size = size;
    Addr current_addr = strt;

    while (current_size < total_mem)
    {
        int arr_idx = int_log2(current_size) - int_log2(bblock);
        if (arr_idx < 0 || arr_idx >= arr_length -1) {
            break;
        }

        if (buddy_join(current_addr, &arr_ptr[arr_idx], arr_idx, current_size))
        {
            //successfully merged, move up to the next level
            current_size *= 2;
            current_addr = buddy_flip(current_addr, current_size / 2);
        }
        else
        {
            //cannot merge further
            break;
        }
    }
}

//allocate memory
Addr my_malloc(unsigned int _length) 
{
    if (_length == 0) return NULL;

    unsigned int l_w_header = _length + sizeof(unsigned int); //Use sizeof(unsigned int) for header
    unsigned int new_ceil = next_power_of_two(l_w_header);
    
    Addr place_here = split_block(new_ceil); 
    if (place_here != NULL)
    {  
        //write the size to the header
        *((unsigned int*)place_here) = new_ceil; 
        //return the memory after the header
        return (Addr)((unsigned char*)place_here + sizeof(unsigned int)); 
    }
    return NULL;
}

//free allocated memory
int my_free(Addr _a) 
{
    if (_a == NULL) return -1;

    Addr header = (Addr)((unsigned char*)_a - sizeof(unsigned int));
    unsigned int size = *((unsigned int*)header);

    //add the block back to the free list and coalesce
    add_block(header, size);
    coalesce(header, size);
    
    printf("Freed block at %p with size %u\n", header, size);
    return 0;
}

//function to print the free lists (for debugging)
void print_free_lists()
{
    for (int i = 0; i < arr_length; i++) 
    {
        unsigned int block_size = bblock * (1 << i);
        printf("Block size %u: ", block_size);
        struct node* current = arr_ptr[i].next;
        while (current != NULL)
        {
            printf("%p -> ", current->data);
            current = current->next;
        }
        printf("NULL\n");
    }
}

int main()
{
    init_allocator(4, 64);
    Addr ptr1 = my_malloc(2);
    Addr ptr2 = my_malloc(8);
    Addr ptr3 = my_malloc(16);

    printf("free lists after allocations: \n");
    print_free_lists();

    if (ptr1 != NULL)
    {
        my_free(ptr1); 
    }
    if (ptr2 != NULL)
    {
        my_free(ptr2); 
    }
    if (ptr3 != NULL)
    {
        my_free(ptr3); 
    }

    printf("\nfree lists after deallocations: \n");
    print_free_lists();
    release_allocator();
    return 0;
}
