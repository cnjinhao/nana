#Nana C++ Library
#
#Makefile created by Jinhao(cnjinhao@hotmail.com)

GCC	= g++
INCROOT	= ../../include
SRCROOT	= ../../source
EXTRLIB = ../../extrlib
NANA_INC= $(INCROOT)/nana

INCS	= -I$(INCROOT) -I/usr/include/freetype2 -I$(EXTRLIB)

BIN	= libnana.a

SRC_NANA = $(wildcard $(SRCROOT)/*.cpp)
SRC_DETAIL = $(wildcard $(SRCROOT)/detail/*.cpp)
SRC_FILESYSTEM = $(wildcard $(SRCROOT)/filesystem/*.cpp)
SRC_AUDIO = $(wildcard $(SRCROOT)/audio/*.cpp)
SRC_AUDIO_DETAIL = $(wildcard $(SRCROOT)/audio/detail/*.cpp)
SRC_GUI = $(wildcard $(SRCROOT)/gui/*.cpp)
SRC_GUI_DETAIL = $(wildcard $(SRCROOT)/gui/detail/*.cpp)
SRC_GUI_WIDGETS = $(wildcard $(SRCROOT)/gui/widgets/*.cpp)
SRC_GUI_WIDGETS_SKELETONS = $(wildcard $(SRCROOT)/gui/widgets/skeletons/*.cpp)
SRC_PAINT = $(wildcard $(SRCROOT)/paint/*.cpp)
SRC_PAINT_DETAIL = $(wildcard $(SRCROOT)/paint/detail/*.cpp)
SRC_SYSTEM = $(wildcard $(SRCROOT)/system/*.cpp)
SRC_THREADS= $(wildcard $(SRCROOT)/threads/*.cpp)

SOURCES = $(SRC_NANA) $(SRC_DETAIL) $(SRC_FILESYSTEM) $(SRC_AUDIO) $(SRC_AUDIO_DETAIL) $(SRC_GUI) $(SRC_GUI_DETAIL) $(SRC_GUI_WIDGETS) $(SRC_GUI_WIDGETS_SKELETONS) $(SRC_PAINT) $(SRC_PAINT_DETAIL) $(SRC_SYSTEM) $(SRC_THREADS)

LINKOBJ	= $(SOURCES:.cpp=.o)

$(BIN): $(LINKOBJ)
	ar r ../bin/$(BIN) $(LINKOBJ)
	ranlib ../bin/$(BIN)

.cpp.o:
	$(GCC) -g -c $< -o $@ $(INCS) -std=c++11 -Wall

clean:
	rm -f $(LINKOBJ)
	rm -f ../bin/$(BIN)

