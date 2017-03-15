/*
 * hivemindgone: a library for spatializing audio based on dissonance 
 *
 * by: David Estes-Smargiassi, Johann Lau, Erik Peterson, Nicole Regenauer,
 * Brian Voyer, Andy Wiggins
 *
 * <destessm|jlau|epeters1|nregenau|bvoyer|awiggins@stevens.edu>
 */

#include "m_pd.h"

static char *version = "hivemindgone: a library for spatializing audio based on dissonance";
static char *author  = "(c) David Estes-Smargiassi, Johann Lau, Erik Peterson, Nicole Regenauer, Brian Voyer, Andy Wiggins 2017";
static char *emails  = "<destessm|jlau|epeters1|nregenau|bvoyer|awiggins@stevens.edu>";

// [scaleroute] - returns MIDI pitches and how consonant they are based on a
//                root
#include "scaleroute.c"


void hivemindgone_setup(void)
{
  scaleroute_setup();
  post(version);
  post(author);
  post(emails);
}
