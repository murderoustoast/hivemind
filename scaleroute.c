/* [scaleroute]: an external for organizing pitch data based on pitch sets.
 * see the help file
 */
#include <string.h>

static t_class *scaleroute_class;

typedef struct _scaleroute
{
  t_object fte;
  char strs[MAXPDSTRING];
  int pcs[12];
  int root;
  t_outlet *outlet[2];
} t_scaleroute;

void scaleroute_float(t_scaleroute *x, t_floatarg f)
{
  int index = ((int)f + 12 - x->root) % 12;
  if(index < 12 && index > -1)
    {
      outlet_float(x->outlet[0], f);
      outlet_float(x->outlet[1], x->pcs[index]);
    }  
  else
    error("Invalid input");
}

void scaleroute_update(t_scaleroute *x, t_symbol *s, int argc, t_atom *argv)
{
  char temp[MAXPDSTRING];
  int foo = x->root;
  t_atom *bar = &argv[0];
  atom_string(bar, temp, sizeof(temp));
  if(strcmp(temp, "C") == 0 || strcmp(temp, "c") == 0)
    {
      x->root = 0;
      post("C set as root.");
    }  
  if(strcmp(temp, "C#") == 0 || strcmp(temp, "c#") == 0)
    {
      x->root = 1;
      post("C# set as root.");
    }  
  if(strcmp(temp, "Db") == 0 || strcmp(temp, "db") == 0)
    {
      x->root = 1;
      post("Db set as root.");
    }  
  if(strcmp(temp, "D") == 0 || strcmp(temp, "d") == 0)
    {
      x->root = 2;
      post("D set as root.");
    }  
  if(strcmp(temp, "D#") == 0 || strcmp(temp, "d#") == 0)
    {
      x->root = 3;
      post("D# set as root.");
    }  
  if(strcmp(temp, "Eb") == 0 || strcmp(temp, "eb") == 0)
    {
      x->root = 3;
      post("Eb set as root.");
    }  
  if(strcmp(temp, "E") == 0 || strcmp(temp, "e") == 0)
    {
      x->root = 4;
      post("E set as root.");
    }  
  if(strcmp(temp, "E#") == 0 || strcmp(temp, "e#") == 0)
    {
      x->root = 5;
      post("E# set as root.");
    }  
  if(strcmp(temp, "Fb") == 0 || strcmp(temp, "fb") == 0)
    {
      x->root = 4;
      post("Fb set as root.");
    }  
  if(strcmp(temp, "F") == 0 || strcmp(temp, "f") == 0)
    {
      x->root = 5;
      post("F set as root.");
    }  
  if(strcmp(temp, "F#") == 0 || strcmp(temp, "f#") == 0)
    {
      x->root = 6;
      post("F# set as root.");
    }  
  if(strcmp(temp, "Gb") == 0 || strcmp(temp, "gb") == 0)
    {
      x->root = 6;
      post("Gb set as root.");
    }  
  if(strcmp(temp, "G") == 0 || strcmp(temp, "g") == 0)
    {
      x->root = 7;
      post("G set as root.");
    }  
  if(strcmp(temp, "G#") == 0 || strcmp(temp, "g#") == 0)
    {
      x->root = 8;
      post("G# set as root.");
    }  
  if(strcmp(temp, "Ab") == 0 || strcmp(temp, "ab") == 0)
    {
      x->root = 8;
      post("Ab set as root.");
    }  
  if(strcmp(temp, "A") == 0 || strcmp(temp, "a") == 0)
    {
      x->root = 9;
      post("A set as root.");
    }  
  if(strcmp(temp, "A#") == 0 || strcmp(temp, "a#") == 0)
    {
      x->root = 10;
      post("A# set as root.");
    }  
  if(strcmp(temp, "Bb") == 0 || strcmp(temp, "bb") == 0)
    {
      x->root = 10;
      post("Bb set as root.");
    }  
  if(strcmp(temp, "B") == 0 || strcmp(temp, "b") == 0)
    {
      x->root = 11;
      post("B set as root.");
    }  
  if(strcmp(temp, "B#") == 0 || strcmp(temp, "b#") == 0)
    {
      x->root = 0;
      post("B# set as root.");
    }  
  if(strcmp(temp, "Cb") == 0 || strcmp(temp, "cb") == 0)
    {
      x->root = 11;
      post("Cb set as root.");
    }  
}

void *scaleroute_new(t_atom *s, int argc, t_atom *argv)
{
  t_scaleroute *x = (t_scaleroute *)pd_new(scaleroute_class);
  x->root = 0;
  //= {0, 3, 2, 1, 1, 0, 3, 0, 1, 1, 2, 3};
  x->pcs[0] = 0;
  x->pcs[1] = 3;
  x->pcs[2] = 2;
  x->pcs[3] = 1;
  x->pcs[4] = 1;
  x->pcs[5] = 0;
  x->pcs[6] = 3;
  x->pcs[7] = 0;
  x->pcs[8] = 1;
  x->pcs[9] = 1;
  x->pcs[10] = 2;
  x->pcs[11] = 3;
  x->outlet[0] = outlet_new(&x->fte, &s_float);
  x->outlet[1] = outlet_new(&x->fte, &s_float);
  return (void *)x;
}

void scaleroute_setup(void)
{
  scaleroute_class = class_new(gensym("scaleroute"), 
			       (t_newmethod)scaleroute_new, 0,
			       sizeof(t_scaleroute), CLASS_DEFAULT, A_GIMME, 
			       0);
  class_addfloat(scaleroute_class, scaleroute_float);
  class_addmethod(scaleroute_class, scaleroute_update, gensym("root"), 
		  A_GIMME, 0);
}
