#pragma once

#include <cstddef> // <-- Add this line

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        float r;
        float i;
    } kiss_fft_cpx;

    typedef struct kiss_fft_state* kiss_fft_cfg;

    kiss_fft_cfg kiss_fft_alloc(int nfft, int inverse_fft, void* mem, size_t* lenmem);
    void         kiss_fft(kiss_fft_cfg cfg, const kiss_fft_cpx* fin, kiss_fft_cpx* fout);

#ifdef __cplusplus
}
#endif
