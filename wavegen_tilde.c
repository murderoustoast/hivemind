#include <m_pd.h>
#include <math.h>
#include <string.h>
#define TWOPI 6.2831853072
#define MAX_AMPLITUDES 64
static t_class *wavegen_tilde_class;

char *version = "wavegen~ v1.0 by David Estes-Smargiassi";

typedef struct _wavegen_tilde
{
  t_object obj;
  t_sample x_f;

  long table_length; // length of wavetable
  char wave[20];
  t_symbol *waveform;
  float *wavetable; // wavetable
  float *old_wavetable; // used for crossfading
  float sr, phase, si;
  long wavetable_bytes;

  float si_factor; // factor for generating the sampling increment
  
  float *amplitudes; // list of amplitudes for harmonics
  int harmonic_count;

  // crossfade vars
  short dirty;
  int xfade_countdown;
  float xfade_duration;
  int xfade_samples;

  // vpdelay vars
  int delayOn; // flag for delay
  float maximum_delay_time; // maximum delay time
  long delay_length; // length of the delay line in samples
  long delay_bytes; // length of delay line in bytes
  float *delay_line; // the delay line itself
  float *read_ptr; // read pointer into delay line
  float *write_ptr; // write pointer into delay line
  float delay_time; // current delay time
  float feedback; // feedback multiplier
  short delaytime_connected; // inlet connection status
  short feedback_connected; // inlet connection status
  float fdelay; // the fractional delay time
  long idelay; // the integer delay time
  float fraction; // the fractional difference between the fractional and
		  // integer delay times
  float srms; //sampling rate as milliseconds
} t_wavegen_tilde;

void tabswitch(t_wavegen_tilde *x);
void sinebasic(t_wavegen_tilde *x);
void sinebasicAdd(t_wavegen_tilde *x, float amp, int harmonic);
void sawtoothbasic(t_wavegen_tilde *x);
void sawtoothbasicAdd(t_wavegen_tilde *x, float amplitude, int harmonic);
void squarebasic(t_wavegen_tilde *x);
void squarebasicAdd(t_wavegen_tilde *x, float amplitude, int harmonic);
void trianglebasic(t_wavegen_tilde *x);
void trianglebasicAdd(t_wavegen_tilde *x, float amplitude, int harmonic);
void *wavegen_tilde_new(t_symbol *s, int argc, t_atom *argv);
void wavegen_tilde_build_waveform(t_wavegen_tilde *x);
void wavegen_tilde_list(t_wavegen_tilde *x, t_symbol *msg, short argc, t_atom *argv);
void wavegen_tilde_list_sin(t_wavegen_tilde *x, t_symbol *msg, short argc, t_atom *argv);
void wavegen_tilde_list_saw(t_wavegen_tilde *x, t_symbol *msg, short argc, t_atom *argv);
void wavegen_tilde_list_sqr(t_wavegen_tilde *x, t_symbol *msg, short argc, t_atom *argv);
void wavegen_tilde_list_tri(t_wavegen_tilde *x, t_symbol *msg, short argc, t_atom *argv); 
//void wavegen_tilde_changedelay(t_wavegen_tilde *x, t_symbol *msg, short argc, t_atom *argv);
t_int *wavegen_tilde_perform(t_int *w);
void wavegen_tilde_dsp(t_wavegen_tilde *x, t_signal **sp);
void wavegen_tilde_free(t_wavegen_tilde *x);
void wavegen_tilde_setup(void);

void sinebasic(t_wavegen_tilde *x) {
  tabswitch(x);
  for(int i = 0; i < x->table_length; i++) {
    x->wavetable[i] = sin(TWOPI * (float)i / (float)x->table_length);
  }
  x->dirty = 0;
  x->xfade_countdown = x->xfade_samples;
}

void sinebasicAdd(t_wavegen_tilde *x, float amp, int harmonic) {
  for(int i = 0; i < x->table_length; i++) {
    x->wavetable[i] += amp*(sin(harmonic* TWOPI * (float)i / (float)x->table_length));
  }
}

void sawtoothbasic(t_wavegen_tilde *x) {
  tabswitch(x);
  for(int i = 0; i < x->table_length; i++) {
    float saw = 0;
    saw = (float)i / (float)x->table_length;
    //post("%f", saw);
    x->wavetable[i] = saw;
  }
  x->dirty = 0;
  x->xfade_countdown = x->xfade_samples;
}

void sawtoothbasicAdd(t_wavegen_tilde *x, float amplitude, int harmonic) {
  for(int i = 0; i < x->table_length; i++) {
    float saw = 0;
    saw = harmonic*(fmod((double)i, x->table_length/harmonic)) / (float)x->table_length;
    //post("%f", saw);
    x->wavetable[i] += amplitude*saw-0.25;
  }
}

void squarebasic(t_wavegen_tilde *x) {
  tabswitch(x);
    for(int i = 0; i < x->table_length; i++) {
      float sqr = 0;
      if(i > x->table_length/2){
	sqr = 1.0;
      } else {
	sqr = 0.0;
      }
      //post("%f", sqr);
      x->wavetable[i] = sqr;
    }
    x->dirty = 0;
    x->xfade_countdown = x->xfade_samples;
}

void squarebasicAdd(t_wavegen_tilde *x, float amplitude, int harmonic) {
  float pl = (float)x->table_length/pow(2,harmonic);
  for(int i = 0; i < x->table_length; i++) {
    float sqr = -1.0;
    if( fmod(i,(2*pl)) > pl){
      sqr = 1.0;
    } 
    //post("%f", sqr);
    x->wavetable[i] += amplitude*sqr;
  }
}

void trianglebasic(t_wavegen_tilde *x) {
  tabswitch(x);
  for(int i = 0; i < x->table_length; i++) {
    float tri = 0;
    if(i < x->table_length/2){
      tri = (float)i / ((float)x->table_length);
    } else {
      tri = (1 - ((float)i / ((float)x->table_length)));
    }
    //post("%f", tri);
    x->wavetable[i] = tri;
  }
  
  /*implement normalization*/
  float max = 0.0, rescale;
  
  for(int j = 0; j < x->table_length; j++) {
    if(max < fabs(x->wavetable[j])) {
      max = fabs(x->wavetable[j]);
      // post("%f", max);
    }
  }
  
  if(max==0.0) { //avoid divide by zero
    return;
  }
  
  rescale = 1.0 / max ;
  
  for(int k = 0; k < x->table_length; k++){
    x->wavetable[k] *= rescale ;
  } 
  x->dirty = 0;
  x->xfade_countdown = x->xfade_samples;
}

void trianglebasicAdd(t_wavegen_tilde *x, float amplitude, int harmonic) {
  float pl = (float)x->table_length/pow(2,harmonic);
  for(int i = 0; i < x->table_length; i++) {
    float tri = 0;
    float j = fmod(i, 2*pl);
    if( j < pl){
      tri = (2*j)/pl - 1; 
    } else {
      tri = 1 - 2*(j-pl)/pl;
    }
    x->wavetable[i] += amplitude*tri;//-0.25;
  }
}

void *wavegen_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
  float init_freq;  
  init_freq = 440;
  t_wavegen_tilde *x = (t_wavegen_tilde *) pd_new(wavegen_tilde_class);
  
  inlet_new(&x->obj, &x->obj.ob_pd, gensym("signal"), gensym("signal"));
  inlet_new(&x->obj, &x->obj.ob_pd, gensym("signal"), gensym("signal"));
  outlet_new(&x->obj, gensym("signal"));

  x->table_length = 1048576; //8192; //try 1048576
  x->wavetable_bytes = x->table_length * sizeof(float);
  x->wavetable = (float *) getbytes(x->wavetable_bytes);
  x->old_wavetable = (float *)getbytes(x->wavetable_bytes);
  
  x->phase = 0;
  x->sr = sys_getsr(); 
  x->si_factor = (float) x->table_length / x->sr;
  x->si = init_freq * x->si_factor ;

  x->waveform = atom_getsymbolarg(0, argc, argv);

  x->amplitudes = (float *)getbytes(sizeof(float)*MAX_AMPLITUDES);

  x->dirty = 0;
  x->xfade_countdown = 0;
  x->xfade_duration = 100;
  x->xfade_samples = x->xfade_duration * x->sr / 1000.0;

  /* Branch to the appropriate method to initialize the waveform */
  if (x->waveform == gensym("sine")) {
    sinebasic(x);
  } else if (x->waveform == gensym("triangle")) {
    trianglebasic(x);
  } else if (x->waveform == gensym("square")) {
    squarebasic(x);
  } else if (x->waveform == gensym("sawtooth")) {
    sawtoothbasic(x);
  } else {
    error("%s is not a legal waveform - using sine wave instead", x->waveform->s_name);
    sinebasic(x);
  }
  
  // delay parameters
  x->delayOn = 0;
  float delmax = 250.0;
  float deltime = 100.0;
  float feedback = 0.1;

  if(argc > 1)
    {
      delmax = atom_getfloatarg(1, argc, argv);
    }
  if(argc > 2)
    {
      deltime = atom_getfloatarg(2, argc, argv);
    }
  if(argc > 3)
    {
      feedback = atom_getfloatarg(3, argc, argv);
    }
  
  /* Set delay maximum */
  if(delmax <= 0)
    {
      delmax = 250.0;
    }
  x->maximum_delay_time = delmax * 0.001;
  
  /* Set delay time */
  if(deltime > delmax || deltime <= 0.0)
    {
      error("illegal delay time: %f, reset to 1ms", deltime);
      deltime = 1.0;
    }
  x->delay_time = deltime;
  
  /* set feedback */
  x->feedback = feedback;
  
  /* force memory realloc in dsp method */
  x->sr = 0.0;
  return (void *)x;
}



// Adapted from Eric Lyon, Designing Audio Objects for MAX/MSP and PD
void tabswitch(t_wavegen_tilde *x)
{
  float *wavetable = x->wavetable;
  float *old_wavetable = x->old_wavetable;
  memcpy(old_wavetable, wavetable, x->table_length * sizeof(float));
  x->dirty = 1; 
}

void wavegen_tilde_build_waveform(t_wavegen_tilde *x)
{
  float rescale;
  int i, j;
  float max;
  float *wavetable = x->wavetable;
  float *amplitudes = x->amplitudes;
  int partial_count = x->harmonic_count + 1;
  int table_length = x->table_length;

  if(partial_count < 1)
    {
      error("No harmonics specified, waveform not created!");
      return;
    }
  max = 0.0;
  for(i = 0; i<partial_count; i++)
    {
      max += fabs(amplitudes[i]);
    }
  if(!max)
    {
      error("All zero function, waveform not created!");
      return;
    }
  for( i = 0; i < table_length; i++)
    {
      wavetable[i] = amplitudes[0];
    }
  for( i = 1; i < partial_count; i++)
    {
      if(amplitudes[i])
	{
	  if (x->waveform == gensym("sin")) 
	    {
	      sinebasicAdd(x,amplitudes[i],i);
	    } 
	  else if (x->waveform == gensym("tri")) 
	    {
	      trianglebasicAdd(x,amplitudes[i],i);
	    } 
	  else if (x->waveform == gensym("sqr")) 
	    {
	      squarebasicAdd(x,amplitudes[i],i);
	    } 
	  else if (x->waveform == gensym("saw")) 
	    {
	      sawtoothbasicAdd(x,amplitudes[i],i);
	    } 
	  else 
	    {
	      error("%s is not a legal waveform - using sine wave instead", x->waveform->s_name);
	      sinebasicAdd(x,amplitudes[i],i);
	    }
	}
    }
  // rescale:
  max = 0.0;
  for(i=0; i<table_length/2; i++)
    {
      if(max < fabs(wavetable[i]))
	{
	  max = fabs(wavetable[i]);
	}
    }
  if(max == 0)
    {
      error("Weird all zero error! - EXCITING!");
      return;
    }
  rescale = 1.0 / max;
  for(i=0; i < table_length; i++)
    {
      wavetable[i] *= rescale;
    }
  x->dirty = 0;
  x->xfade_countdown = x->xfade_samples;
}



void wavegen_tilde_list (t_wavegen_tilde *x, t_symbol *msg, short argc, t_atom *argv)
{
  tabswitch(x);
  short i;
  int harmonic_count = 0;
  float * amplitudes = x->amplitudes;
  for (i = 0; i < MAX_AMPLITUDES; i++)
    {
      amplitudes[i] = 0;
    }
  for (i = 0; i < argc; i++) 
    {
      amplitudes[harmonic_count++] = atom_getfloat(argv+i);
    }
  x->harmonic_count = harmonic_count;
  x->waveform = gensym("sin");
  wavegen_tilde_build_waveform(x);
}

void wavegen_tilde_list_sin (t_wavegen_tilde *x, t_symbol *msg, short argc, t_atom *argv)
{
  tabswitch(x);
  short i;
  int harmonic_count = 0;
  float * amplitudes = x->amplitudes;
  for (i = 0; i< MAX_AMPLITUDES; i++)
    {
      amplitudes[i] = 0;
    }
  for (i = 0; i < argc; i++)
    {
      amplitudes[harmonic_count++] = atom_getfloat(argv+i);
    }
  x->harmonic_count = harmonic_count;
  x->waveform = gensym("sin");
  wavegen_tilde_build_waveform(x);
}

void wavegen_tilde_list_saw (t_wavegen_tilde *x, t_symbol *msg, short argc, t_atom *argv)
{
  tabswitch(x);
  short i;
  int harmonic_count = 0;
  float * amplitudes = x->amplitudes;
  for (i = 0; i< MAX_AMPLITUDES; i++)
    {
      amplitudes[i] = 0;
    }
  for (i = 0; i < argc; i++)
    {
      amplitudes[harmonic_count++] = atom_getfloat(argv+i);
    }
  x->harmonic_count = harmonic_count;
  x->waveform = gensym("saw");
  wavegen_tilde_build_waveform(x);
}

void wavegen_tilde_list_sqr (t_wavegen_tilde *x, t_symbol *msg, short argc, t_atom *argv)
{
  tabswitch(x);
  short i;
  int harmonic_count = 0;
  float * amplitudes = x->amplitudes;
  for (i = 0; i< MAX_AMPLITUDES; i++)
    {
      amplitudes[i] = 0;
    }
  for (i = 0; i < argc; i++)
    {
      amplitudes[harmonic_count++] = atom_getfloat(argv+i);
    }
  x->harmonic_count = harmonic_count;
  x->waveform = gensym("sqr");
  wavegen_tilde_build_waveform(x);
}

void wavegen_tilde_list_tri (t_wavegen_tilde *x, t_symbol *msg, short argc, t_atom *argv)
{
  tabswitch(x);
  short i;
  int harmonic_count = 0;
  float * amplitudes = x->amplitudes;
  for (i = 0; i< MAX_AMPLITUDES; i++)
    {
      amplitudes[i] = 0;
    }
  for (i = 0; i < argc; i++)
    {
      amplitudes[harmonic_count++] = atom_getfloat(argv+i);
    }
  x->harmonic_count = harmonic_count;
  x->waveform = gensym("tri");
  wavegen_tilde_build_waveform(x);
}


t_int *wavegen_tilde_perform(t_int *w)
{
  t_wavegen_tilde *x = (t_wavegen_tilde *) (w[1]);
  float *midi = (t_float *)(w[2]);
  float *delay_time = (t_float *)(w[3]);
  float *feedback = (t_float *)(w[4]);
  float *out = (t_float *)(w[5]);
  int n = w[6];

  long iphase;
  float si_factor = x->si_factor;
  float si = x->si;
  float phase = x->phase;
  long table_length = x->table_length;
  float *wavetable = x->wavetable;
  float *old_wavetable = x->old_wavetable;

  float sr = x->sr;
  float *delay_line = x->delay_line;
  float  *read_ptr  = x->read_ptr;
  float  *write_ptr = x->write_ptr;
  long delay_length = x->delay_length;
  float *endmem = delay_line + delay_length;
  short delaytime_connected = x->delaytime_connected;
  short feedback_connected = x->feedback_connected;
  float delaytime_float = x->delay_time;
  float feedback_float = x->feedback;	
  float fraction;
  float fdelay;
  float samp1, samp2;
  long idelay;
  float srms = sr / 1000.0;
  float out_sample, feedback_sample;
  float new_sample;
  

  while (n--) 
    {
      t_sample f = *midi++;
      t_sample freq;
      if( f <= -1500 )
	{
	  freq = 0;
	}
      else 
	{
	  if( f > 1499 )
	    {
	      f = 1499;
	    }
	  freq = 8.17579891564 * exp( 0.0577622650*f );
	}
      si = freq * si_factor;
      iphase = floor(phase); //truncate phase
      
      fdelay = *delay_time++ * srms;
      while(fdelay > delay_length)
	{
	  fdelay -= delay_length;
	}
      while(fdelay < 0)
	{
	  fdelay += delay_length;
	}
      idelay = (int)fdelay;
      fraction = fdelay - idelay;
      read_ptr = write_ptr - idelay;
      while(read_ptr < delay_line)
	{
	  read_ptr += delay_length;
	}
      samp1 = *read_ptr++;
      if(read_ptr == endmem)
	{
	  read_ptr = delay_line;
	}
      samp2 = *read_ptr;
      out_sample = samp1 + fraction * (samp2-samp1);
      feedback_sample = out_sample * *feedback++;
      if(fabs(feedback_sample) < 0.000001)
	{
	  feedback_sample = 0.0;
	}      
      
      if(x->xfade_countdown)
	{
	  float frac = 0.25*TWOPI*(float)x->xfade_countdown / (float)x->xfade_samples;
	  new_sample = sin(frac) * old_wavetable[iphase] + cos(frac) * wavetable[iphase];
	  --x->xfade_countdown;
	}
      else if(x->dirty)
	{
	  new_sample = old_wavetable[iphase];
	}
      else
	{
	  new_sample = wavetable[iphase];
	}
      
      *write_ptr++ = new_sample + feedback_sample;
      *out++ = out_sample;
      phase += si; 
      
      if(write_ptr == endmem)
	{
	  write_ptr = delay_line;
	}

      while(phase >= table_length)
	{
	  phase -= table_length;
	}
      while(phase < 0)
	{
	  phase += table_length;
	}
      x->phase = phase;
    }
  x->write_ptr = write_ptr;
  return w + 7;
}


void wavegen_tilde_dsp(t_wavegen_tilde *x, t_signal **sp)
{
  /*
  x->si *= x->sr / sp[0]->s_sr;
  x->sr = sp[0]->s_sr;
  x->si_factor = (float) x->table_length / x->sr;
  */

  int i;
  int oldbytesize = x->delay_bytes;

  /* Exit if the sampling rate is zero */	
  if(!sp[0]->s_sr){
    return;
  }

  x->delaytime_connected = 1;
  x->feedback_connected = 1;
  
  /* Reset the delayline if the sampling rate has changed */  
  if(x->sr != sp[0]->s_sr){
    x->sr = sp[0]->s_sr; 
    x->delay_length = x->sr * x->maximum_delay_time + 1;
    x->delay_bytes = x->delay_length * sizeof(float);
    
    /* 
       If the delayline pointer is NULL, allocate a new memory block. Note the
       use of sysmem_newptrclear() so that the memory is returned with all of
       its values set to zero. 
    */
    if(x->delay_line == NULL){
      
      /* Allocate memory */
      x->delay_line = (float *) getbytes(x->delay_bytes);
    }
    
    /* 
       Otherwise, resize the existing memory block. Note the use of sysmem_resizeptrclear()
       to set all of the returned memory locations to zero. 
    */
    else {
      
      /* Or resize memory */
      x->delay_line = (float *) resizebytes((void *)x->delay_line, 
					    oldbytesize, x->delay_bytes);
    }
    if(x->delay_line == NULL){
      error("wavegen~: cannot realloc %d bytes of memory", x->delay_bytes);
      return;
    }
    
    /* Clear the delay line */
    for(i = 0; i < x->delay_length; i++){
      x->delay_line[i] = 0.0;
    }
    
    /* Assign the write pointer to the start of the delayline */
    x->write_ptr = x->delay_line; 
  }

  /* add routine to dsp tree */
  dsp_add(wavegen_tilde_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
}


void wavegen_tilde_free(t_wavegen_tilde *x)
{
  t_freebytes(x->wavetable, x->wavetable_bytes);
  t_freebytes(x->old_wavetable, x->wavetable_bytes);
  t_freebytes(x->delay_line, x->delay_bytes);
}


void wavegen_tilde_setup(void)
{
  wavegen_tilde_class = class_new(gensym("wavegen~"), (t_newmethod)wavegen_tilde_new, (t_method)wavegen_tilde_free, 
			       sizeof(t_wavegen_tilde), CLASS_DEFAULT, A_GIMME, 0);
  class_addmethod(wavegen_tilde_class, (t_method)wavegen_tilde_dsp, gensym("dsp"),0);
  CLASS_MAINSIGNALIN(wavegen_tilde_class, t_wavegen_tilde, x_f);
  
  class_addmethod(wavegen_tilde_class, (t_method)sinebasic, gensym("sine"),0);
  class_addmethod(wavegen_tilde_class, (t_method)trianglebasic, gensym("triangle"),0);
  class_addmethod(wavegen_tilde_class, (t_method)sawtoothbasic, gensym("sawtooth"),0);
  class_addmethod(wavegen_tilde_class, (t_method)squarebasic, gensym("square"),0);
  
  class_addmethod(wavegen_tilde_class, (t_method)wavegen_tilde_list, gensym("list"), A_GIMME, 0);
  class_addmethod(wavegen_tilde_class, (t_method)wavegen_tilde_list_sin, gensym("sin"), A_GIMME, 0);
  class_addmethod(wavegen_tilde_class, (t_method)wavegen_tilde_list_saw, gensym("saw"), A_GIMME, 0);
  class_addmethod(wavegen_tilde_class, (t_method)wavegen_tilde_list_sqr, gensym("sqr"), A_GIMME, 0);
  class_addmethod(wavegen_tilde_class, (t_method)wavegen_tilde_list_tri, gensym("tri"), A_GIMME, 0);

  post(version);
}
