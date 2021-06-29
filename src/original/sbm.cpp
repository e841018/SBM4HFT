#include "sbm.h"
#include <math.h> // sqrt

// #define DEBUG_PRINT
#define LOG_X_P

#ifdef DEBUG_PRINT
#include <iostream>
#include <iomanip>

#ifdef LOG_X_P
#include <fstream>
std::ofstream log_file;
#endif

void print_J(float J[N][N]) {
    std::cout << '\n';

    std::cout << "J =          (";
    for (int j = 0; j < N; j++)
        // std::cout << std::setw(2) << j / n << ',' << std::setw(2) << j % n << ") (";
        std::cout << std::setw(2) << j << ") (";
    std::cout << "\b \n";

    std::cout << "           +-";
    for (int j = 0; j < N; j++)
        std::cout << "--------";
    std::cout << '\n';

    for (int i = 0; i < N; i++) {
        //std::cout << "    (" << std::setw(2) << i / n << ',' << std::setw(2) << i % n << ")| ";
        std::cout << "    (" << std::setw(2) << i << ")| ";
        for (int j = 0; j < N; j++) {
            if (J[i][j] == 0)
                std::cout << std::setw(7) << 0 << " ";
            else
                std::cout << std::setw(7) << J[i][j] << " ";
        }
        std::cout << '\n';
    }
}

void print_h(float h[N]) {
    std::cout << '\n';

    std::cout << "             (";
    for (int j = 0; j < N; j++)
        // std::cout << std::setw(2) << j / n << ',' << std::setw(2) << j % n << ") (";
        std::cout << std::setw(2) << j << ") (";
    std::cout << "\b \n";

    std::cout << "h =          ";
    for (int i = 0; i < N; i++)
        std::cout << std::setw(7) << h[i] << " ";
    std::cout << '\n';
}

std::ostream &output() {
#ifdef LOG_X_P
    return log_file;
#else
    return std::cout;
#endif
}

void print_x_p(float x[Nb], float p[Nb]) {
    output() << "x | ";
    for (int i = 0; i < Nb; i++)
        output() << std::setw(7) << x[i] << " | ";
    output() << '\n';

    output() << "p | ";
    for (int i = 0; i < Nb; i++)
        output() << std::setw(7) << p[i] << " | ";
    output() << '\n';
}
#endif

/*
Top-level function to export

Parameters:
 * xrate: input array, log2(exchange rate)
    example: xrate[1][3] = log2(2.5) means 1 currency1 = 2.5 currency3
 * x_init: input array, used to initialize x[N]
 * p_init: input array, used to initialize p[N]
 * activation: output array, activation of links in xrate
*/
void bottom(float J[N][N], float h[N], float x_init[N], float p_init[N], bool spin[N]) {
    MMTE blocks[Pb];
    float std_of_J  = sd(J);
    float gamma0 = 0.7 * alpha0 / std_of_J / sqrt(N); // Xi0 in [1]
    float Delta_t_times_gamma0 = Delta_t * gamma0;
    for (int b=0; b < Pb; b++) {
        int offset = Nb * b; // [Nb * b : Nb * b + Nb]
        blocks[b].init(J + offset, h + offset, x_init + offset, p_init + offset, Delta_t_times_gamma0);
    }
    SBM(J, h, blocks, spin);
}


/*
calculate population standard deviation

Parameters:
 * J: input array

 Returns:
 * std: output scalar
*/
float sd(float J[N][N]) {
    float mean = 0;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            mean += J[i][j];
        }
    }
    mean /= (N * N);

    float var = 0;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            float diff = J[i][j] - mean;
            var += diff * diff;
        }
    }
    var /= (N * N);

    float std = sqrt(var);

#ifdef DEBUG_PRINT
    std::cout << '\n';
    std::cout << "sd(J) = " << std << '\n';
#endif

    return std;
}

/*
Simulated bifurcation machine for finding an approximate solution of an Ising model of the form:
  \min_s \left( - \frac{1}{2} \sum_{i=0}^{N-1} \sum_{j=0}^{N-1} {J_{ij} s_i s_j} + \sum_{i=0}^{N-1} {h_i s_i} \right)

Parameters:
 * blocks: object array, elements are initialized MMTE modules
 * spin: output array, true: s_i=1, false: s_i=-1
*/
void SBM(float J[N][N], float h[N], MMTE blocks[Pb], bool spin[N]) {
    float x_a[N] = {0}; // double buffer
    float x_b[N] = {0}; // double buffer

#ifdef DEBUG_PRINT
#ifdef LOG_X_P
    log_file.open("log.txt");
#else
    std::cout << '\n';
#endif
#endif

#ifdef K2000
    int cut = 0;
    int cost = 0;
    int bestCut = -1000;
    int bestCost = -1000;
    bool bestSpin[N] = {0};
#endif

    for (int t = 0; t < N_step; t++) {

#ifdef DEBUG_PRINT
#ifndef LOG_X_P
        std::cout << "t=" << t << ":\n";
#endif
#endif

        for (int b=0; b < Pb; b++) {
            int offset = Nb * b; // [Nb * b : Nb * b + Nb]
            if (t % 2 == 0) {
                blocks[b].step(t, x_a, x_b + offset);
            } else {
                blocks[b].step(t, x_b, x_a + offset);
            }
        }
        for (int i = 0; i < N; i++)
            spin[i] = x_a[i] > 0; // spin = sign(x)

        #ifdef K2000
        evalCost(J, h, spin, cut, cost);
        if (cut > bestCut) {
            bestCut = cut;
            bestCost = cost;
            for (int i = 0; i < N; ++i) {
                bestSpin[i] = spin[i];
            }
        }
        #endif
    }

    #ifdef K2000
    std::cout << "Best cut = " << bestCut << std::endl;
    std::cout << "Best cost = " << bestCost << std::endl;
    std::cout << "Best spin = ";
    for (int i = 0; i < N; ++i) {
        std::cout << bestSpin[i];
    }
    std::cout << std::endl;
    #endif

#ifdef DEBUG_PRINT
#ifndef LOG_X_P
    std::cout << "t="<< N_step <<" (final):\n";
#endif
    for (int b=0; b < Pb; b++)
        print_x_p(blocks[b].x_sub, blocks[b].p_sub);
#ifdef LOG_X_P
    log_file.close();
#endif
#endif
}

#ifdef K2000
void evalCost(float J[N][N], float h[N], bool spin[N], int& cut, int& cost) {
    cost = 0;
    cut = 0;
    for (int j=1; j<N; ++j) {
        for (int i=0; i<j; ++i) {
            cost += J[i][j] * (2*spin[i]-1) * (2*spin[j]-1);
            //cost -= J[i][j] * (spin[i] * spin[j]);
            if (spin[i] != spin[j]) {
                //std::cout << i << " " << j << " " << J[i][j] << std::endl;
                cut += J[i][j];
            }
        }
    }
    //std::cout << "spin = ";
    //for (int i=0; i<N; ++i) {
    //    std::cout << spin[i];
    //}
    //std::cout << "\n";
    //std::cout << "cut = " << cut << std::endl;
    //std::cout << "cost = " << cost << std::endl;
}
#endif // K2000

/*
Initialize a MMTE block (Nb spins)

Parameters:
 * J_sub: input array
 * h_sub: input array
 * x_sub_init: input array
 * p_sub_init: input array
 * Delta_t_times_gamma0: input scalar
*/
void MMTE::init(float J_sub[Nb][N], float h_sub[Nb], float x_sub_init[Nb], float p_sub_init[Nb], float Delta_t_times_gamma0) {
    for (int i = 0; i < Nb; i++) {
        for (int j = 0; j < N; j++) {
            this->J_sub[i][j] = J_sub[i][j];
        }
        this->h_sub[i] = h_sub[i];
        this->x_sub[i] = x_sub_init[i];
        this->p_sub[i] = p_sub_init[i];
        this->Delta_t_times_gamma0 = Delta_t_times_gamma0;
    }
}

/*
Simulate a time step

Parameters:
 * t: input scalar, time step
 * x_prime_in: input array, x_prime from all blocks, passed to MM
 * x_prime_out: output array, x_prime of this block
*/
void MMTE::step(int t, float x_prime_in[N], float x_prime_out[Nb]) {
    float Delta_P[Nb] = {0};

#ifdef DEBUG_PRINT
    print_x_p(x_sub, p_sub);
#endif

    update_alpha_eta(t);

    MM(x_prime_in, Delta_P);

    for (int i = 0; i < Nb; i++) {
        p_sub[i] += Delta_P[i];
    }

    TE();

    for (int i = 0; i < Nb; i++) {
        x_prime_out[i] = x_sub[i] * Delta_t_times_gamma0;
    }
}

/*
update time-dependent functions alpha(t) and eta(t)

Parameters:
 * t: input scalar
*/
void MMTE::update_alpha_eta(int t) {
    const float alpha_beg = 0.;
    const float alpha_end = 1.;
    // alpha(t) is p(t) in [1], changes linearly from 0 to 1
    alpha = (alpha_end - alpha_beg) * ((float) t / N_step) + alpha_beg;
    // eta(t) is stated to be dependent on alpha(t) in [2], but not presents in [1]
    eta = alpha * 0.7 * alpha0 / sqrt(N);
}

/*
Matrix-vector multiplication, Delta_P = J_sub * x

Parameters:
 * x_prime_in: input array
 * Delta_P: output array
*/
inline void MMTE::MM(float x_prime_in[N], float Delta_P[Nb]) {
    for (int i = 0; i < Nb; i++) {
        float temp = 0;
        for (int j = 0; j < N; j++) {
            temp += J_sub[i][j] * x_prime_in[i];
        }
        Delta_P[i] = temp;
    }
}

/*
Time evolution, updates x_sub and p_sub
*/
inline void MMTE::TE() {
    for (int m = 0; m < M; m++) {
        for (int i = 0; i < Nb; i++) {
            p_sub[i] += FX(x_sub[i], h_sub[i]);
            x_sub[i] += FP(p_sub[i]);
        }
    }
}

/*
FX of aSB

Parameters:
 * x_sub_i: input scalar
 * h_sub_i: input scalar

Returns:
 * delta_p: output scalar
*/
inline float MMTE::FX(float x_sub_i, float h_sub_i) {
    float delta_p =  delta_t * (
        - (alpha0 - alpha + beta0 * x_sub_i * x_sub_i) * x_sub_i
        - eta * h_sub_i
    );
    return delta_p;
}

/*
FP

Parameters:
 * p_sub_i: input scalar

Returns:
 * delta_x: output scalar
*/
inline float MMTE::FP(float p_sub_i) {
    float delta_x = delta_t * p_sub_i;
    return delta_x;
}
