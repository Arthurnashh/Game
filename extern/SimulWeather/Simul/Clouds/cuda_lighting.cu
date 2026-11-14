// includes, system
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// includes, project
#include <cutil_inline.h>

// includes, kernels
#include <cppIntegration_kernel.cu>

////////////////////////////////////////////////////////////////////////////////
//! Entry point for Cuda functionality on host side
//! @param argc  command line argument count
//! @param argv  command line arguments
//! @param data  data to process on the device
//! @param len   len of \a data
////////////////////////////////////////////////////////////////////////////////
extern "C" void
runTest(const int argc, const char** argv, char* data, int2* data_int2, unsigned int len)
{
    // use command-line specified CUDA device, otherwise use device with highest Gflops/s
    if( cutCheckCmdLineFlag(argc, (const char**)argv, "device") )
        cutilDeviceInit(argc, (char**)argv);
    else
        cudaSetDevice( cutGetMaxGflopsDeviceId() );

    const unsigned int num_threads = len / 4;
    cutilCondition(0 == (len % 4));
    const unsigned int mem_size = sizeof(char) * len;
    const unsigned int mem_size_int2 = sizeof(int2) * len;

    // allocate device memory
    char* d_data;
    cutilSafeCall(cudaMalloc((void**) &d_data, mem_size));
    // copy host memory to device
    cutilSafeCall(cudaMemcpy(d_data, data, mem_size,
                            cudaMemcpyHostToDevice) );
    // allocate device memory for int2 version
    int2* d_data_int2;
    cutilSafeCall(cudaMalloc((void**) &d_data_int2, mem_size_int2));
    // copy host memory to device
    cutilSafeCall(cudaMemcpy(d_data_int2, data_int2, mem_size_int2,
                            cudaMemcpyHostToDevice) );

    // setup execution parameters
    dim3 grid(1, 1, 1);
    dim3 threads(num_threads, 1, 1);
    dim3 threads2(len, 1, 1); // more threads needed fir separate int2 version
    // execute the kernel
    kernel<<< grid, threads >>>((int*) d_data);
    kernel2<<< grid, threads2 >>>(d_data_int2);

    // check if kernel execution generated and error
    cutilCheckMsg("Kernel execution failed");

    // copy results from device to host
    cutilSafeCall(cudaMemcpy(data, d_data, mem_size,
                            cudaMemcpyDeviceToHost));
    cutilSafeCall(cudaMemcpy(data_int2, d_data_int2, mem_size_int2,
                            cudaMemcpyDeviceToHost));

    // cleanup memory
    cutilSafeCall(cudaFree(d_data));
    cutilSafeCall(cudaFree(d_data_int2));

    cudaThreadExit();
}
