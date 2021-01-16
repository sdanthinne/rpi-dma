/*
 * simple DMA copy example
 * Using Jeremy Bentham's rpi_dma_utils library
 * Erin Clark and Sebastien Danthinne
 */
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "rpi_dma_utils.h"
#include <string.h>
#include <stdio.h>
#include <sys/io.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DMA_CHAN_USED 5
/*
 * this inserts some string into the uncached memory 
 *  this function is necessary because the DMA engine 
 *  NEEDS bus addresses to "see" the memory and thus 
 *  the data must already be somewhere where we know 
 *  the bus addresses, which is true for the 
 *  videocore-allocated memory 
 */
void copyToUncached(char * string, void * the_memory)
{
    strcpy((char *)the_memory,string);
}

int main()
{
    MEM_MAP memory;
    MEM_MAP * mp = &memory;
    DMA_CB * controlblock;
    char * source;
    char * mystring = "copied";
    void * memoryp = map_uncached_mem(&memory,10000);//allocate one page
    init_dma();
    if(memoryp==0)
    {
        printf("memory allocation failed\n");
    }
    enable_dma(DMA_CHAN_USED);
    //while(!dma_active(DMA_CHAN_USED))
    //{
    //   printf("DMA not active! %d\n",dma_transfer_len(DMA_CHAN_USED));
    //}
    //populate control block
    
    memset(memoryp,0,sizeof(controlblock));
    controlblock = memoryp;
    copyToUncached(mystring,controlblock+1);
    controlblock->srce_ad = MEM_BUS_ADDR(mp,controlblock+1);
    controlblock->dest_ad = MEM_BUS_ADDR(mp,controlblock+50);
    controlblock->tfr_len = sizeof(mystring);
    start_dma(&memory,DMA_CHAN_USED,controlblock,0);
    while(dma_active(DMA_CHAN_USED));//wait for the dma to be done
    printf("source address: %p\ndestination address:%p\n source contents: %s\ndestination contents:%s\n",
            controlblock+1,
            controlblock+50,(char *)(controlblock+1),(char *)(controlblock+50));
    stop_dma(DMA_CHAN_USED);
    unmap_periph_mem(&memory);
    return 0;
}
