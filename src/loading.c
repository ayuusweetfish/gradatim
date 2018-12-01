#include "loading.h"
#include "global.h"

#include <stdbool.h>

static const int N_STRIPS = 10;
static const double STRIP_DELAY = 0.03;
static const double STRIP_DUR = 0.3;
static const double ANIM_TOT_DUR = STRIP_DELAY * (N_STRIPS - 1) + STRIP_DUR;

static const double CHK_INTERVAL = 1;

static inline int loading_thread(loading_scene *this)
{
    scene *ret = this->func(this->userdata);
    SDL_AtomicLock(&this->rw_lock);
    this->_base.b = ret;
    SDL_AtomicUnlock(&this->rw_lock);
    SDL_AtomicUnlock(&this->fin_lock);
    return 0;
}

/* Overrides base class implementation */
static void loading_tick(loading_scene *this, double dt)
{
    if ((this->_base.elapsed += dt) < ANIM_TOT_DUR)
        scene_tick(this->_base.a, dt);
    if (this->since_finish >= 0) {
        if ((this->since_finish += dt) >= ANIM_TOT_DUR) {
            *(this->_base.p) = this->_base.b;
            scene_drop(this);
        }
    } else if ((this->until_next_check -= dt) <= 0) {
        if (SDL_AtomicTryLock(&this->fin_lock)) {
            /* The routine has completed! */
            this->since_finish = 0;
            /* Resource deallocation */
            SDL_AtomicUnlock(&this->fin_lock);
            SDL_WaitThread(this->thread, NULL);
            if (this->postfunc) this->postfunc(this->userdata, this->_base.b);
        }
        this->until_next_check = CHK_INTERVAL;
    }
}

static void loading_draw(loading_scene *this)
{
    SDL_AtomicLock(&this->rw_lock);
    scene *b = this->_base.b;
    SDL_AtomicUnlock(&this->rw_lock);
    if (this->since_finish >= 0) {
        SDL_SetRenderTarget(g_renderer, this->_base.b_tex);
        scene_draw(b);
    } else {
        SDL_SetRenderTarget(g_renderer, this->_base.a_tex);
        scene_draw(this->_base.a);
    }
    SDL_SetRenderTarget(g_renderer, NULL);

    double t;
    int i;
    static SDL_Rect r[N_STRIPS];

    if (this->since_finish >= 0) {
        SDL_RenderCopy(g_renderer, this->_base.b_tex, NULL, NULL);
        t = this->since_finish;
        for (i = 0; i < N_STRIPS; ++i) {
            double tt = (t - STRIP_DELAY * i) / STRIP_DUR;
            if (tt <= 0) tt = 0;
            if (tt > 1) tt = 1; else tt = ease_quad_inout(tt);
            r[i] = (SDL_Rect){
                tt * WIN_W, i * WIN_H / N_STRIPS, WIN_W, WIN_H / N_STRIPS
            };
        }
    } else {
        SDL_RenderCopy(g_renderer, this->_base.a_tex, NULL, NULL);
        t = this->_base.elapsed;
        for (i = 0; i < N_STRIPS; ++i) {
            double tt = (t - STRIP_DELAY * i) / STRIP_DUR;
            if (tt <= 0) break;
            if (tt > 1) tt = 1; else tt = ease_quad_inout(tt);
            r[i] = (SDL_Rect){
                0, i * WIN_H / N_STRIPS, tt * WIN_W, WIN_H / N_STRIPS
            };
        }
    }
    
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
    SDL_RenderFillRects(g_renderer, r, i);
}

loading_scene *loading_create(scene **a,
    loading_routine func, loading_postroutine postfunc, void *userdata)
{
    transition_scene *_ret = transition_create(a, NULL, 1e10);
    loading_scene *ret = realloc(_ret, sizeof(loading_scene));
    if (!ret) return NULL;
    ret->_base.preserves_a = true;
    ret->_base._base.tick = (scene_tick_func)loading_tick;
    ret->_base._base.draw = (scene_draw_func)loading_draw;
    ret->func = func;
    ret->postfunc = postfunc;
    ret->userdata = userdata;
    ret->fin_lock = ret->rw_lock = 0;
    ret->until_next_check = CHK_INTERVAL;
    ret->since_finish = -1;

    /* Start the thread */
    SDL_AtomicLock(&ret->fin_lock);
    ret->thread = SDL_CreateThread(
        (SDL_ThreadFunction)loading_thread, "Loading thread", ret);

    return ret;
}
