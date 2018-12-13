#include "credits.h"
#include "global.h"
#include "profile_data.h"
#include "orion/orion.h"

static const double BGM_VOL = 0.6;
static const double BGM_LP_VOL = 0.1;

static void credits_tick(credits_scene *this, double dt)
{
    floue_tick(this->f, dt);
}

static void credits_draw(credits_scene *this)
{
    floue_draw(this->f);
}

static void credits_drop(credits_scene *this)
{
    floue_drop(this->f);
    orion_ramp(&g_orion, TRACKID_MAIN_BGM, 0.2, profile.bgm_vol * VOL_VALUE);
    orion_ramp(&g_orion, TRACKID_MAIN_BGM_LP, 0.2, 0);
}

static void credits_key(credits_scene *this, SDL_KeyboardEvent *ev)
{
}

credits_scene *credits_create(scene *bg)
{
    credits_scene *this = malloc(sizeof(credits_scene));
    this->_base.children = bekter_create();
    this->_base.tick = (scene_tick_func)credits_tick;
    this->_base.draw = (scene_draw_func)credits_draw;
    this->_base.drop = (scene_drop_func)credits_drop;
    this->_base.key_handler = (scene_key_func)credits_key;
    this->bg = bg;

    this->f = floue_create((SDL_Color){0xd3, 0xbf, 0xa9, 255});
    int i;
    for (i = 0; i < 16; ++i)
        floue_add(this->f, (SDL_Point){rand() % WIN_W, rand() % WIN_H},
            i % 2 == 0 ?
                (SDL_Color){0xfd, 0xf7, 0xed, 0xff} :
                (SDL_Color){0xaa, 0x8d, 0x7a, 0xff},
            rand() % (WIN_W / 8) + WIN_W / 8,
            (double)rand() / RAND_MAX * 0.25 + 0.1);

    orion_ramp(&g_orion, TRACKID_MAIN_BGM, 0.2, BGM_LP_VOL * profile.bgm_vol * VOL_VALUE * 2 / 3);
    orion_ramp(&g_orion, TRACKID_MAIN_BGM_LP, 0.2, BGM_VOL * profile.bgm_vol * VOL_VALUE);
    orion_ramp(&g_orion, TRACKID_MAIN_BGM_CANON, 0.2, BGM_LP_VOL * profile.bgm_vol * VOL_VALUE / 3);

    return this;
}
