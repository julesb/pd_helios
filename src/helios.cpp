/*
 * pd_helios
 * � 2019 Tim Redfern
 * licenced under the LGPL v3.0
 * see LICENSE
 */

extern "C" {
  void helios_setup(void);
}

#include "m_pd.h"

#include "pd_helios.h"

using namespace std;

static t_class *helios_class;

typedef struct _helios {
  t_object  x_obj;       
  Helios *helios;
  t_outlet *f_out;
} t_helios;

void helios_bang(t_helios *x)
{
  t_float f=0.0f;
  
  outlet_float(x->f_out, f);
}

static void helios_list(t_helios *x, t_symbol *s, int argc, t_atom *argv)
{
  if (argc==1){
    x->helios->set_enabled(atom_getint(&argv[0]));

    //on enable, redraw and emit the number of pounts
    if (atom_getint(&argv[0])){
      int num_drawn = x->helios->draw();

      outlet_float(x->f_out, (float)num_drawn);
    }
    return;
  }

    std::vector <point> points;

    for (int i=0;i<argc/5;i++){
      points.push_back(point{
        atom_getfloat(&argv[i*5]),
        atom_getfloat(&argv[i*5+1]),
        (uint8_t)atom_getfloat(&argv[i*5+2]),
        (uint8_t)atom_getfloat(&argv[i*5+3]),
        (uint8_t)atom_getfloat(&argv[i*5+4])
      });

    }

    int num_drawn = x->helios->draw(points);

    outlet_float(x->f_out, (float)num_drawn);
}

static void redraw(t_helios *x){
  int num_drawn = 0;
  if (x->helios->get_rawmode() == 0) {
    num_drawn = x->helios->draw();
  }
  else {
    num_drawn = x->helios->draw_raw();
  }
  outlet_float(x->f_out, (float)num_drawn);
}

void intensity_set(t_helios *x, t_floatarg f)
{
  int intensity=min(255,max(0,(int)f));
  x->helios->set_intensity(intensity);
  redraw(x);
}

void maxangle_set(t_helios *x, t_floatarg f)
{
  float maxangle=min(90,max(0,(int)f));
  x->helios->set_maxangle(maxangle);
  redraw(x);
}

void subdivide_set(t_helios *x, t_floatarg f)
{
  int subdivide=min(255,max(1,(int)f));
  x->helios->set_subdivide(subdivide);
  redraw(x);
}

void blanknum_set(t_helios *x, t_floatarg f)
{
  int blanknum=min(255,max(0,(int)f));
  x->helios->set_blanknum(blanknum);
  redraw(x);
}

void rawmode_set(t_helios *x, t_floatarg f)
{
  int rawmode=min(1,max(0,(int)f));
  x->helios->set_rawmode(rawmode);
  post("rawmode: %d", rawmode);
  redraw(x);
}

void maxstatuspoll_set(t_helios *x, t_floatarg f)
{
  int maxpoll=max(0,(int)f);
  x->helios->set_maxstatuspoll(maxpoll);
  post("maxstatuspoll: %d", maxpoll);
  redraw(x);
}

void startimmediately_set(t_helios *x, t_floatarg f)
{
    int startimmediately = min(1, max(0, (int)f));
    x->helios->set_startimmediately(startimmediately);
    post("startimmediately: %d", startimmediately);
    redraw(x);
}
void singlemode_set(t_helios *x, t_floatarg f)
{
    int singlemode = min(1, max(0, (int)f));
    x->helios->set_singlemode(singlemode);
    post("singlemode: %d", singlemode);
    redraw(x);
}
void dontblock_set(t_helios *x, t_floatarg f)
{
    int dontblock = min(1, max(0, (int)f));
    x->helios->set_dontblock(dontblock);
    post("dontblock: %d", dontblock);
    redraw(x);
}
void pps_set(t_helios *x, t_floatarg f)
{
  int newpps=min(0xffff, max(10000,(int)f));
  x->helios->set_pps(newpps);
  post("pps: %d", newpps);
  //redraw(x);
}



void helios_free(t_helios *x)
{
  delete x->helios;
}

void *helios_new(t_symbol *s, int argc, t_atom *argv)
{
  t_helios *x = (t_helios *)pd_new(helios_class);

  post("pd_helios new: opening devices");

  /* depending on the number of arguments we interprete them differently */
  switch(argc){
    default:
    case 2:{
      t_float f2=atom_getfloat(argv+1);
    }
    case 1:{
      int pps=atom_getint(argv);
      x->helios=new Helios(pps=pps);
      break;
    }
    case 0:
      x->helios=new Helios();
      break;
  }


  x->f_out = outlet_new(&x->x_obj, &s_float);

  return (void *)x;
}


/**
 * define the function-space of the class
 */
void helios_setup(void) {

  post("pd_helios setup: starting");

  helios_class = class_new(gensym("helios"),
                            (t_newmethod)helios_new,
                            (t_method)helios_free,
                            sizeof(t_helios),
                            CLASS_DEFAULT, 
                            A_GIMME, /* an arbitrary number of arguments 
                                      * which are of arbitrary type */
                            0);

  /* call a function when a "bang" message appears on the first inlet */
  class_addbang(helios_class, helios_bang); 

  class_addlist(helios_class, helios_list);

  class_addmethod(helios_class,
        (t_method)pps_set, gensym("pps"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)intensity_set, gensym("intensity"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)maxangle_set, gensym("maxangle"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)subdivide_set, gensym("subdivide"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)blanknum_set, gensym("blanknum"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)rawmode_set, gensym("rawmode"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)maxstatuspoll_set, gensym("maxstatuspoll"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)startimmediately_set, gensym("startimmediately"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)singlemode_set, gensym("singlemode"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)dontblock_set, gensym("dontblock"), A_DEFFLOAT, 0);

  /* set the name of the help-patch to "help-helios"(.pd) */
  class_sethelpsymbol(helios_class, gensym("help-helios"));

}
