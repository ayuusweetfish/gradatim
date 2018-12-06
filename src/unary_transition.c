#include "unary_transition.h"
#include "global.h"

static void ut_fade_key_handler(utransition *this, SDL_KeyboardEvent *ev)
{
    double p = this->_base.elapsed / this->_base.duration;
    if (p >= 2./3)
        scene_handle_key(this->_base.a, ev);
}

static void ut_fade_draw(utransition *this)
{
    double p = this->_base.elapsed / this->_base.duration;
    if (p <= 1./3) {
        SDL_SetTextureAlphaMod(this->_base.a_tex, iround(255 * (1 - p * 3)));
        SDL_RenderCopy(g_renderer, this->_base.a_tex, NULL, NULL);
    } else {
        if (this->cb != NULL) {
            this->cb(this->_base.a);
            this->cb = NULL;
        }
        if (p >= 2./3) {
            SDL_SetTextureAlphaMod(this->_base.a_tex, iround(255 * (p * 3 - 2)));
            SDL_RenderCopy(g_renderer, this->_base.a_tex, NULL, NULL);
        }
    }
}

utransition *utransition_fade_create(
    scene **a, double dur, utransition_callback cb)
{
    transition_scene *_base = transition_create(a, *a, dur);
    _base->_base.key_handler = (scene_key_func)ut_fade_key_handler;
    _base->preserves_a = true;
    SDL_SetTextureBlendMode(_base->a_tex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(_base->b_tex, SDL_BLENDMODE_BLEND);
    _base->t_draw = (transition_draw_func)ut_fade_draw;
    utransition *ret = realloc(_base, sizeof(utransition));
    ret->cb = cb;
    return ret;
}
