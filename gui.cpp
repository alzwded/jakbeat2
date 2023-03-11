#include <engine.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Dial.H>

extern "C" void gui(Engine* engine, int argc, char* argv[])
{
    // Fl::gl_visual(FL_RGB); // to enable OGL
    Fl_Window *window = new Fl_Window(480,480);
    Fl_Dial* dial = new Fl_Dial(48, 48, 48, 48, "VOL");
    dial->type(FL_NORMAL_DIAL);
    dial->minimum(0);
    dial->maximum(127);
    //Fl_Box *box = new Fl_Box(20,40,300,100,"Hello, World!");
    //dial->labelfont(FL_BOLD+FL_ITALIC);
    dial->labelsize(8);
    //dial->labeltype(FL_SHADOW_LABEL);
    window->end();
    window->show(argc, argv);
    exit(Fl::run());
}
