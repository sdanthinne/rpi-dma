#include "rpi_dma_utils.h"

void *virt_dma_regs, *bus_dma_mem,*virt_dma_mem;
int mbox_fd, dma_mem_h;
void initdma()
{
    signal(SIGINT,terminate);
    virt_dma_regs = map_segment((void*)DMA_BASE,PAGE_SIZE);
    enable_dma();
    mbox_fd= open_mbox();
    if ((dma_mem_h = alloc_vc_mem(mbox_fd, DMA_MEM_SIZE, DMA_MEM_FLAGS)) <= 0 ||
        (bus_dma_mem = lock_vc_mem(mbox_fd, dma_mem_h)) == 0 ||
        (virt_dma_mem = map_segment(BUS_PHYS_ADDR(bus_dma_mem), DMA_MEM_SIZE)) == 0)
            FAIL("Error: can't allocate uncached memory\n");
    printf("VC mem handle %u, phys %p, virt %p\n", dma_mem_h, bus_dma_mem, virt_dma_mem);
}
/**
* this function copies memory from one area to another using the DMA engine
*   on the raspberry pi
 */
void * dmacp(void * origin, void * destination, uint32_t size)
{
    DMA_CB *cbp = virt_dma_mem;

    //char *srce = (char *)(cbp+1); currently this is not needed if 
    //it can be guaranteed that the source 
    //char *dest = srce + 0x100;

    //strcpy(srce, "memory transfer OK");
    memset(cbp, 0, sizeof(DMA_CB));
    cbp->ti = DMA_CB_SRC_INC | DMA_CB_DEST_INC;
    cbp->srce_ad = BUS_DMA_MEM(origin);
    cbp->dest_ad = BUS_DMA_MEM(destination);
    cbp->tfr_len = size;
    start_dma(cbp);
    usleep(10);
#if DEBUG
    disp_dma();
#endif
    //printf("DMA test: %s\n", dest[0] ? dest : "failed");
    return destination;
}
