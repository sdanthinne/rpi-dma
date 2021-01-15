#ifndef TESTTOTAL_H
#define TESTTOTAL_H
#include <stdint.h>
#include <stddef.h>
typedef struct {
uint32_t ti,
         src_ad,
         dest_ad,
         tfr_len,
         stride,
         next_cb,
         debug,
         unused;
} DMA_CB __attribute__ ((aligned(32)));

void enable_dma(void);
void start_dma(DMA_CB *cbp);
void stop_dma(void);
void * dmacp(void * origin, void * destination, size_t size);

void *videocore_allocator(void);
void *no_cache_memory(void);


#endif
