#ifndef __CI_FFT_H
#define __CI_FFT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fft.h"

extern int ci_software_fft_init(riscv_rfft_fast_instance_f32 *S,int len);
extern int ci_software_fft(riscv_rfft_fast_instance_f32 *S,float *in, float *result );
extern int ci_software_ifft(riscv_rfft_fast_instance_f32 *S,float* fft, float* result);




extern riscv_rfft_fast_instance_f32 S;

#ifdef __cplusplus
}
#endif

#endif //__FFT_512_H
