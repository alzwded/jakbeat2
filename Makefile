BUILDGUI ?= 1
CXXFLAGS ?= -O3 -march=nehalem

ALL = jb2 libengine.a

ifeq ($(BUILDGUI), 1)
ALL += libgui.so
CXXFLAGS += -DBUILDGUI=1
endif

all: $(ALL)

clean:
	rm -rf libgui.so *.o jb2 libengine.a

.PHONY: clean

engine.o: engine.cpp engine.h
	g++ --std=c++17 $(CXXFLAGS) -c -o engine.o -I. -fPIC engine.cpp -lm

libengine.a: engine.o
	ar -crs libengine.a engine.o

jb2: cli.cpp engine.h libengine.a
	g++ --std=c++17 $(CXXFLAGS) -o jb2 -I. -fPIC cli.cpp  -ldl -L. -lengine -Wl,-rpath,'$$'ORIGIN -Wl,-rpath-link,'$$'ORIGIN

ifeq ($(BUILDGUI), 1)
libgui.so: gui.cpp engine.h libengine.a
	g++ --std=c++17 $(CXXFLAGS) -I. -L. -shared -fPIC -o libgui.so `fltk-config --use-gl --cxxflags` `pkg-config --cflags sdl` gui.cpp `fltk-config --use-gl --ldflags` `pkg-config --libs sdl` -lengine
endif

