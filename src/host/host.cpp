#include <stdlib.h>
#include <fstream>
#include <iostream>
#include "xcl2.hpp"
#include <iomanip>
#include <vector>
#include "mm_datatype.h"

// problem size
const int n = 3;       // number of currencies
const int N = n * n;   // number of variables
const int N_step = 40; // number of steps
const int M = 2;       // number of substeps in a step

// degree of parallelism
const int Pb = 1;      // number of blocks
const int Nb = N / Pb; // the number of spins to be handled in a block

/*
These parameters are set according to p.6 of [2].
The corresponding components in [1] are annotated in comments.
[1] Combinatorial optimization by simulating adiabatic bifurcations in nonlinear Hamiltonian systems
[2] FPGA-based Simulated Bifurcation Machine
*/
const float alpha0 = 1.; // Delta in [1]
const float beta0 = 1.;  // K in [1]
const float Delta_t = 0.7;
const float delta_t = Delta_t / M;
const float alpha_init = 0.; // p(t) in [1], changes linearly from 0 to 1
const float Delta_alpha = (1. - alpha_init) / N_step;

const float M1 = 0.2;
const float M2 = 0.2;

static const std::string error_message =
    "Error: Result mismatch:\n"
    "i = %d CPU result = %d Device result = %d\n";

void rand_init(std::vector<float, aligned_allocator<float>> &arr, float low, float high)
{
    for (int i = 0; i < N; i++)
    {
        float r = ((float)rand()) / ((float)RAND_MAX);
        arr[i] = low + r * (high - low);
    }
}

float calc_obj0(float xrate[n][n], bool activation[n][n])
{
    float obj0 = 0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            obj0 += activation[i][j] * xrate[i][j];
    return obj0;
}

float calc_obj1(bool activation[n][n])
{
    float obj1 = 0;
    for (int i = 0; i < n; i++)
    {
        int temp = 0;
        for (int j = 0; j < n; j++)
        {
            temp += activation[i][j];
            temp -= activation[j][i];
        }
        obj1 -= M1 * temp * temp;
    }
    return obj1;
}

float calc_obj2(bool activation[n][n])
{
    float obj2 = 0;
    for (int i = 0; i < n; i++)
    {
        int temp = 0;
        for (int j = 0; j < n; j++)
        {
            temp += activation[i][j];
        }
        obj2 -= M2 * temp * (temp - 1);
    }
    return obj2;
}

void print_activation(bool activation[n][n])
{
    std::cout << '\n';
    std::cout << "activation =\n";
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            std::cout << ' ' << activation[i][j];
        }
        std::cout << '\n';
    }
}

void print_objectives(float xrate[n][n], bool activation[n][n])
{
    float obj0 = calc_obj0(xrate, activation);
    float obj1 = calc_obj1(activation);
    float obj2 = calc_obj2(activation);
    std::cout << '\n';
    std::cout << "maximize {  obj0  +   obj1  +   obj2 }" << '\n';
    std::cout << "          " << std::setw(7) << obj0
              << " - " << std::setw(7) << -obj1
              << " - " << std::setw(7) << -obj2
              << " = " << obj0 + obj1 + obj2 << '\n';
}

bool find_1_on_diagonal(bool activation[n][n])
{
    for (int i = 0; i < n; i++)
        if (activation[i][i] == 1)
            return true;
    return false;
}

void brute_force(float xrate[n][n])
{
    int n_config = 1 << N;
    bool activation[n][n];
    for (int config = 0; config < n_config; config++)
    {
        int bin = config;
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                activation[i][j] = bin & 1;
                bin >>= 1;
            }
        }

        if (find_1_on_diagonal(activation))
            continue;
        float obj0 = calc_obj0(xrate, activation);
        if (obj0 <= 0)
            continue;
        if (calc_obj1(activation) < 0)
            continue;
        if (calc_obj2(activation) < 0)
            continue;

        std::cout << '\n';
        std::cout << "brute_force: solution found: \n";
        print_activation(activation);
        std::cout << "obj0 = " << obj0 << '\n';
    }
}

int main(int argc, char *argv[])
{

    //TARGET_DEVICE macro needs to be passed from gcc command line
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <xclbin>" << std::endl;
        return EXIT_FAILURE;
    }

    char *xclbinFilename = argv[1];

    // Compute the size of array in bytes
    //size_t size_in_bytes = DATA_SIZE * sizeof(int);

    //size_t xrate_size_bytes = n * n * sizeof(float);
    size_t xpinit_size_bytes = n * sizeof(float);
    //size_t activatetion_size_bytes = n * n * sizeof(bool);
    size_t orderEntryOperation_t_size_bytes = N * sizeof(orderEntryOperation_t);

    // Creates a vector of DATA_SIZE elements with an initial value of 10 and 32
    // using customized allocator for getting buffer alignment to 4k boundary

    std::vector<cl::Device> devices;
    cl::Device device;
    std::vector<cl::Platform> platforms;
    bool found_device = false;

    //traversing all Platforms To find Xilinx Platform and targeted
    //Device in Xilinx Platform
    cl::Platform::get(&platforms);
    for (size_t i = 0; (i < platforms.size()) & (found_device == false); i++)
    {
        cl::Platform platform = platforms[i];
        std::string platformName = platform.getInfo<CL_PLATFORM_NAME>();
        if (platformName == "Xilinx")
        {
            devices.clear();
            platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices);
            if (devices.size())
            {
                device = devices[0];
                found_device = true;
                break;
            }
        }
    }
    if (found_device == false)
    {
        std::cout << "Error: Unable to find Target Device "
                  << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        return EXIT_FAILURE;
    }

    // Creating Context and Command Queue for selected device
    cl::Context context(device);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE);

    // Load xclbin
    std::cout << "Loading: '" << xclbinFilename << "'\n";
    std::ifstream bin_file(xclbinFilename, std::ifstream::binary);
    bin_file.seekg(0, bin_file.end);
    unsigned nb = bin_file.tellg();
    bin_file.seekg(0, bin_file.beg);
    char *buf = new char[nb];
    bin_file.read(buf, nb);

    // Creating Program from Binary File
    cl::Program::Binaries bins;
    bins.push_back({buf, nb});
    devices.resize(1);
    cl::Program program(context, devices, bins);

    // This call will get the kernel object from program. A kernel is an
    // OpenCL function that is executed on the FPGA.
    cl::Kernel krnl_sbm(program, "exch");

    float xrate[n][n] = {
        {0, -0.1203764797, 0.04217729502},
        {0.1203365239, 0, 0.1620771323},
        {-0.042122476, -0.1619456788, 0},
    };
    //orderEntryOperation_t Operations[n];
    std::vector<orderEntryOperation_t, aligned_allocator<orderEntryOperation_t>> Operations(n);
    std::vector<float, aligned_allocator<float>> x_init(N);
    std::vector<float, aligned_allocator<float>> p_init(N);
    orderBookResponse_t *current_order;
    current_order = new orderBookResponse_t;
    //float x_init[N] = {0};
    //float p_init[N] = {0};

    // These commands will allocate memory on the Device. The cl::Buffer objects can
    // be used to reference the memory locations on the device.
    //cl::Buffer buffer_xrate(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, xrate_size_bytes, xrate);
    cl::Buffer buffer_order(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(orderBookResponse_t), current_order);
    cl::Buffer buffer_xinit(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, xpinit_size_bytes, x_init.data());
    cl::Buffer buffer_pinit(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, xpinit_size_bytes, p_init.data());
    cl::Buffer buffer_orderEntryOperation(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, orderEntryOperation_t_size_bytes, Operations.data());

    //set the kernel Arguments
    int narg = 0;
    //krnl_sbm.setArg(narg++, buffer_xrate);

    krnl_sbm.setArg(narg++, buffer_order);
    krnl_sbm.setArg(narg++, buffer_orderEntryOperation);
    krnl_sbm.setArg(narg++, buffer_xinit);
    krnl_sbm.setArg(narg++, buffer_pinit);

    // //We then need to map our OpenCL buffers to get the pointers
    // float **ptr_xrate = (float **)q.enqueueMapBuffer(buffer_xrate, CL_TRUE, CL_MAP_WRITE, 0, xrate_size_bytes);
    // float *ptr_xinit = (float *)q.enqueueMapBuffer(buffer_xinit, CL_TRUE, CL_MAP_WRITE, 0, xpinit_size_bytes);
    // float *ptr_pinit = (float *)q.enqueueMapBuffer(buffer_pinit, CL_TRUE, CL_MAP_WRITE, 0, xpinit_size_bytes);
    // bool **ptr_activation = (bool **)q.enqueueMapBuffer(buffer_activation, CL_TRUE, CL_MAP_READ, 0, activatetion_size_bytes);

    //set the random seed to zero to reproduce same result every time;
    srand(0);
    std::cout << std::fixed << std::setprecision(4);

    for (unsigned int i = 0; i < n * n; i++)
    {
        // invoke kernel
        if (xrate[i / n][i % n] == 0.)
        {
            continue;
        }
        rand_init(x_init, -0.1, 0.1);
        rand_init(p_init, -0.1, 0.1);
        current_order->symbolRow = i / n;
        current_order->symbolCol = i % n;
        current_order->askPrice = xrate[i / n][i % n];
        //std::cout << current_order->symbolRow << " " << current_order->symbolCol << " " << current_order->askPrice << std::endl;

        // Data will be migrated to kernel space
        q.enqueueMigrateMemObjects({buffer_xinit, buffer_pinit, buffer_order}, 0 /* 0 means from host*/);
        //Launch the Kernel
        q.enqueueTask(krnl_sbm);
        brute_force(xrate);
        // The result of the previous kernel execution will need to be retrieved in
        // order to view the results. This call will transfer the data from FPGA to
        // source_results vector
        q.enqueueMigrateMemObjects({buffer_orderEntryOperation}, CL_MIGRATE_MEM_OBJECT_HOST);
        q.finish();
        for (int j = 0; j < N; j++)
        {
            if (Operations[j].timestamp == (uint)(-1))
            {
                if (j == 0)
                {
                    std::cout << "No solution" << std::endl;
                }
                break;
            }
            std::cout << "buy index (row,col) = (" << Operations[j].symbolRow << ", " << Operations[j].symbolCol << ")" << std::endl
                      << std::endl;
        }
        bool activation[n][n] = {0};
        for (int c = 0; c < N; c++)
        {
            if (Operations[c].timestamp == (unsigned int)(-1))
                break;
            //uint64_t t = Operations[c].timestamp;
            activation[Operations[c].symbolRow][Operations[c].symbolCol] = true;
        }

        print_activation(activation);
        print_objectives(xrate, activation);
    }

    //Verify the result
    // match = 0;
    // for (int i = 0; i < DATA_SIZE; i++)
    // {
    //     int host_result = ptr_a[i] + ptr_b[i];
    //     if (ptr_result[i] != host_result)
    //     {
    //         printf(error_message.c_str(), i, host_result, ptr_result[i]);
    //         match = 1;
    //         break;
    //     }
    // }

    // q.enqueueUnmapMemObject(buffer_a, ptr_a);
    // q.enqueueUnmapMemObject(buffer_b, ptr_b);
    // q.enqueueUnmapMemObject(buffer_result, ptr_result);
    // q.finish();

    //std::cout << "TEST " << (match ? "FAILED" : "PASSED_YA") << std::endl;
    //return (match ? EXIT_FAILURE : EXIT_SUCCESS);
    return 0;
}
