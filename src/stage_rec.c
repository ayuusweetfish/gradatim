#include "stage_rec.h"
#include <stdio.h>

struct stage_rec *stage_read(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return NULL;

    struct stage_rec *this = malloc(sizeof(*this));
    if (!this) return NULL;
    memset(this, 0, sizeof(*this));

    int worldr, worldc, nrows, ncols;
    int cam_r1, cam_c1, cam_r2, cam_c2;
    int i, j;

    /* TODO: Report all fscanf()'s incorrect return values? */
    fscanf(f, "%d,%d", &worldr, &worldc);
    fscanf(f, "%d,%d", &nrows, &ncols);
    fscanf(f, "%d,%d,%d,%d", &cam_r1, &cam_c1, &cam_r2, &cam_c2);
    fscanf(f, "%d,%d", &this->spawn_r, &this->spawn_c);

    this->sim = sim_create(nrows, ncols);
    this->sim->worldr = worldr;
    this->sim->worldc = worldc;
    this->sim->prot.x = this->spawn_c;
    this->sim->prot.y = this->spawn_r;
    this->sim->prot.w = this->sim->prot.h = 0.6;
    this->cam_r1 = cam_r1;
    this->cam_c1 = cam_c1;
    this->cam_r2 = cam_r2;
    this->cam_c2 = cam_c2;

    for (i = 0; i < nrows; ++i)
        for (j = 0; j < ncols; ++j) {
            int t;
            fscanf(f, "%d", &t);
            fgetc(f);
            sim_grid(this->sim, i, j).tag = t;
        }

    int m;

    fscanf(f, "%d", &m);
    for (i = 0; i < m; ++i) {
        int r, c, tag;
        double x1, y1, x2, y2, t;
        fscanf(f, "%d,%d,%d,%lf,%lf,%lf,%lf,%lf",
            &r, &c, &tag, &y1, &x1, &y2, &x2, &t);
        sobj *o = malloc(sizeof(sobj));
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
        sim_add(this->sim, o);
    }

    /* TODO: This should be replaced */
    this->prot_tex = retrieve_texture("uwu.png");
    this->prot_fail_tex[0] = retrieve_texture("fragile1.png");
    this->prot_fail_tex[1] = retrieve_texture("fragile2.png");
    this->prot_fail_tex[2] = retrieve_texture("fragile3.png");
    this->prot_fail_tex[3] = retrieve_texture("fragile4.png");
    this->grid_tex[1] = retrieve_texture("block.png");
    this->grid_tex[OBJID_SPRING] = retrieve_texture("spring1.png");
    this->grid_tex[OBJID_SPRING_PRESS] = retrieve_texture("spring2.png");
    this->grid_tex[OBJID_CLOUD_ONEWAY] =
    this->grid_tex[OBJID_CLOUD_RTRIP] = retrieve_texture("cloud.png");
    this->grid_tex[OBJID_FRAGILE] = retrieve_texture("fragile1.png");
    this->grid_tex[OBJID_FRAGILE + 1] = retrieve_texture("fragile2.png");
    this->grid_tex[OBJID_FRAGILE + 2] = retrieve_texture("fragile3.png");
    this->grid_tex[OBJID_FRAGILE + 3] = retrieve_texture("fragile4.png");
    this->grid_tex[OBJID_BILLOW] = retrieve_texture("fragile1.png");
    this->grid_tex[OBJID_BILLOW + 1] = retrieve_texture("fragile3.png");
    this->grid_tex[OBJID_BILLOW + 2] = retrieve_texture("fragile2.png");
    this->grid_tex[OBJID_MUSHROOM_T] = retrieve_texture("mushroom_t.png");
    this->grid_tex[OBJID_MUSHROOM_B] = retrieve_texture("mushroom_b.png");
    this->grid_tex[OBJID_MUSHROOM_BR] =
    this->grid_tex[OBJID_MUSHROOM_BL] =
    this->grid_tex[OBJID_MUSHROOM_TR] =
    this->grid_tex[OBJID_MUSHROOM_TL] = retrieve_texture("mushroom_tl.png");
    this->grid_tex[OBJID_REFILL] = retrieve_texture("refill1.png");
    this->grid_tex[OBJID_REFILL + 1] = retrieve_texture("refill2.png");
    this->grid_tex[OBJID_REFILL + 2] = retrieve_texture("refill3.png");
    this->grid_tex[OBJID_REFILL + 3] = retrieve_texture("refill4.png");
    this->grid_tex[OBJID_PUFF_L] = retrieve_texture("puff_l.png");
    this->grid_tex[OBJID_PUFF_L_CURL] = retrieve_texture("spring1.png");
    this->grid_tex[OBJID_PUFF_L_AFTER] = retrieve_texture("fragile2.png");
    this->grid_tex[OBJID_PUFF_R] = retrieve_texture("puff_r.png");
    this->grid_tex[OBJID_MUD] = retrieve_texture("fragile1.png");
    this->grid_tex[OBJID_WET] = retrieve_texture("fragile2.png");

    this->strtab = bekter_create();
    this->plot = bekter_create();

    fclose(f);
    return this;
}

void stage_drop(struct stage_rec *this)
{
    bekter_drop(this->strtab);
    bekter_drop(this->plot);
    free(this);
}
