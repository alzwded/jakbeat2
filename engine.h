#ifndef ENGINE_H
#define ENGINE_H

struct __attribute__((visibility("default"))) Engine
{
    Engine(const char* path);
    ~Engine();
private:
    void* pImpl;
};

#endif
