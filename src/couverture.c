#include "couverture.h"
#include "global.h"
#include "profile_data.h"
#include "orion/orion.h"

#include <stdlib.h>

static const double TINT_DUR = 1. / 16; /* In beats */

static void couverture_tick(couverture *this, double dt)
{
    floue_tick(this->f, dt);
}

static inline void shift_colours(couverture *this)
{
    this->r1 = this->r2;
    this->g1 = this->g2;
    this->b1 = this->b2;
    this->r2 = 255 - rand() % 32;
    this->g2 = 255 - rand() % 32;
    this->b2 = 255 - rand() % 32;
    switch (rand() % 3) {
        case 0: this->r2 = 255; break;
        case 1: this->g2 = 255; break;
        case 2: this->b2 = 255; break;
    }
    this->finished = false;
}

static void couverture_draw(couverture *this)
{
    long p = orion_overall_tell(&g_orion);
    if (p >= BGM_LOOP_A) {
        double frac = fmod(
            (p - BGM_LOOP_A) / 44100.0, BGM_BEAT * 4) / (BGM_BEAT * 4);
        if (frac >= 1 - TINT_DUR) {
            if (this->finished) {
                shift_colours(this);
            } else {
                double r = (frac - (1 - TINT_DUR)) / TINT_DUR;
                this->f->c0 = (SDL_Color){
                    round(this->r1 + (this->r2 - this->r1) * r),
                    round(this->g1 + (this->g2 - this->g1) * r),
                    round(this->b1 + (this->b2 - this->b1) * r),
                    255
                };
            }
        } else {
            this->f->c0 = (SDL_Color){this->r2, this->g2, this->b2, 255};
            this->finished = true;
        }
    }

    floue_draw(this->f);
}

static void couverture_drop(couverture *this)
{
    floue_drop(this->f);
}

couverture *couverture_create()
{
    couverture *this = malloc(sizeof(couverture));
    memset(this, 0, sizeof(*this));
    this->_base.tick = (scene_tick_func)couverture_tick;
    this->_base.draw = (scene_draw_func)couverture_draw;
    this->_base.drop = (scene_drop_func)couverture_drop;

    this->f = floue_create((SDL_Color){255, 255, 255, 255});
    this->r2 = this->g2 = this->b2 = 255;
    shift_colours(this);

    orion_load_ogg(&g_orion, TRACKID_MAIN_BGM, "copycat.ogg");
    orion_play_loop(&g_orion, TRACKID_MAIN_BGM, 0, BGM_LOOP_A, BGM_LOOP_B);
    orion_ramp(&g_orion, TRACKID_MAIN_BGM, 0, profile.bgm_vol * VOL_VALUE);
    orion_apply_lowpass(&g_orion, TRACKID_MAIN_BGM, TRACKID_MAIN_BGM_LP, 1760);
    orion_play_loop(&g_orion, TRACKID_MAIN_BGM_LP, 0, BGM_LOOP_A, BGM_LOOP_B);
    orion_ramp(&g_orion, TRACKID_MAIN_BGM_LP, 0, 0);
    orion_overall_play(&g_orion);

    return this;
}
