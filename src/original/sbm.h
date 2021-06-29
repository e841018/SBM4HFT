#ifndef __SBM_H__
#define __SBM_H__

#include "global.h"

// #define K2000
// #define CUR_EXCH

extern "C"
{
    class MMTE
    {
    public:
        float J_sub[Nb][N];
        float h_sub[Nb];
        float x_sub[Nb];
        float p_sub[Nb];
        float Delta_t_times_gamma0;
        float alpha;
        float eta;
        void init(float J_sub[Nb][N], float h_sub[Nb], float x_sub_init[Nb], float p_sub_init[Nb], float Delta_t_times_gamma0);
        void step(int t, float x_prime_in[N], float x_prime_out[Nb]);
        void update_alpha_eta(int t);
        inline void MM(float x_prime_in[N], float Delta_P[Nb]);
        inline void TE();
        inline float FX(float x_sub_i, float h_sub_i);
        inline float FP(float p_sub_i);
    };
    void bottom(float J[N][N], float h[N], float x_init[N], float p_init[N], bool spin[N]);
    float sd(float J[N][N]);
    void SBM(float J[N][N], float h[N], MMTE blocks[Pb], bool spin[N]);
}

#ifdef K2000
void evalCost(float J[N][N], float h[N], bool spin[N], int &cut, int &cost);
#endif // K2000

#ifdef CUR_EXCH
#endif // CUR_EXCH

#endif
