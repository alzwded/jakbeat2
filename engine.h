#ifndef ENGINE_H
#define ENGINE_H

#include <cstddef>

struct __attribute__((visibility("default"))) Engine
{
    Engine(const char* path);
    ~Engine();

    enum class Global {
        VERSION,
        VOLUME,
        FILTER,
        BLEND,
        TEMPO,
        N1LP, N1HP, N1SAG,
        N2LP, N2HP, N2SAG,
        N3LP, N3HP, N3SAG,
        N4LP, N4HP, N4SAG,
        N5LP, N5HP, N5SAG,
        N6LP, N6HP, N6SAG,
        N7LP, N7HP, N7SAG,
        N8LP, N8HP, N8SAG,
        SQ1LP, SQ1HP,
        SQ2LP, SQ2HP,
        TRVOL,

        _SIZE_
    };

    enum class Control {
        N1VOLUME, N1WHEN,
        N2VOLUME, N2WHEN,
        N3VOLUME, N3WHEN,
        N4VOLUME, N4WHEN,
        N5VOLUME, N5WHEN,
        N6VOLUME, N6WHEN,
        N7VOLUME, N7WHEN,
        N8VOLUME, N8WHEN,
        SQ1, SQ2, TR,

        _SIZE_
    };

    typedef int Pattern[32];
    static_assert(sizeof(Pattern)/sizeof(int) >= static_cast<size_t>(Control::_SIZE_), "bad size");

    typedef int Globals[64];
    static_assert(sizeof(Globals)/sizeof(int) >= static_cast<size_t>(Global::_SIZE_), "bad size");

    typedef int Arrangement[64];

    Globals* globals();
    Pattern* pattern(int i);
    Arrangement* arrangement();

    void set_sample_rate(int sr);
    void reset();
    float next();

    void use_arrangement(bool use_it);
    bool use_arrangement();

private:
    friend class Impl;
    void* pImpl;
};

#endif
