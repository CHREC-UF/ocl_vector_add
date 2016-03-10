XILINX_OPENCL := $(XILINX_SDACCEL)
DSA := xilinx:adm-pcie-7v3:1ddr:2.1
XOCC := $(XILINX_SDACCEL)/bin/xocc
ALTERA_OPENCL := $(ALTERAOCLSDKROOT)
BSP := p385_hpc_d5
AOC := $(ALTERA_OPENCL)/bin/aoc
CPP := g++

#OPENCL_INC := $(XILINX_OPENCL)/runtime/include/1_2
#OPENCL_LIB := $(XILINX_OPENCL)/runtime/lib/x86_64

#AOCL_COMPILE_CONFIG := $(shell aocl compile-config)
AOCL_LINK_CONFIG := $(shell aocl link-config)

AOCL_COMPILE_CONFIG := -I$(ALTERAOCLSDKROOT)/host/include
#AOCL_LINK_CONFIG := -L$(ALTERAOCLSDKROOT)/host/linux64/lib

CXXFLAGS := -g -Wall -std=c++11
CLXFLAGS := -g --xdevice $(DSA)
CLAFLAGS := -g --board $(BSP)

.PHONY: all
all: vector_add

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

vector_add: main.cpp
	$(CXX) $(CXXFLAGS) $(AOCL_COMPILE_CONFIG) -o $@ main.cpp -lOpenCL $(AOCL_LINK_CONFIG) 

vector_add.aocx: vector_add.cl
	$(AOC) $(CLAFLAGS) $< -o $@

vector_add.xclbin: vector_add.cl
	$(XOCC) $(CLXFLAGS) $< -o $@

clean:
	rm -rf vector_add
