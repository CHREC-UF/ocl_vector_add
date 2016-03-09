#!/bin/bash

export XILINX_OPENCL=/opt/Xilinx/SDAccel/2015.4
export XCL_PLATFORM=xilinx_adm-pcie-7v3_1ddr_2_1

env LD_LIBRARY_PATH=$XILINX_OPENCL/runtime/lib/x86_64:$LD_LIBRARY_PATH ./xilinx_add vector_add.xclbin

