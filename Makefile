# Minimalistic Makefile
# generate TARGET
# usage: make

# The final target
TARGET := cfdbm

# The source and object files needed by the target
SRC    := online_fdbm.c audio.c
OBJ	   := $(SRC:%c=%o)

# default receipe
all: clean $(TARGET)

# target receipe
$(TARGET): $(OBJ)
	gcc -Wall -o $@ $^

# objects receipe
%.o: %.c
	gcc -Wall -c $<

# clean receipe
clean:
	rm -f $(OBJ) $(TARGET)
