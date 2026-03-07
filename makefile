MAKEFLAGS += --no-builtin-rules

SRC = $(shell find -name "*.c")
OBJ = $(SRC:.c=.o)

#cc flags
CFLAGS += -Wall -Wextra -std=c99 -fpic
CFLAGS += -Iinclude

all : libterm.so libterm.a

libterm.so : $(OBJ)
	$(CC) $(CFLAGS) -shared -o $@ $^

libterm.a : $(OBJ)
	$(AR) rcs $@ $^

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $^
	
install : all
	@echo '[install headers]'
	@mkdir -p $(PREFIX)/include
	@cp include/libterm.h $(PREFIX)/include
	@echo '[install libterm.so]'
	@mkdir -p $(PREFIX)/lib
	@cp libterm.so $(PREFIX)/lib
	@echo '[install libterm.a]'
	@cp libterm.a $(PREFIX)/lib

clean : 
	rm -f $(OBJ)
