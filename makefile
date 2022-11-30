# Makefile for ZeroToHero

PROJECT=zero
BASE=./

# Compiler
CC=g++
#CC=clang

# Linker
LL=$(CC)
# Must link with gcc if we are trying to avoid libstdc++
#LL=gcc

INC=-Isrc -Isrc/$(PROJECT) -I$(BASE)src -I$(BASE)/lib/libuv/include 

# To not use the standard library we have to disable threadsafe satics along
# with the usual suspects like exceptions and rtti
#NOSTDLIB=-nostdlib -nodefaultlibs -fno-threadsafe-statics -fno-exceptions -fno-rtti 

# CLang requires -lstdc++, no escaping that !

#LIB=$(NOSTDLIB) -lm -lpthread -ldl -lrt -fsanitize=address -L$(BASE)
#LIB=-lstdc++ -lm -lpthread -ldl -lrt -fsanitize=address -L$(BASE)
LIB=-lstdc++ -lm -lpthread -ldl -lrt -L$(BASE)
#LIB=-lm -lpthread -lz -ldl -lssl -lcrypto -lrt -ltracy -fsanitize=address -L$(BASE)
#LIB= $(NOSTDLIB) -lm -lpthread -lz -ldl -lssl -lcrypto -lrt -ltracy -fsanitize=address -L$(BASE)

OBJ=$(BASE)lib/libuv/libuv.a
#OPTSTD=--std=c++20
#OPTSTD=--std=c++17
#OPTSTD=--std=c++14
OPTSTD=--std=c++11

OPTARCH=-march=native
OPTFLAG=-fpermissive -fno-operator-names -fno-exceptions -fno-rtti 
#OPTFLAG=-fpermissive -fno-operator-names $(NOSTDLIB)

# Release

#CFLAGS=$(OPTFLAG) $(OPTARCH) $(OPTSTD) -w -O3 -Wno-narrowing

# Address sanitizer
#CFLAGS=$(OPTFLAG) $(OPTARCH) $(OPTSTD) -Wno-narrowing -w -O0 -ggdb -fno-omit-frame-pointer -fsanitize=address -D_DEBUG

# Tracy pofiler
#CFLAGS=$(OPTFLAG) $(OPTARCH) --std=c++17 -Wno-narrowing -w -O0 -ggdb -fno-omit-frame-pointer -fsanitize=address -D_DEBUG -DTRACY_ENABLE
#CFLAGS=$(OPTFLAG) $(OPTARCH) -Wno-narrowing -w -O0 -ggdb -fno-strict-aliasing -fno-omit-frame-pointer -fsanitize=address -D_DEBUG -DTRACY_ENABLE
CFLAGS=$(OPTFLAG) $(OPTARCH) -Wno-narrowing -O0 -ggdb -fno-strict-aliasing -fno-omit-frame-pointer -D_DEBUG -DTRACY_ENABLE

SRCDIR=src
OBJDIR=build

SRCFILES=$(shell find $(SRCDIR)/ -name "*.cpp")
INCFILES=$(shell find $(SRCDIR)/ -name "*.d")

OBJFILES=$(patsubst %.cpp,%.o,$(SRCFILES))
DEPFILES=$(patsubst %.cpp,%.d,$(SRCFILES))

default: all

clean:
	$(warning $(OBJFILES))
	rm -f $(OBJFILES)
	rm -f $(DEPFILES)
	#rm -rf *.o
	#rm -rf *.d

all: $(PROJECT)

$(PROJECT): $(OBJFILES)	 
	$(LL) $(OBJFILES) $(OBJ) $(LIB) -o $@

$(OBJFILES):  
	$(CC) $(INC) $(CFLAGS) -c $< -o $@

depend: $(DEPFILES)

$(DEPFILES): 
	$(warning $< $@)
	$(CC) $(OPTSTD) $(INC) $(patsubst %.d,%.cpp,$@) -E -MM -MT $(patsubst src/%.d,src/%.o,$@) -MF $@ > /dev/null
	
include $(INCFILES)
