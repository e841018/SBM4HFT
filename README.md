# Simulated Bifurcation Machine for High-Frequency Trading

This is a final project of the course Advanced Computer Architecture (CSIE 5059) at National Taiwan University, 2021 spring.

This repository contains an implementation of a SBM-based currency arbitrage machine. Please refer to the [slides](https://docs.google.com/presentation/d/1Y7Ps9WcomCCpFojgtLt-LdptRU9dq9axmlOFJ6MbmJU) for an overview.

## Folder structure

* `docs` contains a README file describing how the `QUBO2Ising` function is derived mathematically.
* `data` contains a Python script for generating K2000 or smaller datasets.

* `src/original` contains the kernel codes and their test benches. They can be simply built with `src/original/Makefile`.
* `src/host` contains the OpenCL host program.
* `impl_result` contains the csynth reports.

## Build Setup

* The kernel testbenches are built with the following settings:
  * Vivado HLS 2019.2
  * FPGA: Alveo U280, the only Alveo model with HBM found in  Vivado HLS 2019.2.
* The host program is built with the following settings:
  * Vitis 2020.2
  * FPGA: Alveo U50

