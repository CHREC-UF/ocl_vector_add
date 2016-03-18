XDEVICE := xilinx:adm-pcie-7v3:1ddr:2.1
XOCC := $(XILINX_SDACCEL)/bin/xocc
ALTERA_OPENCL := $(ALTERAOCLSDKROOT)
BSP := p385_hpc_d5
AOC := $(ALTERA_OPENCL)/bin/aoc
CC := g++

AOCL_COMPILE_CONFIG := $(shell aocl compile-config)
AOCL_LINK_CONFIG := $(shell aocl link-config)

CFLAGS := -g -Wall -std=c++11
LFLAGS += -lOpenCL $(AOCL_LINK_CONFIG)

SRCS = main.cpp

OBJS := $(SRCS:.cpp=.o)

TARGET = host_vector_add

.PHONY: all
all: $(TARGET)

.PHONY: altera
altera : CFLAGS += -DALTERA_CL
altera : CLFLAGS += -DALTERA_CL
altera : $(TARGET) $(CL_SRCS:.cl=.aocx)

.PHONY: xilinx
xilinx : CFLAGS += -DXILINX_CL
xilinx : CLFLAGS += -DXILINX_CL
xilinx : $(TARGET) $(CL_SRCS:.cl=.xclbin)

.PHONY: altera_debug
altera_debug : CFLAGS += -DALTERA_CL -DCL_EMULATE
altera_debug : CLFLAGS += -DALTERA_CL -DCL_EMULATE -march=emulator -g
altera_debug : $(TARGET) $(CL_SRCS:.cl=.aocx)

.PHONY: xilinx_debug
xilinx_debug : CFLAGS += -DXILINX_CL -DCL_EMULATE
xilinx_debug : CLFLAGS += -DXILINX_CL -DCL_EMULATE -t sw_emu
xilinx_debug : $(TARGET) $(CL_SRCS:.cl=.xclbin)))

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LFLAGS) -o $@ 

%.o : %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

%.aocx : %.cl
	$(AOC) --board $(AOCL_BOARD) $(CLFLAGS) $< -o $@ 

%.xclbin : %.cl
	$(XOCC) --xdevice $(XDEVICE) $(CLFLAGS) $< -o $@

clean:
	rm -rf $(TARGET) $(OBJS)

