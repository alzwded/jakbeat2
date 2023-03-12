#include <engine.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Dial.H>
#include <FL/Fl_Slider.H>

#include <SDL/SDL.h>

#include <list>
#include <memory>
#include <functional>
#include <sstream>

#define BS 48
#define W 10
#define H 10

class MainWindow
: public Fl_Window
{
    struct Data
    {
        Engine* engine;
        enum { GLOBAL, LOCAL, ARRANGEMENT } type;
        union {
            struct {
                int* pattern;
                Engine::Control control;
            };
            Engine::Global global;
            int slot;
        };
        Data(Engine* ptr_, Engine::Global global_)
            : engine(ptr_)
            , type(GLOBAL)
            , global(global_)
        {}
        Data(Engine* ptr_, int* pattern_, Engine::Control control_)
            : engine(ptr_)
            , type(LOCAL)
            , pattern(pattern_)
            , control(control_)
        {}
        Data(Engine* ptr_, int arrangementSlot)
            : engine(ptr_)
            , type(ARRANGEMENT)
            , slot(arrangementSlot)
        {}
    };

    Engine* engine;
    int pattern64;
    bool arrangement, playing;
    std::list<std::unique_ptr<Fl_Widget>> owned;
    std::list<Fl_Widget*> group1;
    std::list<Fl_Widget*> group2;
    std::list<std::function<void()>> updateCallbacks;
    std::list<std::unique_ptr<Data>> datas;
    std::list<std::function<void()>> trash;

    Fl_Widget* AddKnob(int x, int y, const char* text, Engine::Global global)
    {
        datas.emplace_back(new Data(engine, global));
        auto* data = datas.back().get();

        Fl_Dial* dial = new Fl_Dial(x*BS, y*BS, BS, BS-8, text);
        dial->type(FL_NORMAL_DIAL);
        dial->minimum(0);
        dial->maximum(127);
        dial->step(1);
        dial->when(FL_WHEN_CHANGED);
        dial->callback([](Fl_Widget* widget, void* pdata) {
                auto* self = dynamic_cast<Fl_Dial*>(widget);
                auto* data = (Data*)pdata;
                auto& globals = *data->engine->globals();
                //printf("global %d old=%02x ", static_cast<int>(data->global), globals[static_cast<int>(data->global)]);
                globals[static_cast<int>(data->global)] = self->value();
                //printf("new=%02x\n", globals[static_cast<int>(data->global)]);
                }, data);
        dial->labelsize(8);
        updateCallbacks.emplace_back([dial, data]() {
                auto& globals = *data->engine->globals();
                //printf("global %d old=%02x ", static_cast<int>(data->global), globals[static_cast<int>(data->global)]);
                dial->value(globals[static_cast<int>(data->global)]);
                //printf("new=%02x\n", globals[static_cast<int>(data->global)]);
                });
        owned.emplace_back(dial);
        return dial;
    }

    Fl_Widget* AddSlider(int x0, int x1, int y, Engine::Control control)
    {
        datas.emplace_back(new Data(engine, &pattern64, control));
        auto* data = datas.back().get();

        Fl_Slider* slider = new Fl_Slider(x0 * BS, y * BS, (x1-x0+1)*BS, BS);
        slider->type(FL_HOR_NICE_SLIDER);
        slider->bounds(0, 127);
        slider->step(1);
        slider->when(FL_WHEN_CHANGED);
        slider->callback([](Fl_Widget* widget, void* pdata) {
                auto* self = dynamic_cast<Fl_Slider*>(widget);
                auto* data = (Data*)pdata;
                auto& pattern = *data->engine->pattern(*data->pattern);
                //printf("global %d old=%02x ", static_cast<int>(data->global), globals[static_cast<int>(data->global)]);
                pattern[static_cast<int>(data->control)] = self->value();
                //printf("new=%02x\n", globals[static_cast<int>(data->global)]);
                }, data);
        updateCallbacks.emplace_back([slider, data]() {
                auto& pattern = *data->engine->pattern(*data->pattern);
                //printf("global %d old=%02x ", static_cast<int>(data->global), globals[static_cast<int>(data->global)]);
                slider->value(pattern[static_cast<int>(data->control)]);
                //printf("new=%02x\n", globals[static_cast<int>(data->global)]);
                });
        owned.emplace_back(slider);
        return slider;
    }

public:
    MainWindow(Engine* engine_)
        : Fl_Window(W*BS, H*BS)
        , engine(engine_)
        , pattern64(0)
        , arrangement(false)
        , playing(false)
        , owned()
        , group1()
        , group2()
        , updateCallbacks()
        , datas()
        , trash()
    {
        label("JakBeat2");
        // always visible
        (void) AddKnob(1, 0, "VOL", Engine::Global::VOLUME);
        (void) AddKnob(2, 0, "FLT", Engine::Global::FILTER);
        (void) AddKnob(3, 0, "BLND", Engine::Global::BLEND);
        (void) AddKnob(4, 0, "TEMPO", Engine::Global::TEMPO);

        // non-arrangement mode
        {
            Fl_Box *box = new Fl_Box(FL_NO_BOX, 0, 0, BS, BS, "00");
            box->labeltype(FL_NORMAL_LABEL);
            box->align(FL_ALIGN_CENTER);
            box->labelfont(FL_COURIER);
            char* s = (char*)malloc(4); s[0] = '1'; s[1] = ' '; s[2] = '1'; s[3] = '\0';
            updateCallbacks.emplace_back([this, box, s]() {
                    s[0] = '1' + (pattern64>>3);
                    s[2] = '1' + (pattern64&0x7);
                    box->label(s);
                    });
            owned.emplace_back(box);
            trash.emplace_back([s]() {
                    free(s);
                    });
            group1.push_back(box);
        }
        for(int i = 0; i < 8; ++i) {
            {
                char* s = strdup("N#0");
                s[2] = '0' + i + 1;
                trash.emplace_back([s]() { free(s); });
                Fl_Box* box = new Fl_Box(FL_NO_BOX, 0, (2+i)*BS, BS, BS, s);
                group1.push_back(box);
                owned.emplace_back(box);
            }
            group1.push_back(AddKnob(1, 2+i, "VOL", static_cast<Engine::Global>(static_cast<int>(Engine::Global::N1VOLUME) + 4 * i + 0)));
            group1.push_back(AddKnob(2, 2+i, "LP",  static_cast<Engine::Global>(static_cast<int>(Engine::Global::N1VOLUME) + 4 * i + 1)));
            group1.push_back(AddKnob(3, 2+i, "HP",  static_cast<Engine::Global>(static_cast<int>(Engine::Global::N1VOLUME) + 4 * i + 2)));
            group1.push_back(AddKnob(4, 2+i, "LEN", static_cast<Engine::Global>(static_cast<int>(Engine::Global::N1VOLUME) + 4 * i + 3)));
            group1.push_back(AddSlider(5, 9, 2+i, static_cast<Engine::Control>(static_cast<int>(Engine::Control::N1WHEN) + i)));
        }
        // osc1 (sq1)
        {
            Fl_Box* box = new Fl_Box(FL_NO_BOX, 0, BS, BS, BS, "OSC1");
            group1.push_back(box);
            owned.emplace_back(box);
        }
        // TODO note
        // osc2 (sq2)
        {
            Fl_Box* box = new Fl_Box(FL_NO_BOX, 3*BS, BS, BS, BS, "OSC2");
            group1.push_back(box);
            owned.emplace_back(box);
        }
        // TODO note
        // osc3 (tr)
        {
            Fl_Box* box = new Fl_Box(FL_NO_BOX, 6*BS, BS, BS, BS, "BASS");
            group1.push_back(box);
            owned.emplace_back(box);
        }
        // TODO note
        AddKnob(5, 0, "BASS", Engine::Global::TRVOL);
        AddKnob(6, 0, "O1 LP", Engine::Global::SQ1LP);
        AddKnob(7, 0, "O1 HP", Engine::Global::SQ1HP);
        AddKnob(8, 0, "O2 LP", Engine::Global::SQ2LP);
        AddKnob(9, 0, "O2 HP", Engine::Global::SQ2HP);


        // arrangement mode
        {
            Fl_Box *box = new Fl_Box(FL_NO_BOX, 0, 0, BS, BS, "ARR");
            box->labeltype(FL_NORMAL_LABEL);
            box->align(FL_ALIGN_CENTER);
            box->labelfont(FL_COURIER);
            group2.push_back(box);
            owned.emplace_back(box);
        }
        for(int i = 0; i < 64; ++i) {
            int row = i >> 3;
            int col = i & 0x7;
            // TODO bank/preset selector
        }

        // refresh
        std::for_each(updateCallbacks.begin(), updateCallbacks.end(), [](std::function<void()> const& fn) { fn(); });
        std::for_each(group1.begin(), group1.end(), [](Fl_Widget* w) { w->show(); });
        std::for_each(group2.begin(), group2.end(), [](Fl_Widget* w) { w->hide(); });
        end();

        // TEST
        //SDL_AudioSpec obtained;
        SDL_AudioSpec request;
        memset(&request, 0, sizeof(SDL_AudioSpec));
        request.freq = 44100;
        request.format = AUDIO_S16;
        request.samples = 512;
        request.channels = 0;
        request.callback = audio_callback;
        request.userdata = this;
        SDL_OpenAudio(&request, NULL); // force my format
        SDL_PauseAudio(1);
        playing = false;

        engine->set_sample_rate(44100);
        //engine->set_sample_rate(obtained.freq);
        engine->use_arrangement(false);
    }

    static void audio_callback(void* userdata, Uint8* stream, int nstream)
    {
        MainWindow* self = (MainWindow*)userdata;

        int16_t* samples = (int16_t*)stream;
        int nsamples = nstream/2;

        for(int i = 0; i < nsamples; ++i) {
            float smp = self->engine->next();
            samples[i] = 32767 * std::max(-1.f, std::min(1.f, smp));
        }
    }

    int handle(int event)
    {
        switch(event) {
            case FL_KEYDOWN:
                switch(Fl::event_key()) {
                    case FL_F+1:
                    case FL_F+2:
                    case FL_F+3:
                    case FL_F+4:
                    case FL_F+5:
                    case FL_F+6:
                    case FL_F+7:
                    case FL_F+8:
                        {
                            int k = Fl::event_key() - FL_F - 1;
                            pattern64 = ((pattern64 & 0x7) | (k << 3));
                            (void) engine->pattern(pattern64);
                            //printf("Bank %d off %02x\n", k, pattern64);
                            std::for_each(updateCallbacks.begin(), updateCallbacks.end(), [](std::function<void()> const& fn) { fn(); });
                        }
                        break;
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                        {
                            int k = Fl::event_key() - '1';
                            pattern64 = ((pattern64 & ~0x7) | (k));
                            (void) engine->pattern(pattern64);
                            //printf("Pattern %d off %02x\n", k, pattern64);
                            std::for_each(updateCallbacks.begin(), updateCallbacks.end(), [](std::function<void()> const& fn) { fn(); });
                        }
                        break;
                    case ' ':
                        playing = !playing;
                        if(playing) engine->reset();
                        SDL_PauseAudio(!playing);
                        break;
                    case FL_Tab:
                        if(arrangement = !arrangement;
                                arrangement)
                        {
                            std::for_each(group1.begin(), group1.end(), [](Fl_Widget* w) { w->hide(); });
                            std::for_each(group2.begin(), group2.end(), [](Fl_Widget* w) { w->show(); });
                        } else {
                            std::for_each(group1.begin(), group1.end(), [](Fl_Widget* w) { w->show(); });
                            std::for_each(group2.begin(), group2.end(), [](Fl_Widget* w) { w->hide(); });
                        }
                        break;
                    case FL_Delete:
                        if(arrangement) {
                            auto& arrangement = *engine->arrangement();
                            for(int i = 0; i < 64; ++i) {
                                arrangement[i] = 0;
                            }
                        } else {
                            auto& pattern = *engine->pattern(pattern64);
                            for(int i = 0; i < static_cast<int>(Engine::Control::_SIZE_); ++i) {
                                pattern[i] = 0;
                            }
                        }
                        std::for_each(updateCallbacks.begin(), updateCallbacks.end(), [](std::function<void()> const& fn) { fn(); });
                        break;
                }// switch Fl::event_Key()
                return 1;
            default:
                return Fl_Window::handle(event);
        }
    }
};

extern "C" void gui(Engine* engine, int argc, char* argv[])
{
    // Fl::gl_visual(FL_RGB); // to enable OGL
    Fl_Window *window = new MainWindow(engine);
    window->show(argc, argv);
    exit(Fl::run());
}
