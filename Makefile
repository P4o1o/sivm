CC ?= gcc
CFLAGS = -O3 -Wall -std=c18 -I./src -I./src/lib -I./src/core -I./src/lib/thread
DFLAGS = -ggdb3
LIBFLAGS = -lm -lpthread -lrt

SRCDIR = src
BINDIR = bin
ASMDIR = assembly

# tutti i file .c nelle sottocartelle
SRCFILES = $(shell find $(SRCDIR) -name "*.c")
OBJFILES = $(patsubst $(SRCDIR)/%,$(BINDIR)/%,$(SRCFILES:.c=.o))
ASMFILES = $(patsubst $(SRCDIR)/%,$(ASMDIR)/%,$(SRCFILES:.c=.s))

all: sivm

sivm: $(OBJFILES)
	$(CC) $(CFLAGS) $(DFLAGS) -o $@ $(OBJFILES) $(LIBFLAGS)

# Compilazione file C in oggetti
$(BINDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DFLAGS) -c $< -o $@ $(LIBFLAGS)

# Generazione assembly
asm: $(ASMFILES)

$(ASMDIR)/%.s: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -S -masm=intel $< -o $@ $(LIBFLAGS)

# Pulizia completa
.PHONY: clean
clean:
	rm -rf $(BINDIR) $(ASMDIR)
