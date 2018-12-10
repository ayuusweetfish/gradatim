#include "couverture.h"
#include "global.h"
#include "profile_data.h"

static void couverture_tick(couverture *this, double dt)
{
}

static void couverture_draw(couverture *this)
{
    SDL_SetRenderDrawColor(g_renderer, 216, 255, 192, 255);
    SDL_RenderFillRect(g_renderer, NULL);
}

couverture *couverture_create()
{
    couverture *this = malloc(sizeof(couverture));
    memset(this, 0, sizeof(*this));
    this->_base.tick = (scene_tick_func)couverture_tick;
    this->_base.draw = (scene_draw_func)couverture_draw;

    orion_load_ogg(&g_orion, TRACKID_MAIN_BGM, "copycat.ogg");
    orion_play_loop(&g_orion, TRACKID_MAIN_BGM, 0, BGM_LOOP_A, BGM_LOOP_B);
    orion_ramp(&g_orion, TRACKID_MAIN_BGM, 0, profile.bgm_vol * VOL_VALUE);
    orion_apply_lowpass(&g_orion, TRACKID_MAIN_BGM, TRACKID_MAIN_BGM_LP, 1760);
    orion_play_loop(&g_orion, TRACKID_MAIN_BGM_LP, 0, 0, -1);
    orion_ramp(&g_orion, TRACKID_MAIN_BGM_LP, 0, 0);
    orion_overall_play(&g_orion);

    return this;
}
