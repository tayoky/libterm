TOP = $(CURDIR)
TMAKE_DIR = $(TOP)/make
include $(TMAKE_DIR)/tmake-init.mk

LIB = term
HEADERS = include/libterm.h
CFLAGS += -Iinclude

include $(TMAKE_DIR)/tmake-lib.mk
