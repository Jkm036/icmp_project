CC=gcc
CFLAGS=-Wall Wextra
TARGET=joshping
SRC=icmp_ping.c
#Substitution command (replacing .c with .o in file)
OBJ=$(SRC:.c=.o)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ 
%.o:  %.c
	$(CC) -c $< -c $@

.PHONY: clean
clean:
	rm -f $(OBJ)
