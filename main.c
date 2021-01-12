#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
// DMA control block (must be 32-byte aligned)
typedef struct {
    uint32_t ti,    // Transfer info
        srce_ad,    // Source address
        dest_ad,    // Destination address
        tfr_len,    // Transfer length
        stride,     // Transfer stride
        next_cb,    // Next control block
        debug,      // Debug register
        unused;
} DMA_CB __attribute__ ((aligned(32)));

#define DMA_CB_DEST_INC (1<<4)
#define DMA_CB_SRC_INC  (1<<8)
void *virt_dma_mem;
void *virt_gpio_regs, *virt_dma_regs, *virt_pwm_regs;
void *bus_dma_mem;
#define BUS_DMA_MEM(a)  ((uint32_t)a-(uint32_t)virt_dma_mem+(uint32_t)bus_dma_mem)
#define BUS_PHYS_ADDR(a) ((void *)((uint32_t)(a)&~0xC0000000))
#define VIRT_DMA_REG(a) ((volatile uint32_t *)((uint32_t)virt_dma_regs + a))
char *dma_regstrs[] = {"DMA CS", "CB_AD", "TI", "SRCE_AD", "DEST_AD",
    "TFR_LEN", "STRIDE", "NEXT_CB", "DEBUG", ""};
#define PAGE_SIZE       0x1000
// Round up to nearest page
#define PAGE_ROUNDUP(n) ((n)%PAGE_SIZE==0 ? (n) : ((n)+PAGE_SIZE)&~(PAGE_SIZE-1))
#define PHYS_REG_BASE    0x3F000000  // Pi 2 or 3
// Size of uncached memory for DMA control blocks and data
#define DMA_MEM_SIZE    PAGE_SIZE
#define DMA_CHAN        5
#define DMA_PWM_DREQ    5
#define DMA_BASE        (PHYS_REG_BASE + 0x007000)
#define DMA_CS          (DMA_CHAN*0x100)
#define DMA_CONBLK_AD   (DMA_CHAN*0x100 + 0x04)
#define DMA_TI          (DMA_CHAN*0x100 + 0x08)
#define DMA_SRCE_AD     (DMA_CHAN*0x100 + 0x0c)
#define DMA_DEST_AD     (DMA_CHAN*0x100 + 0x10)
#define DMA_TXFR_LEN    (DMA_CHAN*0x100 + 0x14)
#define DMA_STRIDE      (DMA_CHAN*0x100 + 0x18)
#define DMA_NEXTCONBK   (DMA_CHAN*0x100 + 0x1c)
#define DMA_DEBUG       (DMA_CHAN*0x100 + 0x20)
#define DMA_ENABLE      0xff0
#define FAIL(x) {printf(x); terminate(0);}

void enable_dma(void);
void start_dma(DMA_CB *cbp);
void stop_dma(void);
void *map_segment(void *addr, int size);
void unmap_segment(void *addr, int size);

// Free memory segments and exit
void terminate(int sig)
{
    printf("Closing\n");
    stop_dma();
    unmap_segment(virt_dma_mem, DMA_MEM_SIZE);
    unmap_segment(virt_dma_regs, PAGE_SIZE);
    exit(0);
}


// Enable and reset DMA
void enable_dma(void)
{
    *VIRT_DMA_REG(DMA_ENABLE) |= (1 << DMA_CHAN);
    *VIRT_DMA_REG(DMA_CS) = 1 << 31;
}

// Start DMA, given first control block
void start_dma(DMA_CB *cbp)
{
    *VIRT_DMA_REG(DMA_CONBLK_AD) = BUS_DMA_MEM(cbp);
    *VIRT_DMA_REG(DMA_CS) = 2;       // Clear 'end' flag
    *VIRT_DMA_REG(DMA_DEBUG) = 7;    // Clear error bits
    *VIRT_DMA_REG(DMA_CS) = 1;       // Start DMA
}

// Halt current DMA operation by resetting controller
void stop_dma(void)
{
    if (virt_dma_regs)
        *VIRT_DMA_REG(DMA_CS) = 1 << 31;
}

// Display DMA registers
void disp_dma(void)
{
    uint32_t *p=(uint32_t *)VIRT_DMA_REG(DMA_CS);
    int i=0;

    while (dma_regstrs[i][0])
    {
        printf("%-7s %08X ", dma_regstrs[i++], *p++);
        if (i%5==0 || dma_regstrs[i][0]==0)
            printf("\n");
    }
}

void *map_segment(void *addr, int size)
{
    int fd;
    void *mem;

    size = PAGE_ROUNDUP(size);
    if ((fd = open ("/dev/mem", O_RDWR|O_SYNC|O_CLOEXEC)) < 0)
        FAIL("Error: can't open /dev/mem, run using sudo\n");
    mem = mmap(0, size, PROT_WRITE|PROT_READ, MAP_SHARED, fd, (uint32_t)addr);
    close(fd);
#if DEBUG
    printf("Map %p -> %p\n", (void *)addr, mem);
#endif
    if (mem == MAP_FAILED)
        FAIL("Error: can't map memory\n");
    return(mem);
}
// Free mapped memory
void unmap_segment(void *mem, int size)
{
    if (mem)
        munmap(mem, PAGE_ROUNDUP(size));
}

// DMA memory-to-memory test
int dma_test_mem_transfer(void)
{
    DMA_CB *cbp = virt_dma_mem;
    char *srce = (char *)(cbp+1);
    char *dest = srce + 0x100;
 
    strcpy(srce, "memory transfer OK");
    memset(cbp, 0, sizeof(DMA_CB));
    cbp->ti = DMA_CB_SRC_INC | DMA_CB_DEST_INC;
    cbp->srce_ad = BUS_DMA_MEM(srce);
    cbp->dest_ad = BUS_DMA_MEM(dest);
    cbp->tfr_len = strlen(srce) + 1;
    start_dma(cbp);
    usleep(10);
#if DEBUG
    disp_dma();
#endif
    printf("DMA test: %s\n", dest[0] ? dest : "failed");
    return(dest[0] != 0);
}



int main()
{
    signal(SIGINT, terminate);
    virt_dma_regs = map_segment((void *)DMA_BASE, PAGE_SIZE);
    enable_dma();
    if ((virt_dma_mem = map_segment(BUS_PHYS_ADDR(bus_dma_mem), DMA_MEM_SIZE)) == 0)
            FAIL("Error: can't allocate uncached memory\n");

    return dma_test_mem_transfer();
}
