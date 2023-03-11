#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <limits.h>
#include <dlfcn.h>
#include <libgen.h>
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
        char buf[PATH_MAX];
        ssize_t nbuf = readlink("/proc/self/exe", buf, sizeof(buf));
        if(nbuf == -1) err(EXIT_FAILURE, "Failed to read /proc/self/exe");
        char* pathToGui = dirname(buf);
        strcat(pathToGui, "/");
        strcat(pathToGui, "libgui.so");
        void* hgui = dlopen(pathToGui, RTLD_LAZY);
        if(!hgui) err(EXIT_FAILURE, "Failed to dlopen %s; reason: %s", pathToGui, dlerror());
        gui_fn gui = (gui_fn)dlsym(hgui, "gui");
        if(!gui) err(EXIT_FAILURE, "Failed to dlsym 'gui'");
        gui(&engine, 1, argv);
        exit(0);
    }
#endif
    if(argc != 3) {
        printf("Usage: %s song [out.wav]\n", argv[0]);
        exit(2);
    }
    printf("not implemented\n");
    exit(2);
}
