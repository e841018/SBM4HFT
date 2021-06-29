#ifndef __EXCH_H__
#define __EXCH_H__

#include "mm_datatype.h"
#include "global.h"

#define Q2I3 // Convert the QUBO formulation into an Ising model, with object function from [3]

extern "C"
{
    void exch(orderBookResponse_t* update, orderEntryOperation_t operations[max_op], float x_init[N], float p_init[N]);
    void QUBO2Ising(float xrate[n][n], float J[N][N], float h[N]);
}
#endif
