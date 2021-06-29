#ifndef __GLOBAL_H__
#define __GLOBAL_H__

// ==================== constants for sbm ====================

/*
These parameters are set according to p.6 of [2].
The corresponding components in [1] are annotated in comments.
QUBO2Ising_3 is based on the objective function in [3].
[1] Combinatorial optimization by simulating adiabatic bifurcations in nonlinear Hamiltonian systems
[2] FPGA-based Simulated Bifurcation Machine
[3] A Currency Arbitrage Machine based on the Simulated Bifurcation Algorithm for Ultrafast Detection of Optimal Opportunity
*/

// problem size
const int N = 256;        // number of variables
const int N_step = 100; // number of steps
const int M = 2;        // number of substeps in a step

// degree of parallelism
const int Pb = 1;       // number of blocks
const int Nb = N / Pb;  // the number of spins to be handled in a block

// constants
const float alpha0 = 1.;    // Delta in [1]
const float beta0 = 1.;     // K in [1]
const float Delta_t = 0.7;
const float delta_t = Delta_t / M;

// ==================== constants for exch and sbm_tb_k2000 ====================

// constants for interface
const int n = 16;        // number of currencies
const int max_op = N;   // output buffer size for storing operations

// constants in objective function
const float M1 = 0.2;
const float M2 = 0.2;
const float m_c = 1100; // [3]

// initialization
const float x_init_max = 0.001;
const float p_init_max = x_init_max;

#endif
