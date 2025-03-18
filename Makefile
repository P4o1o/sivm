CC = gcc
CFLAGS = -O3 -Wall -pedantic -std=c18
DFLAGS = -ggdb3
SRCDIR = src
BINDIR = bin
SRCFILES = $(wildcard $(SRCDIR)/*.c)
OBJFILES = $(patsubst $(SRCDIR)/%.c,$(BINDIR)/%.o,$(SRCFILES))

all: sivm

sivm : $(OBJFILES)
	$(CC) $(CFLAGS) $(DFLAGS) -o sivm $(OBJFILES) -lm -fopenmp

$(BINDIR)/%.o: $(SRCDIR)/%.c | $(BINDIR)
	$(CC) $(CFLAGS) $(DFLAGS) -c $< -o $@ -fopenmp

$(BINDIR):
	mkdir $(BINDIR)

.PHONY: clean
clean:
	rm -rf $(BINDIR)/*.o
