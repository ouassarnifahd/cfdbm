# Minimalistic Makefile
# generate TARGET
# usage: make

# The targets
TARGET ?= online_fdbm offline_fdbm

# default receipe
all: clean $(TARGET)

# build receipe
%: %.c
	gcc -Wall -lm -s O4 $< -o $@

# clean receipe
clean:
	rm -f $(TARGET)
