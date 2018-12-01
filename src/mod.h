/* List of modifiers */

#ifndef _MOD_H
#define _MOD_H

#define N_MODS      4
#define N_MODSTATES 3

struct mod_text {
    const char *icon;
    unsigned int colour;
    const char *title, *desc;
};

extern const struct mod_text MODS[N_MODS][N_MODSTATES];

#endif
