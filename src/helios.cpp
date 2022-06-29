/*
 * pd_helios
 * © 2019 Tim Redfern
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

    int flip_x = x->helios->get_flip_x() ? -1.0 : 1.0;
    int flip_y = x->helios->get_flip_y() ? -1.0 : 1.0;
    float scale = x->helios->get_scale();
    float rotation = x->helios->get_rotation() / 180.0 * 3.14159265;
    float cosr = cos(rotation);
    float sinr = sin(rotation);
    float scale_x = x->helios->get_scale_x();
    float scale_y = x->helios->get_scale_y();
    float offset_x = x->helios->get_offset_x();
    float offset_y = x->helios->get_offset_y();

    // thanks to mpolak @ photonlexicon for the clarity:
    // https://photonlexicon.com/forums/showthread.php/26099-geometric-correction-algorithm/page4
    float shear_x = x->helios->get_shear_x();
    float shear_y = x->helios->get_shear_y();
    float keystone_x = x->helios->get_keystone_x();
    float keystone_y = x->helios->get_keystone_y();
    float linearity_x = x->helios->get_linearity_x();
    float linearity_y = x->helios->get_linearity_y();
    float bow_x = x->helios->get_bow_x();
    float bow_y = x->helios->get_bow_y();
    float pincushion_x = x->helios->get_pincushion_x();
    float pincushion_y = x->helios->get_pincushion_y();
    int colormapr = x->helios->get_colormapr();
    int colormapg = x->helios->get_colormapg();
    int colormapb = x->helios->get_colormapb();

    int ttlthresh = x->helios->get_ttlthreshold();

    if (colormapr != 1 || colormapg != 2|| colormapb != 4) {
        int rr = (colormapr & 1);
        int rg = (colormapg & 1);
        int rb = (colormapb & 1);
        int gr = ((colormapr & 2) >> 1);
        int gg = ((colormapg & 2) >> 1);
        int gb = ((colormapb & 2) >> 1);
        int br = ((colormapr & 4) >> 2);
        int bg = ((colormapg & 4) >> 2);
        int bb = ((colormapb & 4) >> 2);

        for (auto& p:points) {
            std::uint32_t rnew=0, gnew=0 , bnew=0;
             p.r = (std::uint8_t)(rr * p.r + rg * p.g + rb * p.b);
             p.g = (std::uint8_t)(gr * p.r + gg * p.g + gb * p.b);
             p.b = (std::uint8_t)(br * p.r + bg * p.g + bb * p.b);
        }
    }

    for (auto& p:points) {
        // basic transform
        p.x = p.x * flip_x * scale * scale_x;
        p.y = p.y * flip_y * scale * scale_y;

        float xr = p.x * cosr - p.y * sinr;
        float yr = p.y * cosr + p.x * sinr;
        p.x = xr;
        p.y = yr;

        p.x += offset_x;
        p.y += offset_y;

        // geometric correction
        float xn =  p.x / 2048.0; // map -1..1
        float yn =  p.y / 2048.0; // map -1..1
        float xt = xn;
        float yt = yn;

        xt += shear_x * yn;
        yt += shear_y * xn;

        xt += keystone_x * xn * yn;
        yt += keystone_y * xn * yn;

        xt += linearity_x * xn * xn;
        yt += linearity_y * yn * yn;

        xt += bow_x * yn * yn;
        yt += bow_y * xn * xn;

        xt += pincushion_x * xn * yn * yn;
        yt += pincushion_y * yn * xn * xn;

        p.x = xt * 2048.0;
        p.y = yt * 2048.0;

        // TTL color threshold
        if (ttlthresh > 0) {
            p.r = (p.r >= ttlthresh)? 255 : 0;
            p.g = (p.g >= ttlthresh)? 255 : 0;
            p.b = (p.b >= ttlthresh)? 255 : 0;
        }
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

void pps_set(t_helios *x, t_floatarg f)
{
  int newpps=min(0xffff, max(10,(int)f));
  x->helios->set_pps(newpps);
  //post("pps: %d", newpps);
  //redraw(x);
}

void blankoffset_set(t_helios *x, t_floatarg f)
{
  int bloffs= (int)f;
  x->helios->set_blank_offset(bloffs);
  //post("blankoffset: %d", bloffs);
  //redraw(x);
}

void intensity_set(t_helios *x, t_floatarg f)
{
  int intensity=min(255,max(0,(int)f));
  x->helios->set_intensity(intensity);
  //redraw(x);
}

void ttlthreshold_set(t_helios *x, t_floatarg f)
{
  int ttlthreshold=min(255,max(0,(int)f));
  x->helios->set_ttlthreshold(ttlthreshold);
  //post("ttlthreshold: %d", ttlthreshold);
  //redraw(x);
}

void maxangle_set(t_helios *x, t_floatarg f)
{
  float maxangle=min(90,max(0,(int)f));
  x->helios->set_maxangle(maxangle);
  //redraw(x);
}

void subdivide_set(t_helios *x, t_floatarg f)
{
  int subdivide=min(255,max(1,(int)f));
  x->helios->set_subdivide(subdivide);
  //redraw(x);
}

void blanknum_set(t_helios *x, t_floatarg f)
{
  int blanknum=min(255,max(0,(int)f));
  x->helios->set_blanknum(blanknum);
  //redraw(x);
}

void rawmode_set(t_helios *x, t_floatarg f)
{
  int rawmode=min(1,max(0,(int)f));
  x->helios->set_rawmode(rawmode);
  //post("rawmode: %d", rawmode);
  redraw(x);
}

void maxstatuspoll_set(t_helios *x, t_floatarg f)
{
  int maxpoll=max(0,(int)f);
  x->helios->set_maxstatuspoll(maxpoll);
  //post("maxstatuspoll: %d", maxpoll);
  //redraw(x);
}

void startimmediately_set(t_helios *x, t_floatarg f)
{
    int startimmediately = min(1, max(0, (int)f));
    x->helios->set_startimmediately(startimmediately);
    //post("startimmediately: %d", startimmediately);
    redraw(x);
}
void singlemode_set(t_helios *x, t_floatarg f)
{
    int singlemode = min(1, max(0, (int)f));
    x->helios->set_singlemode(singlemode);
    //post("singlemode: %d", singlemode);
    redraw(x);
}
void dontblock_set(t_helios *x, t_floatarg f)
{
    int dontblock = min(1, max(0, (int)f));
    x->helios->set_dontblock(dontblock);
    //post("dontblock: %d", dontblock);
    redraw(x);
}

void flipx_set(t_helios *x, t_floatarg f)
{
    int flipx = min(1, max(0, (int)f));
    x->helios->set_flip_x(flipx);
    //post("flipx: %d", flipx);
    redraw(x);
}

void flipy_set(t_helios *x, t_floatarg f)
{
    int flipy = min(1, max(0, (int)f));
    x->helios->set_flip_y(flipy);
    //post("flipy: %d", flipy);
    redraw(x);
}

void offsetx_set(t_helios *x, t_floatarg f)
{
    if (f > -4096.0 && f < 4096.0) {
        x->helios->set_offset_x(f);
        //post("offsetx: %f", f);
    }
}

void offsety_set(t_helios *x, t_floatarg f)
{
    if (f > -4096.0 && f < 4096.0) {
        x->helios->set_offset_y(f);
        //post("offsety: %f", f);
    }

}

void scale_set(t_helios *x, t_floatarg f)
{
    float scale = f;
    x->helios->set_scale(scale);
    //post("scale: %f", scale);
}

void rotation_set(t_helios *x, t_floatarg f)
{
    float rot = f;
    x->helios->set_rotation(rot);
    //post("rotation: %f", rot);
}

void scalex_set(t_helios *x, t_floatarg f)
{
    float sx = f;
    x->helios->set_scale_x(sx);
}
void scaley_set(t_helios *x, t_floatarg f)
{
    float sy = f;
    x->helios->set_scale_y(sy);
}


void shearx_set(t_helios *x, t_floatarg f)
{
    x->helios->set_shear_x(f);
    //post("shearx: %f", f);
}
void sheary_set(t_helios *x, t_floatarg f)
{
    x->helios->set_shear_y(f);
    //post("sheary: %f", f);
}
void keystonex_set(t_helios *x, t_floatarg f)
{
    x->helios->set_keystone_x(f);
    //post("keystonex: %f", f);
}
void keystoney_set(t_helios *x, t_floatarg f)
{
    x->helios->set_keystone_y(f);
    //post("keystoney: %f", f);
}
void linearityx_set(t_helios *x, t_floatarg f)
{
    x->helios->set_linearity_x(f);
    //post("linearityx: %f", f);
}
void linearityy_set(t_helios *x, t_floatarg f)
{
    x->helios->set_linearity_y(f);
    //post("linearityy: %f", f);
}
void bowx_set(t_helios *x, t_floatarg f)
{
    x->helios->set_bow_x(f);
    //post("bowx: %f", f);
}
void bowy_set(t_helios *x, t_floatarg f)
{
    x->helios->set_bow_y(f);
    //post("bowy: %f", f);
}
void pincushionx_set(t_helios *x, t_floatarg f)
{
    x->helios->set_pincushion_x(f);
    //post("pincushionx: %f", f);
}
void pincushiony_set(t_helios *x, t_floatarg f)
{
    x->helios->set_pincushion_y(f);
    //post("pincushiony: %f", f);
}
void colormapr_set(t_helios *x, t_floatarg f)
{
    x->helios->set_colormapr((int)f);
}
void colormapg_set(t_helios *x, t_floatarg f)
{
    x->helios->set_colormapg((int)f);
}
void colormapb_set(t_helios *x, t_floatarg f)
{
    x->helios->set_colormapb((int)f);
}


void dumpframe_set(t_helios *x, t_floatarg f)
{
    x->helios->request_framedump();
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
        (t_method)blankoffset_set, gensym("blankoffset"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)intensity_set, gensym("intensity"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)ttlthreshold_set, gensym("ttlthreshold"), A_DEFFLOAT, 0);

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
  
  class_addmethod(helios_class,
        (t_method)flipx_set, gensym("flipx"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)flipy_set, gensym("flipy"), A_DEFFLOAT, 0);
  
  class_addmethod(helios_class,
        (t_method)offsetx_set, gensym("offsetx"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)offsety_set, gensym("offsety"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)scale_set, gensym("scale"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)rotation_set, gensym("rotation"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)scalex_set, gensym("scalex"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)scaley_set, gensym("scaley"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)shearx_set, gensym("shearx"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)sheary_set, gensym("sheary"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)keystonex_set, gensym("keystonex"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)keystoney_set, gensym("keystoney"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)linearityx_set, gensym("linearityx"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)linearityy_set, gensym("linearityy"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)bowx_set, gensym("bowx"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)bowy_set, gensym("bowy"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)pincushionx_set, gensym("pincushionx"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)pincushiony_set, gensym("pincushiony"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)colormapr_set, gensym("colormapr"), A_DEFFLOAT, 0);
  class_addmethod(helios_class,
        (t_method)colormapg_set, gensym("colormapg"), A_DEFFLOAT, 0);
  class_addmethod(helios_class,
        (t_method)colormapb_set, gensym("colormapb"), A_DEFFLOAT, 0);

  class_addmethod(helios_class,
        (t_method)dumpframe_set, gensym("dumpframe"), A_DEFFLOAT, 0);

  /* set the name of the help-patch to "help-helios"(.pd) */
  class_sethelpsymbol(helios_class, gensym("help-helios"));

}
