Building Nana C++ Library
requires:
X11, pthread, Xpm, rt, dl, freetype2, Xft, fontconfig, ALSA

Writing a makefile for creating applications with Nana C++ Library
-------------------
```
GCC	= g++
NANAPATH = [The folder of Nana C++ Library]
BIN	= [The bin file what you want to create.]
SOURCES = [The source file of your application.]

NANAINC	= $(NANAPATH)/include
NANALIB = $(NANAPATH)/build/bin

INCS	= -I$(NANAINC)
LIBS	= -L$(NANALIB) -lnana -lX11 -lpthread -lrt -lXft -lpng -lasound

LINKOBJ	= $(SOURCES:.cpp=.o)

$(BIN): $(LINKOBJ) $(NANALIB)/libnana.a
	$(GCC) $(LINKOBJ) $(INCS) $(LIBS) -o $(BIN) -std=c++0x

.cpp.o:
	$(GCC) -g -c $< -o $@ $(INCS) -std=c++0x

$(NANALIB):
	make -f $(NANAPATH)/build/makefile/makefile

clean:
	rm -f $(LINKOBJ)
```
-------------------
