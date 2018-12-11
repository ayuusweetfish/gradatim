#include "intro.h"
#include "loading.h"
#include "couverture.h"
#include "global.h"

#include <math.h>
#include <string.h>

static const double INTRO_FADEIN_DUR = 0.2;
static const double INTRO_ANIM_DUR = 1.2;
static const double INTRO_DUR = 2;
static const SDL_Color C = {128, 96, 64};
static const int QUAVER_Y = WIN_H * 9 / 20;
static const int TEXT_Y = WIN_H * 6 / 10;

static scene *goto_couverture(intro_scene *this)
{
    return (scene *)couverture_create();
}

static void goto_couverture_cleanup(intro_scene *this, couverture *that)
{
    couverture_generate_dots(that);
    SDL_SetTextureColorMod(this->quaver->tex.sdl_tex, 255, 255, 255);
    SDL_SetTextureAlphaMod(this->quaver->tex.sdl_tex, 255);
    element_drop((element *)this->quaver);
    element_drop((element *)this->text);
    scene_drop(this);
}

static void intro_tick(intro_scene *this, double dt)
{
    if ((this->time += dt) >= INTRO_DUR && g_stage == (scene *)this) {
        g_stage = (scene *)loading_create(
            &g_stage, (loading_routine)goto_couverture,
            (loading_postroutine)goto_couverture_cleanup, this);
    }
}

static void intro_draw(intro_scene *this)
{
    double r = (this->time >= INTRO_FADEIN_DUR ?
        1 : this->time / INTRO_FADEIN_DUR);
    double s = (this->time >= INTRO_ANIM_DUR ? 1 :
        this->time < INTRO_FADEIN_DUR ? 0 :
        (this->time - INTRO_FADEIN_DUR) / (INTRO_ANIM_DUR - INTRO_FADEIN_DUR));
    int opacity = round(255 * r);
    double scale = (1 - cos(M_PI * s * 2)) * 0.1 + 1.1;
    double rot_amp = (1 - cos(M_PI * s * 2)) / 2;
    double rot = 7 * sqr(rot_amp) * sin(s * 20);

    /* Background */
    SDL_SetRenderDrawColor(g_renderer, 255, 216, 192, opacity);
    SDL_RenderFillRect(g_renderer, NULL);

    /* Icon */
    sprite *q = this->quaver;
    SDL_SetTextureAlphaMod(q->tex.sdl_tex, opacity);
    render_texture_ex(q->tex, &(SDL_Rect){
            round(WIN_W / 2 - scale * q->_base.dim.w / 2),
            round(QUAVER_Y - scale * q->_base.dim.h / 2),
            scale * q->_base.dim.w,
            scale * q->_base.dim.h
        }, rot, NULL, SDL_FLIP_NONE
    );

    /* Text */
    this->text->_base.alpha = opacity;
    element_draw((element *)this->text);
}

intro_scene *intro_scene_create()
{
    intro_scene *this = malloc(sizeof(intro_scene));
    memset(this, 0, sizeof(*this));
    this->_base.tick = (scene_tick_func)intro_tick;
    this->_base.draw = (scene_draw_func)intro_draw;
    this->time = 0;

    this->quaver = sprite_create("quaver.png");
    SDL_SetTextureColorMod(this->quaver->tex.sdl_tex, C.r, C.g, C.b);

    this->text = label_create(FONT_UPRIGHT, 40,
        C, WIN_W, "Headphones recommended");
    element_place_anchored((element *)this->text,
        WIN_W / 2, TEXT_Y, 0.5, 0.5);

    return this;
}
