#include "button.h"
#include "global.h"

static const int ANIM_DUR = 0.2 * 1000;

static void button_tick(button *this, double dt)
{
    char s = (this->_base.mouse_in ? (this->_base.mouse_down ? 2 : 1) : 0);
    if (s == 1 && this->last_s == 2)
        if (this->cb) this->cb(this->ud);
    unsigned now = SDL_GetTicks();
    if (this->last_s != s) {
        this->agl_s = this->last_s;
        this->agl_sz = this->cur_sz;
        this->agl_time = now;
        this->last_s = s;
    }
    if (this->agl_s != -1) {
        if (now <= this->agl_time + ANIM_DUR) {
            double p = 1 - (double)(this->agl_time + ANIM_DUR - now) / ANIM_DUR;
            this->cur_sz.x =
                round(this->agl_sz.x + p * (this->sz[s].x - this->agl_sz.x));
            this->cur_sz.y =
                round(this->agl_sz.y + p * (this->sz[s].y - this->agl_sz.y));
        } else {
            this->agl_s = -1;
        }
    }
}

static void button_draw(button *this)
{
    char s = (this->_base.mouse_in ? (this->_base.mouse_down ? 2 : 1) : 0);
    int alpha;
    if (this->agl_s != -1) {
        render_texture_alpha(this->tex[this->agl_s], &(SDL_Rect){
            this->_base.dim.x + (this->sz[0].x - this->cur_sz.x) / 2,
            this->_base.dim.y + (this->sz[0].y - this->cur_sz.y) / 2,
            this->cur_sz.x, this->cur_sz.y
        }, 255);
        /* XXX: DRY? */
        unsigned now = SDL_GetTicks();
        double p = 1 - (double)(this->agl_time + ANIM_DUR - now) / ANIM_DUR;
        alpha = round(p * 255);
    } else {
        alpha = 255;
    }
    render_texture_alpha(this->tex[s], &(SDL_Rect){
        this->_base.dim.x + (this->sz[0].x - this->cur_sz.x) / 2,
        this->_base.dim.y + (this->sz[0].y - this->cur_sz.y) / 2,
        this->cur_sz.x, this->cur_sz.y
    }, alpha);
}

button *button_create(button_callback cb, void *ud,
    const char *img_idle, const char *img_focus, const char *img_down,
    float scale_focus, float scale_down)
{
    button *ret = malloc(sizeof(button));
    ret->_base.mouse_in = ret->_base.mouse_down = false;
    ret->_base.tick = (element_tick_func)button_tick;
    ret->_base.draw = (element_draw_func)button_draw;
    ret->_base.drop = NULL;
    ret->cb = cb;
    ret->ud = ud;
    int w, h;
    ret->tex[0] = retrieve_texture(img_idle);
    w = ret->tex[0].range.w;
    h = ret->tex[0].range.h;
    ret->sz[0] = (SDL_Point){w, h};
    ret->_base.dim.w = w;
    ret->_base.dim.h = h;
    ret->tex[1] = (img_focus == NULL) ?
        ret->tex[0] : retrieve_texture(img_focus);
    ret->sz[1] = (SDL_Point){round(w * scale_focus), round(h * scale_focus)};
    ret->tex[2] = (img_down == NULL) ?
        ret->tex[1] : retrieve_texture(img_down);
    ret->sz[2] = (SDL_Point){round(w * scale_down), round(h * scale_down)};
    ret->last_s = 0;
    ret->cur_sz = ret->sz[0];
    ret->agl_s = -1;
    return ret;
}
