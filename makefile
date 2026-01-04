MAKEFLAGS += --no-builtin-rules

SRC = $(shell find -name "*.c")
OBJ = $(SRC:.c=.o)

#cc flags
CFLAGS += -Wall -Wextra -std=c99
CFLAGS += -Iinclude

all : libterm.a

libterm.a : $(OBJ)
	$(AR) rcs $@ $^

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $^
	
install : all
	@echo '[install headers]'
	@mkdir -p $(PREFIX)/include
	@cp include/libterm.h $(PREFIX)/include
	@echo '[install libterm.a]'
	@mkdir -p $(PREFIX)/lib
	@cp libterm.a $(PREFIX)/lib

clean : 
	rm -f $(OBJ)
