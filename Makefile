XDEVICE := xilinx:adm-pcie-7v3:1ddr:3.0
XOCC := $(XILINX_SDACCEL)/bin/xocc
ALTERA_OPENCL := $(ALTERAOCLSDKROOT)
BSP := p385a_sch_ax115 
AOCL_BOARD := p385a_sch_ax115
AOC := $(ALTERA_OPENCL)/bin/aoc
CC := g++

CFLAGS := -g -Wall  
LFLAGS += -lOpenCL -lpthread 

SRCS = main.cpp
CL_SRCS = vector_add.cl
SIM_CLTARGET = $(basename $(CL_SRCS))_sim

OBJS := $(SRCS:.cpp=.o)

TARGET = host_vector_add

.PHONY: all
all: $(TARGET)

.PHONY: altera
altera : CFLAGS += -DALTERA_CL
altera : CLFLAGS += -v 
altera : $(TARGET) $(CL_SRCS:.cl=.aocx)

.PHONY: xilinx
xilinx : CFLAGS += -DXILINX_CL
xilinx : CLFLAGS += -DXILINX_CL
xilinx : $(TARGET) $(CL_SRCS:.cl=.xclbin)

.PHONY: altera_debug
altera_debug : CFLAGS += -DALTERA_CL -DCL_EMULATE
altera_debug : CLFLAGS += -DALTERA_CL -DCL_EMULATE -march=emulator -g
altera_debug : $(TARGET) $(SIM_CLTARGET).aocx
	env CL_CONTEXT_EMULATOR_DEVICE_ALTERA=$(AOCL_BOARD) ./$(TARGET) $(SIM_CLTARGET).aocx

.PHONY: xilinx_debug
xilinx_debug : CFLAGS += -DXILINX_CL -DCL_EMULATE
xilinx_debug : CLFLAGS += -DXILINX_CL -DCL_EMULATE -t sw_emu
xilinx_debug : $(TARGET) $(SIM_CLTARGET).xclbin
	. $(XILINX_OPENCL)/.settings64-SDx.sh && env XCL_EMULATION_MODE=1 ./$(TARGET) $(SIM_CLTARGET).xclbin

.PHONY: run
run: all
	./$(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LFLAGS) -o $@ 

%.o : %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

%_sim.aocx : %.cl
	$(AOC) --board $(AOCL_BOARD) $(CLFLAGS) $< -o $@ 

%_sim.xclbin : %.cl
	$(XOCC) --xdevice $(XDEVICE) $(CLFLAGS) $< -o $@

%.aocx : %.cl
	$(AOC) --board $(AOCL_BOARD) $(CLFLAGS) $< -o $@ 

%.xclbin : %.cl
	$(XOCC) --xdevice $(XDEVICE) $(CLFLAGS) $< -o $@

clean:
	@rm -rf $(TARGET) $(OBJS) $(SIM_CLTARGET) $(SIM_CLTARGET)* *.aoco sdaccel_* 

