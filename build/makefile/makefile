#Nana C++ Library
#
#Makefile created by sarrow104(sarrow104@gmail.com)

GCC	= g++
INCROOT	= ../../include
SRCROOT	= ../../source
EXTRLIB = ../../extrlib
NANA_INC= $(INCROOT)/nana
OUTROOT = out
CXXFLAGS= -g -fexceptions -std=c++11 -Wall -Wextra -Wunused-variable -Wfatal-errors

INCS	= -I$(INCROOT) -I/usr/include/freetype2 -I$(EXTRLIB)

BIN	= libnana.a

TARGET  = ../bin/$(BIN)

.PHONY: all clean install print

all: $(TARGET)

define walk
$(wildcard $(1)) $(foreach e, $(wildcard $(1)/*), $(call walk, $(e)))
endef

SRCFILES := $(filter %.cpp,$(patsubst ./%,%,$(filter-out .,$(call walk, $(SRCROOT)))))

LINKOBJ	= $(patsubst $(SRCROOT)/%.cpp,$(OUTROOT)/objs/%.o,$(SRCFILES))

print:
	@echo $(LINKOBJ)

$(TARGET): $(LINKOBJ)
	mkdir -p $(dir $@)
	ar rus $@ $?
	ranlib $@

$(OUTROOT)/objs/%.o: $(SRCROOT)/%.cpp
	@mkdir -p $(dir $@)
	$(GCC) -o $@ -c $< $(INCS) $(CXXFLAGS)

clean:
	rm -f $(LINKOBJ)
	rm -f $(TARGET)

install:
	@mkdir -p $(INSTALL_PREFIX)/include
	@mkdir -p $(INSTALL_PREFIX)/bin
	@mkdir -p $(INSTALL_PREFIX)/lib
	cp -rfl $(INCROOT)/* $(INSTALL_PREFIX)/include
	cp -rfl $(TARGET) $(INSTALL_PREFIX)/lib/
