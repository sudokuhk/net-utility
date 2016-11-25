cxx=g++
SOURCES = $(wildcard *.cpp)
OBJECTS = $(patsubst %.cpp, %, $(SOURCES))

PWD = $(shell pwd)
INC_PATH = $(shell pwd)
LIB_PATH = $(shell pwd)
LIBRARY = pthread

INC_PATH += ../
LIB_PATH += ../unetwork ../uhttp ../uthread
LIBRARY  += unetwork uhttp uthread z

INC_PATH := $(patsubst %, -I%, $(INC_PATH))
LIB_PATH := $(patsubst %, -L%, $(LIB_PATH))
LIBNAME  := $(patsubst %, lib%.so, $(LIBRARY))
LIBRARY  := $(patsubst %, -l%, $(LIBRARY))

all : $(OBJECTS)
	@find .. ! -path "*test*" -name "*.so" -exec cp -f {} . \;

% : %.cpp
	$(cxx) -O0 -g $(INC_PATH) $(LIB_PATH) $^ -o $@ $(LIBRARY)

.PHONY : clean

clean:
	-rm -f $(LIBNAME)
	-rm -f $(OBJECTS)
	-rm -f *.o
	-rm -f *.d

