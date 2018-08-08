# Minimalistic *nix Makefile
#
# Usage: build single target
# $ make TARGET=<online_fdbm|offline_fdbm>
# or build all targets using
# $ make
#

# The targets
TARGET ?= online_fdbm offline_fdbm

# default receipe
all: clean $(TARGET)

# build receipe
%: %.c
	gcc -Wall -O3 $< -o $@ -lm

# clean receipe
clean:
	rm -f $(TARGET)
