//
//  pannum~.c
//  pannum~
//
//  Created by Class2017 on 3/15/17.
//  Copyright (c) 2017 Class2017. All rights reserved.
//


/* PREPROCESSOR */

#include "stdio.h"
#include "m_pd.h"
#include "math.h"


/* POINTER TO CLASS */

static t_class *pannum_tilde_class;


/* STRUCTURE */

typedef struct _pannum_tilde
{
    t_object x_obj;
    t_sample pannum;
    t_sample f;
    
    t_inlet *x_in2;
    t_outlet *x_out1;
    t_outlet *x_out2;
    
} t_pannum_tilde;


/* PERFORM ROUTINE */

t_int *pannum_tilde_perform(t_int *w)
{
    t_pannum_tilde *x = (t_pannum_tilde *)(w[1]);
    t_sample *in1 = (t_sample *)(w[2]);
    t_sample *in2 = (t_sample *)(w[3]);
    t_sample *out1 = (t_sample *)(w[4]);
    t_sample *out2 = (t_sample *)(w[5]);
    
    int n = (int)(w[6]);
    
    t_sample f_pan = (x->pannum<1)?1.0:(x->pannum>11)?11.0:x->pannum;
    t_sample in, pannum;
    in = *in1++;
    pannum = *in2;

// Here I have utilized the current values output from the [scaleroute] Brian has put up on git.
// As it stands, 0 should give a full amplitude centered stereo location. A value of 1 should
// return a slightly left-panned but still "full amplitude" signal. A value of 2 should give the
// same except on the right side, and a 3 should give a centered but 1/4 amplitude signal.
    
    while (n--)
    {
        if(pannum == 0)
        {
            (*out1++) = in;
            (*out2++) = in;
        }
        if(pannum == 1)
        {
            (*out1++) = in;
            (*out2++) = in * .75;
        }
        if(pannum == 2)
        {
            (*out1++) = in * .75;
            (*out2++) = in;
        }
        if(pannum == 3)
        {
            (*out1++) = in * .25;
            (*out2++) = in * .25;
        }
    }
    
    return (w+6);
}


/* DSP ROUTINE */

void pannum_tilde_dsp (t_pannum_tilde *x, t_signal **sp)
{
    dsp_add(pannum_tilde_perform,
            6,
            x,
            sp[0]->s_vec,
            sp[1]->s_vec,
            sp[2]->s_vec,
            sp[0]->s_n);
}


/* DESTRUCTOR */

void pannum_tilde_free(t_pannum_tilde *x)
{
    inlet_free(x->x_in2);
    outlet_free(x->x_out1);
    outlet_free(x->x_out2);
}


/* CONSTRUCTOR */

void *pannum_tilde_new(t_floatarg f)
{
    t_pannum_tilde *x = (t_pannum_tilde *)pd_new(pannum_tilde_class);
    x->pannum = f;
    x->x_in2 = inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("symbol"), gensym("symbol"));
    x->x_out1 = outlet_new(&x->x_obj, &s_signal);
    x->x_out2 = outlet_new(&x->x_obj, &s_signal);
    
    return (void *)x;
}


/* SETUP */

void pannum_tilde_setup(void)
{
    pannum_tilde_class = class_new(gensym("pannum~"),
                                   (t_newmethod)pannum_tilde_new,
                                   0,
                                   sizeof(t_pannum_tilde),
                                   CLASS_DEFAULT,
                                   A_DEFFLOAT,
                                   0);
    class_addmethod(pannum_tilde_class,
                    (t_method)pannum_tilde_dsp,
                    gensym("dsp"),
                    0);
    CLASS_MAINSIGNALIN(pannum_tilde_class, t_pannum_tilde, f);
    class_sethelpsymbol(pannum_tilde_class, gensym("pannum~"));
}
