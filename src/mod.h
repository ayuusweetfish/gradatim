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

#define MOD_VIVACE      0x1
#define MOD_ANDANTE     0x2
#define MOD_RUBATO      0x4
#define MOD_A_PIACERE   0x8
#define MOD_SOTTO_VOCO  0x10
#define MOD_A_CAPELLA   0x20
#define MOD_SEMPLICE    0x40
#define MOD_STRETTO     0x80

#endif
