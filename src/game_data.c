#include "game_data.h"
#include "dialogue.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct stage_rec *stage_read(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return NULL;

    struct stage_rec *this = malloc(sizeof(*this));
    if (!this) { fclose(f); return NULL; }
    memset(this, 0, sizeof(*this));

    this->strtab = bekter_create();
    this->plot = bekter_create();

    int i, j;

    /* TODO: Report all fscanf()'s incorrect return values? */
    fscanf(f, "%d,%d", &this->world_r, &this->world_c);
    fscanf(f, "%d,%d", &this->n_rows, &this->n_cols);
    fscanf(f, "%d,%d,%d,%d",
        &this->cam_r1, &this->cam_c1, &this->cam_r2, &this->cam_c2);
    fscanf(f, "%d,%d", &this->spawn_r, &this->spawn_c);

    /* The grid needs only tags, in order to save memory */
    this->grid = malloc(sizeof(*this->grid) * this->n_rows * this->n_cols);
    if (!this->grid) { fclose(f); return NULL; }

    for (i = 0; i < this->n_rows; ++i)
        for (j = 0; j < this->n_cols; ++j) {
            int t;
            fscanf(f, "%d", &t);
            if (t == -1) t = 0;
            fgetc(f);
            this->grid[i * this->n_cols + j] = t;
        }

    int m;

    /* Animate objects; or objects that do not fit into the grid */
    fscanf(f, "%d", &m);
    this->n_anim = m;
    this->anim = malloc(sizeof(*this->anim) * m);
    if (!this->anim) { fclose(f); return NULL; }

    for (i = 0; i < m; ++i) {
        int r, c, tag;
        double x1, y1, x2, y2, t;
        fscanf(f, "%d,%d,%d,%lf,%lf,%lf,%lf,%lf",
            &r, &c, &tag, &y1, &x1, &y2, &x2, &t);
        sobj *o = &this->anim[i];
        memset(o, 0, sizeof(sobj));
        o->tag = tag;
        o->x = c;
        o->y = r;
        o->w = o->h = 1;
        o->vx = x1;
        o->vy = y1;
        o->ax = x2;
        o->ay = y2;
        o->t = t;
    }

    /* String table */
    fscanf(f, "%d", &m);
    fgetc(f);   /* Skip the newline character */
    char s[1024];
    for (i = 0; i < m; ++i) {
        fgets(s, sizeof s, f);
        int p = strlen(s) - 1;
        while (p > 0 && isspace(s[p])) s[p--] = '\0';
        bekter_pushback(this->strtab, strdup(s));
    }

    /* List of dialogues */
    fscanf(f, "%d", &m);
    fgetc(f);
    this->plot_ct = m;
    for (i = 0; i < m; ++i) {
        struct _stage_dialogue d;
        int face;
        fscanf(f, "%d,%d,%d,%d,%d,", &d.r1, &d.c1, &d.r2, &d.c2, &face);
        d.face = face;
        d.content = bekter_create();
        int j = 0, a, b, c;
        while (true) {
            fscanf(f, "%d,%d,%d", &a, &b, &c);
            dialogue_entry e = (dialogue_entry){
                retrieve_texture(bekter_at(this->strtab, a, char *)),
                bekter_at(this->strtab, b, char *),
                bekter_at(this->strtab, c, char *)
            };
            bekter_pushback(d.content, e);
            if (fgetc(f) == '\n') break;
        }
        bekter_pushback(this->plot, d);
    }

    /* List of audio sources */
    fscanf(f, "%d", &m);
    fgetc(f);
    this->aud_ct = m;
    for (i = 0; i < m; ++i) {
        int tid, r, c;
        fscanf(f, "%d,%d,%d", &tid, &r, &c);
        this->aud[i] = (struct _stage_audsrc){(char)tid, r, c};
    }

    /* List of hints */
    fscanf(f, "%d", &m);
    this->hint_ct = m;
    for (i = 0; i < m; ++i) {
        stage_hint h;
        int stridx, keyidx, imgidx;
        fscanf(f, "%d,%d,%d,%d,%d,%d,%d",
            &h.r, &h.c, &stridx, &keyidx, &imgidx, &h.mul, &h.mask);
        h.str = (stridx == -1 ? NULL : bekter_at(this->strtab, stridx, char *));
        h.key = (keyidx == -1 ? NULL : bekter_at(this->strtab, keyidx, char *));
        h.img = (imgidx == -1 ? NULL : bekter_at(this->strtab, imgidx, char *));
        this->hints[i] = h;
    }

    /* TODO: This should be replaced */
    this->prot_tex = retrieve_texture("uwu.png");
    this->prot_fail_tex[0] = retrieve_texture("fragile1.png");
    this->prot_fail_tex[1] = retrieve_texture("fragile2.png");
    this->prot_fail_tex[2] = retrieve_texture("fragile3.png");
    this->prot_fail_tex[3] = retrieve_texture("fragile4.png");
    for (i = 1; i < 256; ++i)
        this->grid_tex[i] = grid_texture(i);

    fclose(f);
    return this;
}

sim *stage_create_sim(struct stage_rec *this)
{
    sim *s = sim_create(this->n_rows, this->n_cols);
    s->worldr = this->world_r;
    s->worldc = this->world_c;
    s->prot.x = this->spawn_c;
    s->prot.y = this->spawn_r;
    s->prot.w = s->prot.h = 0.625;

    int i, j;

    /* Populate the grid */
    for (i = 0; i < this->n_rows; ++i)
        for (j = 0; j < this->n_cols; ++j) {
            unsigned char t = this->grid[i * this->n_cols + j];
            sim_grid(s, i, j).tag = t;
            int tx, ty;
            grid_offset(t, &tx, &ty);
            sim_grid(s, i, j).tx = -tx * 1./16;
            sim_grid(s, i, j).ty = -ty * 1./16;
        }

    /* Initialize all animate objects */
    for (i = 0; i < this->n_anim; ++i) {
        int tx, ty;
        grid_offset(this->anim[i].tag, &tx, &ty);
        this->anim[i].tx = -tx * 1./16;
        this->anim[i].ty = -ty * 1./16;
        sim_add(s, &this->anim[i]);
    }

    sim_reinit(s);
    return s;
}

void stage_drop(struct stage_rec *this)
{
    int i;
    char *s;
    for bekter_each(this->strtab, i, s) free(s);
    bekter_drop(this->strtab);

    struct _stage_dialogue d;
    for bekter_each(this->plot, i, d) bekter_drop(d.content);
    bekter_drop(this->plot);

    free(this->grid);
    free(this->anim);
    free(this);
}

struct chap_rec *chap_read(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return NULL;

    struct chap_rec *this = malloc(sizeof(*this));
    if (!this) { fclose(f); return NULL; }
    memset(this, 0, sizeof(*this));

    fscanf(f, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
        &this->r1, &this->g1, &this->b1,
        &this->r2, &this->g2, &this->b2,
        &this->r3, &this->g3, &this->b3);

    this->idx = -1;

    int bpm, beat_mul, sig, dmask = 0, hmask = 0;
    char measure[64];
    int m, i;

    fgets(measure, sizeof measure, f);
    i = strlen(measure) - 1;
    while (i >= 0 && isspace(measure[i])) measure[i--] = '\0';
    this->title = strdup(measure);

    fscanf(f, "%d,%d\n", &bpm, &beat_mul);
    if (bpm <= 0) { free(this); fclose(f); return NULL; }
    fgets(measure, sizeof measure, f);

    sig = strlen(measure) - 1;
    while (sig >= 0 && isspace(measure[sig])) measure[sig--] = '\0';
    if (++sig == 0) { free(this); fclose(f); return NULL; }
    for (i = 0; i < sig; ++i) switch (measure[i]) {
        case 'x': dmask |= (1 << i);    /* Fallthrough */
        case 'o': hmask |= (1 << i);
        default: break;
    }

    this->bpm = bpm;
    this->beat = 60.0 / bpm;
    this->beat_mul = beat_mul;
    this->sig = sig;
    this->dash_mask = dmask;
    this->hop_mask = hmask;

    fscanf(f, "%lf,%d", &this->offs, &this->loop);

    fscanf(f, "%d", &m);
    if (m > MAX_CHAP_TRACKS) { free(this); fclose(f); return NULL; }
    this->n_tracks = m;
    for (i = 0; i < m; ++i) {
        int src_id;
        char str[64];
        int p;
        double arg;
        fscanf(f, "%d", &src_id);
        fgetc(f);   /* Skip the comma */
        for (p = 0; (str[p] = fgetc(f)) != ','; ++p) ;
        str[p] = '\0';
        fscanf(f, "%lf", &arg);
        this->tracks[i] = (struct _chap_track){src_id, strdup(str), arg};
    }

    fscanf(f, "%d", &m);
    /* XXX: More things to free */
    if (m > MAX_SIDESCROLLERS) { free(this); fclose(f); return NULL; }
    this->n_ss = m;
    for (i = 0; i < m; ++i) {
        fgetc(f);
        char str[64];
        int p;
        double scale, xskip, y, mul, vx;
        for (p = 0; (str[p] = fgetc(f)) != ','; ++p) ;
        str[p] = '\0';
        fscanf(f, "%lf,%lf,%lf,%lf,%lf", &scale, &xskip, &y, &mul, &vx);
        this->ss[i] = (struct _chap_ss){strdup(str), scale, xskip, y, mul, vx};
    }

    fscanf(f, "%d", &m);
    fgetc(f);   /* Skip the newline character */
    if (m > MAX_CHAP_STAGES) { free(this); fclose(f); return NULL; }
    this->n_stages = m;

    int world_r = 0, world_c = 0;
    for (i = 0; i < m; ++i) {
        char s[64];
        fgets(s, sizeof s, f);
        int p = strlen(s) - 1;
        while (p > 0 && isspace(s[p])) s[p--] = '\0';
        this->stages[i] = stage_read(s);
        world_r = (this->stages[i]->world_r += world_r);
        world_c = (this->stages[i]->world_c += world_c);
#ifndef NDEBUG
        printf("Stage #%d: %d %d\n", i + 1, world_r, world_c);
#endif
    }

    fclose(f);
    return this;
}

void chap_drop(struct chap_rec *this)
{
    free(this->title);
    int i;
    for (i = 0; i < this->n_ss; ++i) free(this->ss[i].image);
    for (i = 0; i < this->n_tracks; ++i) free(this->tracks[i].str);
    for (i = 0; i < this->n_stages; ++i) stage_drop(this->stages[i]);
    free(this);
}
