#include <engine.h>

struct Impl
{
};

Engine::Engine(const char* path)
{
    pImpl = new Impl();
}

Engine::~Engine()
{
    delete ((Impl*)pImpl);
}
