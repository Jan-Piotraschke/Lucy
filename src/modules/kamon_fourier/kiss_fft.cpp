#include "kiss_fft.h"
#include <math.h>
#include <stdlib.h>

// Minimal “kiss_fft” from the official library

struct kiss_fft_state
{
    int    nfft;
    int    inverse;
    float* twiddle;
};

static void kf_bfly2(kiss_fft_cpx* Fout, int m, const kiss_fft_cfg st, int stride)
{
    kiss_fft_cpx* Fout2 = Fout + m * stride;
    for (int i = 0; i < m; i++)
    {
        float tr            = Fout2[i * stride].r;
        float ti            = Fout2[i * stride].i;
        Fout2[i * stride].r = Fout[i * stride].r - tr;
        Fout2[i * stride].i = Fout[i * stride].i - ti;
        Fout[i * stride].r += tr;
        Fout[i * stride].i += ti;
    }
}

static void kf_bfly(kiss_fft_cpx* Fout, int fstride, int m, int N, const kiss_fft_cfg st, int stage)
{
    // This is extremely simplified, only for power-of-2
    if (stage == 1)
    {
        kf_bfly2(Fout, N / 2, st, 1);
    }
    else
    {
        // in a real implementation, you'd handle more stages
        // We'll do a naive pass for demonstration only
        kf_bfly2(Fout, N / 2, st, 1);
    }
}

static void kf_work(
    kiss_fft_cpx*       Fout,
    const kiss_fft_cpx* f,
    int                 fstride,
    int                 in_stride,
    int*                factors,
    const kiss_fft_cfg  st,
    int                 N)
{
    // naive copy
    for (int i = 0; i < N; i++)
    {
        Fout[i].r = f[i * in_stride].r;
        Fout[i].i = f[i * in_stride].i;
    }
    // do a single butterfly pass for demonstration
    kf_bfly(Fout, fstride, N, N, st, 1);
}

kiss_fft_cfg kiss_fft_alloc(int nfft, int inverse_fft, void* mem, size_t* lenmem)
{
    kiss_fft_cfg st = (kiss_fft_cfg)malloc(sizeof(struct kiss_fft_state));
    st->nfft        = nfft;
    st->inverse     = inverse_fft;
    st->twiddle     = NULL; // not used in this minimal example
    return st;
}

void kiss_fft(kiss_fft_cfg cfg, const kiss_fft_cpx* fin, kiss_fft_cpx* fout)
{
    // This is a minimal stub that does *no actual real FFT*, just a single butterfly for
    // demonstration. For a real usage, use the full kiss_fft code from official repository or a
    // fully implemented version.
    int factors[2] = {0};
    kf_work(fout, fin, 1, 1, factors, cfg, cfg->nfft);

    // If inverse FFT, scale differently, etc.  (omitted here)
}
