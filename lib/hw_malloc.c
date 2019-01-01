#include "hw_malloc.h"
#include <sys/mman.h>
#define header_size 24
#define mmap_threshold 32*1024

struct chunk_header* get_uchunk_header(struct chunk_header* add_pos)
{
    struct chunk_header* re_header = (struct chunk_header*)((void*)add_pos+add_pos->current_chunk_size);
    if((void*)re_header >= (void*)Heap->start_brk+(64*1024))
        re_header = (void*)re_header-(64*1024);
    return re_header;
}

struct chunk_header* get_lchunk_header(struct chunk_header* add_pos)
{
    struct chunk_header* re_header = (struct chunk_header*)((void*)add_pos-add_pos->prev_chunk_size);
    if((void*)re_header < (void*)Heap->start_brk)
        re_header = (void*)re_header+(64*1024);
    return re_header;
}

void renew_upper_chunk(struct chunk_header* entry)
{
    struct chunk_header* upper_chunk = get_uchunk_header(entry);
    upper_chunk->prev_chunk_size = entry->current_chunk_size;
    upper_chunk->allocated_flag = 0;
    upper_chunk->mmap_bit = 0;
}

void list_del(struct chunk_header* del)
{
    //printf("into list_del\n");
    del->next->prev = del->prev;
    del->prev->next = del->next;
    del->next = NULL;
    del->prev = NULL;
    //printf("end list_del\n");
}

int ppow(int f, int s)
{
    int result=2;
    for(int i=2; i<=s; i++) {
        result=result*2;
    }
    return result;
}
int get_bin_index(chunk_size_t chunk_size)
{
    int index;
    if(chunk_size >= 32768) {
        index=10;
        return index;
    } else if(chunk_size <= 32) {
        index=0;
        return index;
    } else {
        for(int i=5; i<=15; i++) {
            if(chunk_size == ppow(2,i)) {
                index=i-5;
                return index;
            } else if(chunk_size>ppow(2,i) && chunk_size<ppow(2,i+1)) {
                index=i-4;
                return index;
            }
        }
        return -1;
    }
}

void add_node(struct heap_t* Heap, struct chunk_header* entry)
{
    int bin_index = get_bin_index(entry->current_chunk_size);
    //printf("----------- %d\n",bin_index);
    renew_upper_chunk(entry);
    struct chunk_header* listptr;
    struct chunk_header* bin_head = Heap->BIN[bin_index];

    for(listptr = bin_head->next; ; listptr=listptr->next) {
        if(entry->current_chunk_size >= listptr->current_chunk_size) {
            struct chunk_header* new_lst = entry;
            struct chunk_header* pprev = listptr->prev;
            struct chunk_header* nnext = listptr;
            nnext->prev = new_lst;
            new_lst->next = nnext;
            new_lst->prev = pprev;
            pprev->next = new_lst;
            break;
        }
    }
}

/*-----------------only used in heap free------------------------------------------------*/

int is_allocated(struct chunk_header* entry)
{
    struct chunk_header* upper = get_uchunk_header(entry);
    return upper->allocated_flag;
}

void merge(struct chunk_header* upper, struct chunk_header* lower)
{
    list_del(upper);
    list_del(lower);
    renew_upper_chunk(upper);
    lower->current_chunk_size += upper->current_chunk_size;
    renew_upper_chunk(lower);
    add_node(Heap, lower);
}

void put_into_bin(struct chunk_header* free_chunk)
{
    renew_upper_chunk(free_chunk);
    add_node(Heap, free_chunk);
    struct chunk_header* upper = get_uchunk_header(free_chunk);
    struct chunk_header* upper2 = get_uchunk_header(upper);
    struct chunk_header* lower = get_lchunk_header(free_chunk);
    if(upper2->allocated_flag == 0 && upper > free_chunk)
        merge(upper, free_chunk);
    if(free_chunk->allocated_flag == 0 && lower < free_chunk)
        merge(free_chunk, lower);
}

/*-------------------------only used in allocation-------------------------------------*/

struct chunk_header* get_best_fit(struct heap_t* HEAP, size_t bytes)
{
    bytes += sizeof(struct chunk_header);
    int bin_index = get_bin_index(bytes);
    struct chunk_header* find = HEAP->BIN[bin_index]->next;

    for(bin_index=bin_index; bin_index<11; bin_index++) {
        while(HEAP->BIN[bin_index]->next != HEAP->BIN[bin_index]) {
            if(HEAP->BIN[bin_index]->next->current_chunk_size == bytes)
                return HEAP->BIN[bin_index]->next;
            else if(HEAP->BIN[bin_index]->next->current_chunk_size < bytes)
                break;
            else
                find = HEAP->BIN[bin_index]->next;
            HEAP->BIN[bin_index]->next = HEAP->BIN[bin_index]->next->next;
        }
    }
    //printf("to_use_size: %lld\n",find->current_chunk_size);
    //printf("bytes: %ld\n",bytes);
    if(find->current_chunk_size >= bytes) return find;
    else return NULL;
}

void chunk_header_init(struct chunk_header* entry, size_t bytes, chunk_size_t prev_chunk_size, chunk_flag_t allocated_flag, chunk_flag_t mmap_bit)
{
    entry->prev = NULL;
    entry->next = NULL;
    entry->current_chunk_size = bytes;
    entry->prev_chunk_size = prev_chunk_size;
    entry->allocated_flag = allocated_flag;
    entry->mmap_bit = mmap_bit;
}

void bin_init(struct chunk_header** bin)
{
    *bin = (struct chunk_header*)malloc(sizeof(struct chunk_header));
    (*bin)->prev = (*bin);
    (*bin)->next = (*bin);
    (*bin)->prev_chunk_size = sizeof(struct chunk_header);
    (*bin)->current_chunk_size = sizeof(struct chunk_header);
    (*bin)->allocated_flag = 0;
    (*bin)->mmap_bit = 0;
}

void heap_initial(struct heap_t** Heap)
{
    *Heap = (struct heap_t*)malloc(sizeof(struct heap_t));
    (*Heap) -> next = NULL;
    (*Heap) -> start_brk = sbrk(64*1024);
    for(int i=0; i<=10; i++) {
        bin_init((&(*Heap)->BIN[i]));
    }
    struct chunk_header* entry = (struct chunk_header*)(*Heap)->start_brk;
    chunk_header_init(entry, 64*1024, 1,0,0);
    add_node(*Heap, entry);
}

/*struct chunk_header* mmap_alloc_list(struct heap_t* Heap, struct chunk_header* entry)
{
    struct chunk_header* listptr;
    struct chunk_header* new_lst = entry;
    struct chunk_header* pprev = entry->prev;

    listptr->next=entry;
    pprev=listptr;
    listptr->prev=pprev;
    pprev->next=listptr;
    return new_lst;
}*/

void mmap_init(struct heap_t** Heap, size_t bytes)
{
    *Heap = (struct heap_t*)malloc(sizeof(struct heap_t));
    (*Heap) -> next = NULL;
    (*Heap) -> start_brk = sbrk(64*1024);
    struct chunk_header* entry = (struct chunk_header*)(*Heap)->start_brk;
    chunk_header_init(entry, 64*1024, 1,0,1);
    //mmap_alloc_list(*Heap,entry);
}

void split(struct chunk_header* org_chunk, size_t bytes)
{
    bytes += sizeof(struct chunk_header);
    struct chunk_header* rest_chunk = (struct chunk_header*)((void*)org_chunk+bytes);
    if(rest_chunk->current_chunk_size == (64*1024)) {
        chunk_header_init(rest_chunk,org_chunk->current_chunk_size-bytes,bytes,1,0);
        chunk_header_init(org_chunk,bytes,org_chunk->current_chunk_size-bytes,0,0);
    } else {
        chunk_header_init(rest_chunk,org_chunk->current_chunk_size-bytes,bytes,1,0);
        chunk_header_init(org_chunk,bytes,org_chunk->prev_chunk_size,org_chunk->allocated_flag,0);
        renew_upper_chunk(rest_chunk);
    }
    add_node(Heap, rest_chunk);
}

/*--------------------------function for user----------------------------------*/
char* mmap_list[100];
int* mmap_list_size[100];
int i=0;

void *hw_malloc(size_t bytes)
{
    //mmap allocation
    if(bytes+header_size > mmap_threshold) {
        //printf("into mmap allocation\n");
        if(Heap==NULL) {
            address = mmap(NULL,bytes+24,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
            mmap_init(&Heap,bytes);
            mmap_list[i]=(void*)address - get_start_sbrk();
            mmap_list_size[i]=bytes+header_size;
            i++;
            return (void*)address + header_size - get_start_sbrk();
        } else {
            address = mmap(NULL, bytes+24,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
            mmap_list[i]=(void*)address - get_start_sbrk();
            mmap_list_size[i]=bytes+header_size;
            i++;
            return (void*)address +header_size - get_start_sbrk();
        }
    }
    //heap allocation
    else {
        //printf("into heap allocation\n");
        if(Heap==NULL)	heap_initial(&Heap);
        struct chunk_header* found = get_best_fit(Heap,bytes);
        list_del(found);
        //printf("org_size: %lld\n",found->current_chunk_size);
        //printf("chunk_size: %ld\n",bytes+header_size);
        split(found, bytes);
        return (void*)found + header_size - get_start_sbrk();
    }
    return NULL;
}

int hw_free(void *mem)
{
    mem = mem + 24;
    mem = mem + (long long int)Heap->start_brk;
    struct chunk_header* free_chunk = (struct chunk_header*)mem-1;
    //mmap free method
    if(mem>=(void*)Heap->start_brk+(64*1024)) {
        //printf("into mmap free\n");
        mem = mem-(long long int)Heap->start_brk-24;
        munmap(mem,33792);
        return 1;
    }
    //heap free method
    else {
        //printf("into heap free\n");
        if(is_allocated(free_chunk)==1) {
            put_into_bin(free_chunk);
            return 1;
        }
        return 0;
    }
}

void *get_start_sbrk(void)
{
    return Heap->start_brk;
}

void print_bin(int index)
{
    while(Heap->BIN[index]->next != Heap->BIN[index]) {
        printf("0x%012lx--------%lld\n",((void*)Heap->BIN[index]->next - get_start_sbrk()),Heap->BIN[index]->next->current_chunk_size);
        Heap->BIN[index]->next = Heap->BIN[index]->next->next;
    }
}

void print_mmap_alloc_list()
{
    for(int ii=0; ii<i; ii++) {
        printf("0x%012lx--------%ld\n",mmap_list[ii],mmap_list_size[ii]);
    }
}

