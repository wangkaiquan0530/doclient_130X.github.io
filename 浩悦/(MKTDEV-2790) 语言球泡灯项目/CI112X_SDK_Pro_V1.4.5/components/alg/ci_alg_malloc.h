#ifndef CI_ALG_MALLOC_H
#define CI_ALG_MALLOC_H

#include <stdio.h>
#include <stdint.h>


#define USE_STANDARD_MALLOC 1

/*
 * Readme first:
 */


#ifdef __cplusplus
extern "C" {
#endif

    
    /****************************************************************************
     * NAME:     ci_algbuf_malloc
     * FUNCTION: same as malloc()
     * NOTE:     None
     *           
     * ARGS:     size_in_byte: the buffer length in byte
     * RETURN:   The pointer of buffer, NULL if error, else ok
     ****************************************************************************/
    void* ci_algbuf_malloc( size_t size_in_byte );
    void* ci_algbuf_calloc( size_t size_in_byte ,size_t size_of_byte);

    /****************************************************************************
     * NAME:     ci_algbuf_free
     * FUNCTION: same as free()
     * NOTE:     None
     *           
     * ARGS:     buf: the pointer of buffer ( from the return value of ci_algbuf_malloc() )
     * RETURN:   None
     ****************************************************************************/
    void ci_algbuf_free( void* buf );
    
    uint32_t get_alg_malloc_size(void);
    
    uint32_t get_alg_calloc_size(void);
    
#ifdef __cplusplus
}
#endif


#endif /* CI_DATA_IO_H */
