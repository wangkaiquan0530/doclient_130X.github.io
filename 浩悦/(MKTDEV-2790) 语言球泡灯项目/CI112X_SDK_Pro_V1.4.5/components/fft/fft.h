#ifndef __FFT_512_H
#define __FFT_512_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#define RISCVBITREVINDEXTABLE_256_TABLE_LENGTH ((uint16_t)440)
#define RISCVBITREVINDEXTABLE_512_TABLE_LENGTH ((uint16_t)448)

typedef struct
{
    uint16_t fftLen;                   /**< length of the FFT. */
	const float *pTwiddle;             /**< points to the Twiddle factor table. */
	const float *pTwiddle2;   
	const uint16_t *pBitRevTable;      /**< points to the bit reversal table. */
    uint16_t bitRevLength;             /**< bit reversal table length. */
} riscv_cfft_instance_f32;
  
typedef struct
{
    riscv_cfft_instance_f32 Sint;      /**< Internal CFFT structure. */
    uint16_t fftLenRFFT;             /**< length of the real sequence */
    const float * pTwiddleRFFT;        /**< Twiddle factors real stage  */
	const float * pTwiddleRFFT2; 
} riscv_rfft_fast_instance_f32 ;

//#include "riscv_math.h"

//extern riscv_rfft_fast_instance_f32 S;

extern int riscv_rfft_fast_init_f32( riscv_rfft_fast_instance_f32 * S,unsigned int fft_len );
extern void riscv_rfft_fast_f32(
  riscv_rfft_fast_instance_f32 * S,
  float * p,
  float * pOut,
  uint8_t ifftFlag)	;

#ifdef __cplusplus
}
#endif

#endif//__FFT_512_H
