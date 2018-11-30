#include "mod.h"

const struct mod_text MODS[N_MODS][N_MODSTATES] = {{
    {"crotchet.png", 0x445566, "Moderato",
     "A tempo."},
    {"quaver.png", 0x775544, "Vivace",
     "A show-off of dexerity."},
    {"minim.png", 0x446655, "Andante",
     "Go on an unhurried adventure."}
}, {
    {"crotchet.png", 0x445566, "Giusto",
     "Feel the rhythm."},
    {"quaver.png", 0x447777, "Rubato",
     "Never restrict yourself."},
    {"minim.png", 0x338888, "A piacere",
     "Forget about all those rules!"}
}, {
    {"crotchet.png", 0x445566, "Aria",
     "Backed by the Muses."},
    {"quaver.png", 0x333355, "A capella",
     "Sightreading? Cakewalk."},
    {"minim.png", 0x664466, "Stretto",
     "Hearing is believing."}
}};
