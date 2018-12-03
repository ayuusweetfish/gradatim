#include "mod.h"

const struct mod_text MODS[N_MODS][N_MODSTATES] = {{
    {"crotchet.png", 0x445566, "Moderato",
     "Tempo primo."},
    {"quaver.png", 0x775544, "Vivace",
     "A show-off of dexerity."},
    {"minim.png", 0x446655, "Andante",
     "Go on an unhurried adventure."}
}, {
    {"giusto.png", 0x445566, "Giusto",
     "Feel the rhythm."},
    {"rubato.png", 0x447777, "Rubato",
     "Never restrict yourself."},
    {"a_piacere.png", 0x338888, "A piacere",
     "Forget about all those rules!"}
}, {
    {"cantabile.png", 0x445566, "Cantabile",
     "Backed by the Muses."},
    {"sotto_voco.png", 0x447755, "Sotto voco",
     "Metronome retires."},
    {"a_capella.png", 0x333355, "A capella",
     "Sightreading? Cakewalk."}
}, {
    {"aria.png", 0x445566, "Aria",
     "Narrative matters."},
    {"semplice.png", 0x444455, "Semplice",
     "Say less and simply run."},
    {"stretto.png", 0x222266, "Stretto",
     "Hearing is believing."}
}};

int modcomb_id(int mask)
{
    return ((mask & MOD_STRETTO) ? 1 : 0) * 27 +
        ((mask & 0x30) >> 4) * 9 +
        ((mask & 0xc) >> 2) * 3 +
        (mask & 0x3);
}
