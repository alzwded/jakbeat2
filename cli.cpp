#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <limits.h>
#include <dlfcn.h>
//#include <libgen.h>
#include <string.h>
#include <err.h>

#include <engine.h>

extern "C" typedef void (*gui_fn)(Engine*, int, char**);

int main(int argc, char* argv[])
{
    if(argc <= 1) {
        printf("Usage: %s song [out.wav]\n", argv[0]);
        exit(2);
    }
    Engine engine(argv[1]);
#ifdef BUILDGUI
    if(argc == 2) {
        void* hgui = dlopen("libgui.so", RTLD_LAZY);
        if(!hgui) err(EXIT_FAILURE, "Failed to dlopen %s; reason: %s", "libgui.so", dlerror());
        gui_fn gui = (gui_fn)dlsym(hgui, "gui");
        if(!gui) err(EXIT_FAILURE, "Failed to dlsym 'gui'");
        gui(&engine, 1, argv);
        exit(0);
    }
#else
    // maybe add some ncurses GUI or CLI?
#endif
    if(argc != 3) {
        printf("Usage: %s song [out.wav]\n", argv[0]);
        exit(2);
    }
    printf("not implemented\n");
    exit(2);
}
