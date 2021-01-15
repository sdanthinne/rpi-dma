#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "testTotal.h"

void *virt_dma_regs, *bus_dma_mem,*virt_dma_mem;
int mbox_fd, dma_mem_h;
void initdma()
{
    signal(SIGINT,terminate);
}

void * no_cache_memory()
{
    virt_dma_regs = map_segment((void*)DMA_BASE,PAGE_SIZE);
    mbox_fd= open_mbox();
    if ((dma_mem_h = alloc_vc_mem(mbox_fd, DMA_MEM_SIZE, DMA_MEM_FLAGS)) <= 0 ||
        (bus_dma_mem = lock_vc_mem(mbox_fd, dma_mem_h)) == 0 ||
        (virt_dma_mem = map_segment(BUS_PHYS_ADDR(bus_dma_mem), DMA_MEM_SIZE)) == 0)
            FAIL("Error: can't allocate uncached memory\n");
    printf("VC mem handle %u, phys %p, virt %p\n", dma_mem_h, bus_dma_mem, virt_dma_mem);
    return virt_dma_mem;
}

/**
* this function copies memory from one area to another using the DMA engine
*   on the raspberry pi
 */
void * dmacp(void * origin, void * destination, int size)
{
    DMA_CB *cbp = virt_dma_mem;
    MEM_MAP map;
    //char *srce = (char *)(cbp+1); currently this is not needed if 
    //it can be guaranteed that the source 
    //char *dest = srce + 0x100;
    enable_dma(DMA_CHAN_A);
    //strcpy(srce, "memory transfer OK");
    memset(cbp, 0, sizeof(DMA_CB));
    map_uncached_mem(&map,size);
    cbp->ti = DMA_CB_SRC_INC | DMA_CB_DEST_INC;
    cbp->srce_ad = BUS_DMA_MEM(origin);
    cbp->dest_ad = BUS_DMA_MEM(destination);
    cbp->tfr_len = size;
    start_dma(&map,DMA_CHAN_A,cbp,0);
    usleep(10);
#if DEBUG
    disp_dma();
#endif
    //printf("DMA test: %s\n", dest[0] ? dest : "failed");
    return destination;
}
