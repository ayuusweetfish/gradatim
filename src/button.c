#include "button.h"
#include "global.h"

static const int ANIM_DUR = 0.2 * 1000;

static void button_tick(button *this, double dt)
{
    char s = (this->_base.mouse_in ? (this->_base.mouse_down ? 2 : 1) : 0);
    if (s == 1 && this->last_s == 2)
        if (this->cb) this->cb();
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
    if (this->agl_s != -1) {
        SDL_SetTextureAlphaMod(this->tex[this->agl_s], 255);
        SDL_RenderCopy(this->_base.renderer, this->tex[this->agl_s], NULL, &(SDL_Rect){
            this->_base.dim.x + (this->sz[0].x - this->cur_sz.x) / 2,
            this->_base.dim.y + (this->sz[0].y - this->cur_sz.y) / 2,
            this->cur_sz.x, this->cur_sz.y
        });
        /* XXX: DRY? */
        unsigned now = SDL_GetTicks();
        double p = 1 - (double)(this->agl_time + ANIM_DUR - now) / ANIM_DUR;
        SDL_SetTextureAlphaMod(this->tex[s], round(p * 255));
    } else {
        SDL_SetTextureAlphaMod(this->tex[s], 255);
    }
    SDL_RenderCopy(this->_base.renderer, this->tex[s], NULL, &(SDL_Rect){
        this->_base.dim.x + (this->sz[0].x - this->cur_sz.x) / 2,
        this->_base.dim.y + (this->sz[0].y - this->cur_sz.y) / 2,
        this->cur_sz.x, this->cur_sz.y
    });
}

static void button_drop(button *this)
{
    SDL_DestroyTexture(this->tex[0]);
    if (this->tex[1] != this->tex[0])
        SDL_DestroyTexture(this->tex[1]);
    if (this->tex[2] != this->tex[0] && this->tex[2] != this->tex[1])
        SDL_DestroyTexture(this->tex[2]);
}

element *button_create(SDL_Renderer *rdr, button_callback cb,
    const char *img_idle, const char *img_focus, const char *img_down,
    float scale_focus, float scale_down)
{
    button *ret = malloc(sizeof(button));
    ret->_base.renderer = rdr;
    ret->_base.mouse_in = ret->_base.mouse_down = false;
    ret->_base.tick = (element_tick_func)button_tick;
    ret->_base.draw = (element_draw_func)button_draw;
    ret->_base.drop = (element_drop_func)button_drop;
    ret->cb = cb;
    int w, h;
    ret->tex[0] = load_texture(rdr, img_idle, &w, &h);
    ret->sz[0] = (SDL_Point){w, h};
    ret->_base.dim.w = w;
    ret->_base.dim.h = h;
    ret->tex[1] = (img_focus == NULL) ?
        ret->tex[0] : load_texture(rdr, img_focus, &w, &h);
    ret->sz[1] = (SDL_Point){round(w * scale_focus), round(h * scale_focus)};
    ret->tex[2] = (img_down == NULL) ?
        ret->tex[1] : load_texture(rdr, img_down, &w, &h);
    ret->sz[2] = (SDL_Point){round(w * scale_down), round(h * scale_down)};
    ret->last_s = 0;
    ret->cur_sz = ret->sz[0];
    ret->agl_s = -1;
    return (element *)ret;
}
