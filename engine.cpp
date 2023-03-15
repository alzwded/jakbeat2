#include <engine.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <inttypes.h>

#include <err.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <unistd.h>

#define CURRENT_VERSION 1
#define MAX_PATTERN 64
#define FILE_SIZE ( sizeof(Engine::Globals) + MAX_PATTERN * sizeof(Engine::Pattern) + sizeof(Engine::Arrangement) )

#ifndef M_PI
static float M_PI = std::asin(1);
#endif

struct Noise {
    unsigned reg = 0xA001;

    float next() {
        if(reg & 0x1) reg = (reg >> 1) ^ 0xA801;
        else reg >>= 1;
        return float(reg) / float(0xFFFF) - 0.5f;
    }
};

struct HP {
    Engine* engine;
    int control = static_cast<int>(Engine::Global::N1HP);

    float y1 = 0, x1 = 0;

    float next(float x) {
        //float fr = (*engine->globals())[control] / 127.f * 12000.f;
        float fr = (7.f - std::log(128.f - (*engine->globals())[control])/std::log(2)) / 7.f * 16000.f;
        float a = 1.f / (2.f * M_PI * (1.f / engine->get_sample_rate()) * fr + 1.f);
        float o = a * y1 + a * (x - x1);
        y1 = o;
        x1 = x;
        return o;
    }
};

struct LP {
    Engine* engine;
    int control = static_cast<int>(Engine::Global::N1LP);

    float y1 = 0;

    float next(float x) {
        //float fr = (*engine->globals())[control] / 127.f * 12000.f;
        float fr = (7.f - std::log(128.f - (*engine->globals())[control])/std::log(2)) / 7.f * 16000.f;
        //if(control == static_cast<int>(Engine::Global::FILTER)) printf("%g\n", fr);
        float rc = 1.f / (2.f * M_PI * fr);
        float dt = (1.f / engine->get_sample_rate());
        float a = dt / (dt + rc);
        float o = (1-a) * y1 + a * x;
        y1 = o;
        return o;
    }
};

struct Decay {
    Engine* engine;
    int when;
    int decay;
    int volume;
    int on;

    int counter;
    bool active;
    float next(int t, int period) {
        int O = (*engine->pattern())[on];
        if(!O) {
            counter = 0;
            active = false;
            return 0.f;
        }

        int A = (*engine->pattern())[when] * period / 127;
        int N = (*engine->globals())[decay] * engine->get_sample_rate() / 127;
        float V = (*engine->globals())[volume] / 127.f;
        //printf("%p: A=%d N=%d t=%d period=%d V=%f\n",
        //        this,
        //        A, N, t, period, V);
        //printf("when = %d decay = %d volume = %d\n",
        //        when, decay, volume);
        //printf("when raw = %d decay raw = %d volume raw = %d\n",
        //    (*engine->globals())[when],
        //    (*engine->pattern())[decay],
        //    (*engine->globals())[volume]);
        if(!active) {
            if(t == A) {
                active = true;
                counter = 0;
            } else {
                return 0.f;
            }
        }
        if(active) {
            ++counter;
            if(counter < N) {
                return (1.f - float(counter)/float(N)) * V;
            } else {
                active = false;
                counter = 0;
                return 0.f;
            }
        }
        return 0.f;
    }
};

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
    int current_pattern = 0;

    Noise noise;
    Decay decays[8];
    HP HPs[8];
    LP LPs[8];
    LP globalLP;
    LP LPSQs[2];
    HP HPSQs[2];
};

Engine::Engine(const char* path)
    : pImpl(new Impl())
{
    auto* impl = Impl::Get(this);
    if(impl->fd = open(path, O_CREAT|O_RDWR, 0644);
            impl->fd == -1)
        err(EXIT_FAILURE, "can't open %s\n", path);
    off_t sz = lseek(impl->fd, 0, SEEK_END);
    bool reinit = false;
    static const int cv = CURRENT_VERSION;
    if(sz < 4) {
        ftruncate(impl->fd, FILE_SIZE);
        lseek(impl->fd, 0, SEEK_SET);
        write(impl->fd, &cv, 4);
        reinit = true;
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
    if(reinit) {
        std::memset(impl->ptr, 0, FILE_SIZE);
        memcpy(impl->ptr, &cv, 4);

        auto& g = *globals();
        g[static_cast<int>(Global::VOLUME)] = 64;
        g[static_cast<int>(Global::FILTER)] = 127;
        g[static_cast<int>(Global::BLEND)] = 0;
        // 0..127     44100..8*44100; 120bpm = 2s = 127/8
        // 0..127     32..256 bpm
        // 120bpm = (120-32)/256 * 127
        g[static_cast<int>(Global::TEMPO)] = (120-32)*127/256 + 1;

        for(int i = 0; i < 8; ++i) {
            g[static_cast<int>(Global::N1LP) + 4*i] = 127;
        }
        g[static_cast<int>(Global::N1VOLUME)] = 127;
        g[static_cast<int>(Global::N1ENVELOPE)] = 16;

        for(int i = 0; i < MAX_PATTERN; ++i) {
            auto& p = *pattern(i);
            p[static_cast<int>(Control::N1ON)] = 1;
            p[static_cast<int>(Control::N2ON)] = 0;
            p[static_cast<int>(Control::N3ON)] = 0;
            p[static_cast<int>(Control::N4ON)] = 0;
            p[static_cast<int>(Control::N5ON)] = 0;
            p[static_cast<int>(Control::N6ON)] = 0;
            p[static_cast<int>(Control::N7ON)] = 0;
            p[static_cast<int>(Control::N8ON)] = 0;
            p[static_cast<int>(Control::SQ1ON)] = 1;
            p[static_cast<int>(Control::SQ2ON)] = 1;
            p[static_cast<int>(Control::TRON)] = 0;
            p[static_cast<int>(Control::TR)] = 16;
            p[static_cast<int>(Control::SQ1)] = 28;
            p[static_cast<int>(Control::SQ2)] = 32;
        }

        for(int i = 0; i < 64; ++i) {
            auto& a = *arrangement();
            a[i] = -1;
        }

        g[static_cast<int>(Global::SQ1LP)] = 64;
        g[static_cast<int>(Global::SQ2LP)] = 64;
    }

    // initialize crap
    for(int i = 0; i < 8; ++i) {
        impl->decays[i].engine = this;
        impl->decays[i].when = static_cast<int>(Control::N1WHEN) + i;
        impl->decays[i].decay = static_cast<int>(Global::N1ENVELOPE) + i * 4;
        impl->decays[i].volume = static_cast<int>(Global::N1VOLUME) + i * 4;
        impl->decays[i].on = static_cast<int>(Control::N1ON) + i;
        impl->HPs[i].engine = this;
        impl->HPs[i].control = static_cast<int>(Global::N1HP) + 4*i;
        impl->LPs[i].engine = this;
        impl->LPs[i].control = static_cast<int>(Global::N1LP) + 4*i;
    }
    impl->globalLP.engine = this;
    impl->globalLP.control = static_cast<int>(Global::FILTER);
    impl->LPSQs[0].engine = this;
    impl->LPSQs[0].control = static_cast<int>(Global::SQ1LP);
    impl->LPSQs[1].engine = this;
    impl->LPSQs[1].control = static_cast<int>(Global::SQ2LP);
    impl->HPSQs[0].engine = this;
    impl->HPSQs[0].control = static_cast<int>(Global::SQ1HP);
    impl->HPSQs[1].engine = this;
    impl->HPSQs[1].control = static_cast<int>(Global::SQ2HP);
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
    Impl::Get(this)->current_pattern = i;
    return (Engine::Pattern*)&(Impl::Get(this)->ptr[sizeof(Engine::Globals) + i * sizeof(Engine::Pattern)]);
}

Engine::Pattern* Engine::pattern()
{
    auto* impl = Impl::Get(this);

    if(impl->use_arrangement) {
        return pattern((*arrangement())[impl->arrangement_index]);
    } else {
        return pattern(impl->current_pattern);
    }

}

Engine::Arrangement* Engine::arrangement()
{
    return (Engine::Arrangement*)&(Impl::Get(this)->ptr[sizeof(Engine::Globals) + MAX_PATTERN * sizeof(Engine::Pattern)]);
}

void Engine::set_sample_rate(int sr)
{
    Impl::Get(this)->sample_rate = sr;
}

int Engine::get_sample_rate()
{
    return Impl::Get(this)->sample_rate;
}

void Engine::reset()
{
    Impl::Get(this)->t = 0;
    Impl::Get(this)->arrangement_index = 0;
}

float Engine::next()
{
    auto* impl = Impl::Get(this);

    if(impl->use_arrangement) {
        int cnt = 64;
        while((*arrangement())[impl->arrangement_index] < 0) {
            impl->arrangement_index++;
            impl->arrangement_index &= 0x3F;
            cnt--;
        }
        if(cnt <= 0) return 0.f;
        pattern((*arrangement())[impl->arrangement_index]);
    }

    // tempo
    //       0 ...... 8s ........ 8 * sr
    //       127 .... 1s ........ 1 * sr
    int period = std::round(((1.f - (*globals())[static_cast<int>(Global::TEMPO)] / 127.f) * 7.f + 1.f) * impl->sample_rate);

    float sum = 0.f;

    float nsample = impl->noise.next();

    // add noises
    for(int i = 0; i < 8; ++i) {
        float smpl = impl->decays[i].next(impl->t, period) * nsample;
        smpl = impl->HPs[i].next(smpl);
        smpl = impl->LPs[i].next(smpl);
        sum += smpl;
    }
    // apply blend
    sum *= 1.f - (*globals())[static_cast<int>(Global::BLEND)] / 127.f;

    // add squares
    float sqsum = 0.f;
    // TODO samples
    // apply blend
    sqsum *= (*globals())[static_cast<int>(Global::BLEND)] / 127.f;
    sum += sqsum;

    // add bass
    // TODO

    // apply global lowpass
    sum = impl->globalLP.next(sum);

    // apply global volume
    sum *= (*globals())[static_cast<int>(Global::VOLUME)] / 127.f;

    impl->t++;
    if(impl->t >= period) {
        if(impl->use_arrangement) {
            impl->arrangement_index++;
            impl->arrangement_index &= 0x3F;
        }
        impl->t = 0;
    }
    // mix and amp distortion
    return std::atan(sum * M_PI/2.f);
}

void Engine::use_arrangement(bool use_it)
{
    Impl::Get(this)->use_arrangement = use_it;
}

bool Engine::use_arrangement()
{
    return Impl::Get(this)->use_arrangement;
}
