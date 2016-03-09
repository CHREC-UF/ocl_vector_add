XILINX_OPENCL := $(XILINX_SDACCEL)
DSA := xilinx:adm-pcie-7v3:1ddr:2.0
XOCC := $(XILINX_SDACCEL)/bin/xocc
ALTERA_OPENCL := $(ALTERAOCLSDKROOT)
BSP := p385_hpc_d5
AOC := $(ALTERA_OPENCL)/bin/aoc
CPP := g++

OPENCL_INC := $(XILINX_OPENCL)/runtime/include/1_2
OPENCL_LIB := $(XILINX_OPENCL)/runtime/lib/x86_64

AOCL_COMPILE_CONFIG := $(shell aocl compile-config)
AOCL_LINK_CONFIG := $(shell aocl link-config)

CXXFLAGS := -g -Wall -std=c++11
CLXFLAGS := -g --xdevice $(DSA)
CLAFLAGS := -g --board $(BSP)

.PHONY: all
all: xilinx xclbin

.PHONY: aocx
aocx: vector_add.aocx

.PHONY: xclbin
xclbin: vector_add.xclbin

.PHONY: altera
altera: altera_add

.PHONY: altera_debug
altera_debug: CXXFLAGS += -DAOC_EMULATE -g
altera_debug: CLAFLAGS += -DAOC_EMULATE -march=emulator -g
altera_debug: altera_add aocx	
	env CL_CONTEXT_EMULATOR_DEVICE_ALTERA=$(BSP) gdb --args ./altera_add vector_add.aocx

.PHONY: intel
intel: intel_add

altera_add: main.cpp
	$(CXX) $(CXXFLAGS) $(AOCL_COMPILE_CONFIG) -o $@ main.cpp $(AOCL_LINK_CONFIG)

vector_add.aocx: vector_add.cl
	$(AOC) $(CLAFLAGS) $< -o $@

.PHONY: xilinx
xilinx: xilinx_add

xilinx_add: main.cpp
	$(CXX) $(CXXFLAGS) -I$(OPENCL_INC) -L$(OPENCL_LIB) -o $@ main.cpp -lOpenCL #-lxilinxopencl -llmx6.0

vector_add.xclbin: vector_add.cl
	$(XOCC) $(CLXFLAGS) $< -o $@

intel_add: main.cpp
	$(CXX) $(CXXFLAGS) -DINTEL_CL -I/opt/intel/opencl/include -L/opt/intel/opencl/lib64 -o $@ main.cpp -lOpenCL 

clean:
	rm -rf altera_add xilinx_add intel_add
