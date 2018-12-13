#include "credits.h"
#include "global.h"
#include "profile_data.h"
#include "transition.h"
#include "label.h"
#include "orion/orion.h"

static const double BGM_VOL = 0.6;
static const double BGM_LP_VOL = 0.1;

static const double V0 = 50;
static const double V = 480;
static const double A = 720;
static const int RANGE_W = WIN_W * 0.8;
static const int RANGE_H = WIN_H * 0.6;
static const int OFFS_X = (WIN_W - RANGE_W) / 2;
static const int OFFS_Y = WIN_H * 0.232;

static const char *TEXT =
    "== Overall Design (in descending order of bounciness) ==\n"
    "Shiqing\n\n"

    "==== Artists (in alphabetical order) ====\n"
    "axtoncrolley\n"
    "Calciumtrice\n"
    "David McKee (VIRiX Dreamcore)\n"
    "Delfos\n"
    "Kenney.nl\n"
    "syncopika\n\n"
    "Unless otherwise noted, all artwork assets in this project are compatible with the CC BY-SA 4.0 International licence.\n\n"

    "==== Tech fellows (in alphabetical order) ====\n"
    "IIR1 (https://github.com/berndporr/iir1), MIT licence\n"
    "Ogg Vorbis (https://xiph.org/vorbis/), BSD licence\n"
    "PortAudio (http://portaudio.com/), MIT licence\n"
    "SDL, SDL_image, SDL_ttf (http://libsdl.org/), zlib licence\n"
    "SoundTouch (https://www.surina.net/soundtouch/index.html), LGPL v2.1\n\n"

    "==== Tech helpers (in alphabetical order) ===\n"
    "Aseprite (https://www.aseprite.org/)\n"
    "Audacity (https://www.audacityteam.org/)\n"
    "FFmpeg (http://ffmpeg.org/)\n"
    "LMMS (https://lmms.io/)\n"
    "sfxr (http://www.drpetter.se/project_sfxr.html)\n"
    "SpriteSheet Packer (https://amakaseev.github.io/sprite-sheet-packer/)\n"
    "Tiled (https://www.mapeditor.org/)\n\n"

    "==== Shoutouts (in no particular order) ===\n"
    "Celeste (http://www.celestegame.com/)\n"
    "Teeworlds (https://teeworlds.com/)\n"
    "Zhiqian\n"
    "Mr Liu & the TAs\n\n"
    "The artists that bring humanity alive\n"
    "The scientists that nourished the artists\n\n"
    "The Muses\n\n"
    "You\n"
;

static void credits_tick(credits_scene *this, double dt)
{
    floue_tick(this->f, dt);
    double dv = dt * A;
    if (this->v > dv) this->v -= dv;
    else if (this->v < -dv) this->v += dv;
    this->p += (V0 + this->v) * dt;
}

static void credits_draw(credits_scene *this)
{
    floue_draw(this->f);

    /* Draw the main text */
    int h = this->text->_base.tex.range.h + RANGE_H;
    int y = iround(this->p) % h;
    if (y < 0) y += h;

    int ww = this->text->_base.tex.range.w,
        hh = RANGE_H,
        offy = 0;
    if (y > this->text->_base.tex.range.h) {
        hh = y + RANGE_H - h;
        offy = RANGE_H - hh;
        y = 0;
    } else if (y + RANGE_H > this->text->_base.tex.range.h) {
        hh = this->text->_base.tex.range.h - y;
    }

    SDL_RenderCopy(
        g_renderer, this->text->_base.tex.sdl_tex,
        &(SDL_Rect){0, y, ww, hh},
        &(SDL_Rect){OFFS_X, OFFS_Y + offy, ww, hh}
    );

    scene_draw_children((scene *)this);
}

static void credits_drop(credits_scene *this)
{
    floue_drop(this->f);
    element_drop((element *)this->text);
    orion_ramp(&g_orion, TRACKID_MAIN_BGM, 0.2, profile.bgm_vol * VOL_VALUE);
    orion_ramp(&g_orion, TRACKID_MAIN_BGM_LP, 0.2, 0);
}

static void credits_key(credits_scene *this, SDL_KeyboardEvent *ev)
{
    if (ev->state != SDL_PRESSED) return;
    switch (ev->keysym.sym) {
        case SDLK_ESCAPE:
            g_stage = transition_slidedown_create(&g_stage, this->bg, 0.5);
            break;
        case SDLK_LEFT:
        case SDLK_UP:
            this->v = -V - V0; break;
        case SDLK_RIGHT:
        case SDLK_DOWN:
        case SDLK_RETURN:
            this->v = +V - V0; break;
    }
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

    label *header = label_create(FONT_UPRIGHT, 60,
        (SDL_Color){0, 0, 0}, WIN_W, "CREDITS");
    bekter_pushback(this->_base.children, header);
    element_place_anchored((element *)header, WIN_W / 15, WIN_H / 7, 0, 0.5);

    label *text = label_create(FONT_ITALIC, 32,
        (SDL_Color){0, 0, 0}, RANGE_W, TEXT);
    this->text = text;

    this->v = -V0;
    this->p = -RANGE_H / 2;

    label *footer = label_create(FONT_UPRIGHT, 32, (SDL_Color){0}, WIN_H, "");
    bekter_pushback(this->_base.children, footer);
    label_set_keyed_text(footer,
        " ..`.  ..`.  Scroll      ..`.  Back", "^v~");
    element_place_anchored((element *)footer, WIN_W / 2, WIN_H * 0.9, 0.5, 0.5);

    orion_ramp(&g_orion, TRACKID_MAIN_BGM, 0.2, BGM_LP_VOL * profile.bgm_vol * VOL_VALUE * 2 / 3);
    orion_ramp(&g_orion, TRACKID_MAIN_BGM_LP, 0.2, BGM_VOL * profile.bgm_vol * VOL_VALUE);
    orion_ramp(&g_orion, TRACKID_MAIN_BGM_CANON, 0.2, BGM_LP_VOL * profile.bgm_vol * VOL_VALUE / 3);

    return this;
}
