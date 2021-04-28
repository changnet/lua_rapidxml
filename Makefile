##### Build defaults #####
CC = g++
TARGET_SO =         lua_rapidxml.so
TARGET_A  =         liblua_rapidxml.a
PREFIX =            /usr/local
CPPFLAGS =            -O0 -g3 -Wall -pedantic
#CPPFLAGS =            -O2 -Wall -pedantic -DNDEBUG
LUA_RAPIDXML_CFLAGS =      -fpic
LUA_RAPIDXML_LDFLAGS =     -shared
LUA_INCLUDE_DIR =   $(PREFIX)/include
RAPIDXML_INCLUDE_DIR = ./rapidxml
AR= ar rc
RANLIB= ranlib

BUILD_CFLAGS =      -I$(LUA_INCLUDE_DIR) -I$(RAPIDXML_INCLUDE_DIR) $(LUA_RAPIDXML_CFLAGS)
OBJS =              lrapidxml.o

all: $(TARGET_SO) $(TARGET_A)

$(TARGET_SO): $(OBJS)
	$(CC) $(LDFLAGS) $(LUA_RAPIDXML_LDFLAGS) -o $@ $(OBJS)

$(TARGET_A): $(OBJS)
	$(AR) $@ $(OBJS)
	$(RANLIB) $@

lrapidxml.o:  lrapidxml.cpp lrapidxml.hpp
	$(CC) -c $(CPPFLAGS) $(BUILD_CFLAGS) -o $@ $<

test:
	lua test.lua

clean:
	rm -f *.o test.xml $(TARGET_SO) $(TARGET_A)

.PHONY: all clean test
