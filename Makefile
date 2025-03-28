CC = gcc
CFLAGS = -O3 -Wall -pedantic -std=c18
DFLAGS = -ggdb3
SRCDIR = src
BINDIR = bin
GPERF_INPUT = $(SRCDIR)/instr.gperf
GPERF_OUTPUT = $(SRCDIR)/instr_hash.c
SRCFILES = $(wildcard $(SRCDIR)/*.c) $(GPERF_OUTPUT)
OBJFILES = $(patsubst $(SRCDIR)/%.c,$(BINDIR)/%.o,$(SRCFILES))

all: $(GPERF_OUTPUT) sivm

sivm: $(OBJFILES)
	$(CC) $(CFLAGS) $(DFLAGS) -o $@ $(OBJFILES) -lm -fopenmp

$(BINDIR)/%.o: $(SRCDIR)/%.c | $(BINDIR)
	$(CC) $(CFLAGS) $(DFLAGS) -c $< -o $@ -fopenmp

$(GPERF_OUTPUT): $(GPERF_INPUT)
	gperf -L C -t -N inrstr_lookup -K mnem $< > $@

$(BINDIR):
	mkdir -p $(BINDIR)

.PHONY: clean
clean:
	rm -rf $(BINDIR)/*.o $(GPERF_OUTPUT) sivm
