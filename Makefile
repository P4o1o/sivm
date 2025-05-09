NVCC = nvcc
CUDAFLAGS = -arch=native
INCLUDES = -I/usr/local/cuda/include
CC = gcc
CFLAGS = -O3 -Wall -pedantic -std=c18 $(INCLUDES)
DFLAGS = -ggdb3
LIBFLAGS = -lcudart -lm -fopenmp -L/usr/local/cuda/lib64
SRCDIR = src
BINDIR = bin
ASMDIR = assembly
GPERF_INPUT = $(SRCDIR)/instr.gperf
GPERF_OUTPUT = $(SRCDIR)/instr_hash.h
SRCFILES = $(wildcard $(SRCDIR)/*.c)
OBJFILES = $(patsubst $(SRCDIR)/%.c,$(BINDIR)/%.o,$(SRCFILES))
ASMFILES = $(patsubst $(SRCDIR)/%.c,$(ASMDIR)/%.s,$(SRCFILES))

all: $(GPERF_OUTPUT) sivm

sivm: $(OBJFILES)
	$(CC) $(CFLAGS) $(DFLAGS) -o $@ $(OBJFILES) $(LIBFLAGS)

$(BINDIR)/%.o: $(SRCDIR)/%.c | $(BINDIR)
	$(CC) $(CFLAGS) $(DFLAGS) -c $< -o $@ $(LIBFLAGS)

$(BINDIR)/%.o: $(SRCDIR)/%.cu | $(BINDIR)
	$(NVCC) -dc $(CUDAFLAGS) $(INCLUDES) $< -o $@  

asm: $(GPERF_OUTPUT) $(ASMFILES)

$(GPERF_OUTPUT): $(GPERF_INPUT)
	gperf -L C -t -N inrstr_lookup -K mnem $< > $@

$(ASMDIR)/%.s: $(SRCDIR)/%.c | $(ASMDIR)
	$(CC) $(CFLAGS) -S -masm=intel $< -o $@ $(LIBFLAGS)

$(BINDIR) $(ASMDIR):
	mkdir -p $@

.PHONY: clean
clean:
	rm -rf $(BINDIR)/*.o $(ASMDIR)/*.s $(GPERF_OUTPUT) sivm
