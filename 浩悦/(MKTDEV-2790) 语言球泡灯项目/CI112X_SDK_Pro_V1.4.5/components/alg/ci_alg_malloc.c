/*
   2019.01.23, lt, Create.
***************************************************************************************/
#include "ci_alg_malloc.h"
#include "../printf/ci_log.h"
#if (USE_STANDARD_MALLOC || WIN32)
#   include <stdlib.h>
#endif


/* macros
***************************************************************************************/


/* structions
***************************************************************************************/


/* global vars. definitions
***************************************************************************************/


/* function definitions
***************************************************************************************/


/****************************************************************************
* read the *.h file please
****************************************************************************/

static uint32_t alg_calloc_size = 0;
uint32_t get_alg_calloc_size(void)
{
    return alg_calloc_size;
}

void* ci_algbuf_calloc( size_t size_in_byte ,size_t size_of_byte)
{
    void *addr;
    alg_calloc_size += size_in_byte*size_of_byte;
    //mprintf("alg_calloc_size = %d\n",alg_calloc_size);
    addr = calloc( size_in_byte, size_of_byte);
    if(NULL == addr)
    {
        ci_logerr(LOG_USER,"alg calloc err:%d\n",alg_calloc_size);
        while(1);
    }
    else 
    {
        return addr;
    }

}


static uint32_t alg_malloc_size = 0;
uint32_t get_alg_malloc_size(void)
{
    return alg_malloc_size;
}
#if 1
void* ci_algbuf_malloc( size_t size_in_byte )
{
    void *addr;
    alg_malloc_size += size_in_byte;
    
#if (USE_STANDARD_MALLOC || WIN32)
    addr = malloc( size_in_byte );
    if(NULL == addr)
    {
        ci_logerr(LOG_USER,"alg malloc err:%d\n",alg_malloc_size);
        while(1);
    }
    else 
    {
        return addr;
    }
#else
    return (void*)pvPortMalloc( size_in_byte );
#endif
}


/****************************************************************************
* read the *.h file please
****************************************************************************/
void ci_algbuf_free( void *buf )
{
    ci_loginfo(LOG_USER,"denoise free\n");
#if (USE_STANDARD_MALLOC || WIN32)
    free( buf );
#else
    vPortFree( buf );
#endif
}

#else // for check the memory info
void* ci_algbuf_malloc( size_t size )
{
    unsigned char *p = NULL;
    if(0 == size)
    {
        while(1);
    }
    
    //p = malloc(()size+12);
    p = (unsigned char*)pvPortMalloc(size+12);
    if (p)
    {
        mprintf("#M 0x%x\n",(p));
        *(unsigned int*)p = 0xAAAACDEF;
        *(unsigned int*)(p+4) = (unsigned int)size;
        *(unsigned int*)(p+8+size) = 0x5555CDEF;
        mprintf("#M 0x%x\n",(p));
        mprintf("#S 0x%d\n",size);
        return (void*)(p + 8);
    }
    else
    {
        while(1);
    }
}

/****************************************************************************
* read the *.h file please
****************************************************************************/
void ci_algbuf_free( void *pp )
{
    unsigned char *p = (unsigned char *)pp - 8;
    unsigned int size = *(unsigned int*)(p+4);
    //mprintf("#F 0x%x\n",p);
    if (*(unsigned int*)p != 0xAAAACDEF || *(unsigned int*)(p+8+size) != 0x5555CDEF)
    {
        mprintf("dheap overflow p:0x%x, size:%0x e:%0x \n",p,*(unsigned int*)(p+4),*(unsigned int*)(p+8+size));
        while(1);
    }
    else
    {
        //free(p);
        vPortFree(p);
    }
}
#endif
