# Building Nana C++ Library directly with make
If you are using make directly, it require:
X11, pthread, Xpm, rt, dl, freetype2, Xft, fontconfig, ALSA

Example (Ubuntu):
```
sudo apt install libxft-dev
sudo apt install libfreetype2-dev
```
Some Debian Linux distro use apt-get instead of apt. 
These commands will install both the library and their headers. 


Example of writing a makefile for creating applications with Nana C++ Library
-------------------
```
GCC	= g++
NANAPATH = [The folder of Nana C++ Library]
BIN	= [The bin file what you want to create.]
SOURCES = [The source file of your application.]

NANAINC	= $(NANAPATH)/include
NANALIB = $(NANAPATH)/build/bin

INCS	= -I$(NANAINC)
LIBS	= -L$(NANALIB) -lnana -lX11 -lXcursor -lpthread -lrt -lXft -lpng -lasound -lfontconfig -lstdc++fs

LINKOBJ	= $(SOURCES:.cpp=.o)

$(BIN): $(LINKOBJ) $(NANALIB)/libnana.a
	$(GCC) $(LINKOBJ) $(INCS) $(LIBS) -o $(BIN)

.cpp.o:
	$(GCC) -g -c $< -o $@ $(INCS) -std=c++17

$(NANALIB):
	make -f $(NANAPATH)/build/makefile/makefile

clean:
	rm -f $(LINKOBJ)
```
-------------------
