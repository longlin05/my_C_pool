//
// Created by 祖龙 on 2026/5/13.
//

#ifndef WEEK2DEMO_POOL_H
#define WEEK2DEMO_POOL_H


typedef struct mem_block mem_block_t;
typedef struct mem_page mem_page_t;
typedef struct mem_pool mem_pool_t;

void *mem_pool_init(void);
void mem_pool_destroy(mem_pool_t *pool);
void* mem_pool_alloc(mem_pool_t *pool);
void mem_pool_free(mem_pool_t *pool, void *ptr);

#endif //WEEK2DEMO_POOL_H
