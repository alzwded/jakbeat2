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
        N1VOLUME, N1LP, N1HP, N1ENVELOPE,
        N2VOLUME, N2LP, N2HP, N2ENVELOPE,
        N3VOLUME, N3LP, N3HP, N3ENVELOPE,
        N4VOLUME, N4LP, N4HP, N4ENVELOPE,
        N5VOLUME, N5LP, N5HP, N5ENVELOPE,
        N6VOLUME, N6LP, N6HP, N6ENVELOPE,
        N7VOLUME, N7LP, N7HP, N7ENVELOPE,
        N8VOLUME, N8LP, N8HP, N8ENVELOPE,
        SQ1LP, SQ1HP,
        SQ2LP, SQ2HP,
        TRVOL,

        _SIZE_
    };

    enum class Control {
        N1WHEN,
        N2WHEN,
        N3WHEN,
        N4WHEN,
        N5WHEN,
        N6WHEN,
        N7WHEN,
        N8WHEN,
        SQ1,
        SQ2,
        TR,
        N1ON,
        N2ON,
        N3ON,
        N4ON,
        N5ON,
        N6ON,
        N7ON,
        N8ON,
        SQ1ON,
        SQ2ON,
        TRON,

        _SIZE_
    };

    typedef int Pattern[32];
    static_assert(sizeof(Pattern)/sizeof(int) >= static_cast<size_t>(Control::_SIZE_), "bad size");

    typedef int Globals[64];
    static_assert(sizeof(Globals)/sizeof(int) >= static_cast<size_t>(Global::_SIZE_), "bad size");

    typedef int Arrangement[64];

    Globals* globals();
    Pattern* pattern(int i);
    Pattern* pattern();
    Arrangement* arrangement();

    void set_sample_rate(int sr);
    int get_sample_rate();
    void reset();
    float next();

    void use_arrangement(bool use_it);
    bool use_arrangement();

private:
    friend class Impl;
    void* pImpl;
};

#endif
