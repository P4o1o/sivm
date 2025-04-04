CC = gcc
CFLAGS = -O3 -Wall -pedantic -std=c18
DFLAGS = -ggdb3
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
	$(CC) $(CFLAGS) $(DFLAGS) -o $@ $(OBJFILES) -lm -fopenmp

$(BINDIR)/%.o: $(SRCDIR)/%.c | $(BINDIR)
	$(CC) $(CFLAGS) $(DFLAGS) -c $< -o $@ -fopenmp

asm: $(GPERF_OUTPUT) $(ASMFILES)

$(GPERF_OUTPUT): $(GPERF_INPUT)
	gperf -L C -t -N inrstr_lookup -K mnem $< > $@

$(ASMDIR)/%.s: $(SRCDIR)/%.c | $(ASMDIR)
	$(CC) $(CFLAGS) -S -masm=intel $< -o $@

$(BINDIR) $(ASMDIR):
	mkdir -p $@

.PHONY: clean
clean:
	rm -rf $(BINDIR)/*.o $(ASMDIR)/*.s $(GPERF_OUTPUT) sivm
