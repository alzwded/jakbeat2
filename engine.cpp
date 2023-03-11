#include <engine.h>

#include <cmath>
#include <cstdio>
#include <err.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <cstdint>
#include <inttypes.h>

#define CURRENT_VERSION 1
#define MAX_PATTERN 16
#define FILE_SIZE ( sizeof(Engine::Globals) + MAX_PATTERN * sizeof(Engine::Pattern) + sizeof(Engine::Arrangement) )

#ifndef M_PI
static float M_PI = std::asin(1);
#endif

struct Impl
{
    static Impl* const Get(Engine* e) {
        return (Impl*)(e->pImpl);
    }

    int lockh = -1;
    int fd = -1;
    uint8_t* ptr = nullptr;

    bool use_arrangement = true;
    int t = 0;
    int arrangement_index = 0;
    int sample_rate = 44100;
};

Engine::Engine(const char* path)
    : pImpl(new Impl())
{
    auto* impl = Impl::Get(this);
    if(impl->fd = open(path, O_CREAT|O_RDWR, 0644);
            impl->fd == -1)
        err(EXIT_FAILURE, "can't open %s\n", path);
    off_t sz = lseek(impl->fd, 0, SEEK_END);
    if(sz < 4) {
        static const int cv = CURRENT_VERSION;
        ftruncate(impl->fd, FILE_SIZE);
        lseek(impl->fd, 0, SEEK_SET);
        write(impl->fd, &cv, sizeof(cv));
    }
    if(sz < FILE_SIZE) ftruncate(impl->fd, FILE_SIZE);
    if(sz > FILE_SIZE) warn("%s is larger (%d) than the expected %d\n",
            path,
            int(sz),
            int(FILE_SIZE));
    if(impl->lockh = flock(impl->fd, LOCK_EX);
            impl->lockh == -1)
        err(EXIT_FAILURE, "can't lock %s\n", path);
    if(impl->ptr = (uint8_t*)mmap(NULL, FILE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_POPULATE, impl->fd, 0);
            impl->ptr == MAP_FAILED)
        err(EXIT_FAILURE, "can't mmap %s\n", path);
    int version = *((int*)impl->ptr);
    if(version != CURRENT_VERSION)
        warn("%s's version %d is different to the current version %d\n",
                path,
                version,
                CURRENT_VERSION);
}

Engine::~Engine()
{
    auto* impl = Impl::Get(this);
    munmap(impl->ptr, FILE_SIZE);
    flock(impl->fd, LOCK_UN);
    close(impl->fd);
    delete ((Impl*)pImpl);
}

Engine::Globals* Engine::globals()
{
    return (Engine::Globals*)&(Impl::Get(this)->ptr[0]);
}

Engine::Pattern* Engine::pattern(int i)
{
    if(i >= MAX_PATTERN || i < 0) { abort(); }
    return (Engine::Pattern*)&(Impl::Get(this)->ptr[sizeof(Engine::Globals) + i * sizeof(Engine::Pattern)]);
}

Engine::Arrangement* Engine::arrangement()
{
    return (Engine::Arrangement*)&(Impl::Get(this)->ptr[sizeof(Engine::Globals) + MAX_PATTERN * sizeof(Engine::Pattern)]);
}

void Engine::set_sample_rate(int sr)
{
    Impl::Get(this)->sample_rate = sr;
}

void Engine::reset()
{
    Impl::Get(this)->t = 0;
    Impl::Get(this)->arrangement_index = 0;
}

float Engine::next()
{
    return std::atan(0.f * 2.f * M_PI);
}

void Engine::use_arrangement(bool use_it)
{
    Impl::Get(this)->use_arrangement = use_it;
}

bool Engine::use_arrangement()
{
    return Impl::Get(this)->use_arrangement;
}
