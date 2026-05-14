#include <stdlib.h>
#include <stdint.h>

#include "pool.h"

#define BLOCK_SIZE 64U  //块大小
#define PAGE_SIZE 4096U    //页大小
#define MEM_POOL_ALIGN_SIZE 4U

//小块空闲链表用于复用内存
struct mem_block {
    struct mem_block* next; //单向链表，串联所有空闲块
};

//内存池大页一次性申请的内存
struct mem_page {
    struct mem_page* next;   //指向下一个页
    void* raw_ptr;           //原始malloc地址
    uint8_t* start;          //页起始地址
    uint8_t* end;            //页结束地址
    size_t free;             //当前页剩余可用大小
};

//内存池核心控制结构体
struct mem_pool{
    mem_page_t* first_page;  //分配的第一页
    mem_page_t* curr_page;   //正在分配的内存页
    mem_block_t* free_list;  //空闲小块内存链表
    size_t page_size;        //单个内存页大小
    uint32_t align;          //内存对齐字节数
};

//切割内存块
static void mem_page_cut(mem_pool_t* pool, mem_page_t* page) {
    //拿到第一个块的头部
    mem_block_t* curr = (mem_block_t*)page -> start;
    while (page -> free >= BLOCK_SIZE) {
        mem_block_t* next_block = (mem_block_t*)((char*)curr + BLOCK_SIZE);
        curr -> next = next_block;
        page -> free -= BLOCK_SIZE;
        curr = next_block;
    }
    curr -> next = NULL;
}

//创建页
static struct mem_page* mem_page_create(const mem_pool_t *pool) {
    mem_page_t* page = (mem_page_t*)malloc(sizeof(mem_page_t));
    if (page == NULL)
        {return NULL;}
    void* raw_ptr = malloc(PAGE_SIZE);
    if (raw_ptr == NULL) {
        free(page);
        return NULL;
    }
    const uintptr_t raw_addr = (uintptr_t)raw_ptr;
    const uintptr_t aligned_addr = raw_addr + (pool -> align -  \
                        (raw_addr % pool -> align)) % pool -> align;
    page -> raw_ptr = raw_ptr;
    page -> start = (uint8_t*)aligned_addr;
    page ->end = (uint8_t*)raw_ptr + PAGE_SIZE;
    page -> free = page ->end - page ->start;
    page -> next = NULL;
    mem_page_cut(pool, page);
    return page;
}

//初始化内存池
void *mem_pool_init(void) {
    mem_pool_t* pool = (mem_pool_t*)malloc(sizeof(mem_pool_t));
    if (pool == NULL)
        {return NULL;}
    pool -> align = MEM_POOL_ALIGN_SIZE;
    pool -> page_size = PAGE_SIZE;
    mem_page_t* page = mem_page_create(pool);
    if (page == NULL) {
        free(pool);
        return NULL;
    }
    pool -> first_page = page;
    pool -> curr_page = page;
    pool -> free_list = (mem_block_t*)pool -> curr_page -> start;
    return pool;
}

//释放内存块
void mem_pool_free(mem_pool_t* pool, void *ptr) {
    if (pool == NULL || ptr == NULL)
        {return;}
    mem_block_t* block = (mem_block_t*)ptr;
    block -> next = pool -> free_list;
    pool -> free_list = block;
}

//分配块
void *mem_pool_alloc(mem_pool_t *pool) {
    if (pool == NULL)
        {return NULL;}
    if (pool ->free_list == NULL || pool -> curr_page \
                            -> free <= 2 * BLOCK_SIZE) {
        mem_page_t* page = mem_page_create(pool);
        if (page == NULL)
            {return NULL;}
        pool -> curr_page ->next = page;
        pool -> curr_page = page;
//直接覆盖free_list，保留余下的一两个块时间开销大（只会用循环嵌套解决）
        pool -> free_list = (mem_block_t*)page -> start;
    }
    mem_block_t* curr_block = pool -> free_list;
    pool -> free_list = pool -> free_list -> next;
    return (void*)curr_block;
}

//释放内存池
void mem_pool_destroy(mem_pool_t* pool) {
    if (pool == NULL)
        {return;}
    mem_page_t* first_page = pool -> first_page;
    while (first_page != NULL) {
        mem_page_t* next = first_page -> next;
        free(first_page -> raw_ptr);
        free(first_page);
        first_page = next;
    }
    free(pool);
}