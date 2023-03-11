#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <limits.h>
#include <dlfcn.h>
#include <libgen.h>
#include <string.h>

#include <engine.h>

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
        char* pathToGui = dirname(buf);
        strcat(pathToGui, "/");
        strcat(pathToGui, "libgui.so");
        void* hgui = dlopen(pathToGui, RTLD_LAZY);
        void (*gui)(Engine*) = (void (*)(Engine*))dlsym(hgui, "gui");
        gui(&engine);
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
