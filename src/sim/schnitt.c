#include "schnitt.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef SCHNITT_TEST
    #include <assert.h>
    #include <math.h>
    #include <stdio.h>
#else
    #define assert(__x)
#endif

/* These assume that argument expressions have no side effects */
#define min(__a, __b) ((__a) < (__b) ? (__a) : (__b))
#define max(__a, __b) ((__a) > (__b) ? (__a) : (__b))
#define sign(__x) ((__x) < 0 ? -1 : +1)

#define MAX_RECTS   64

/* Global minimum & maximum coordinates */
static float xmin = 1, xmax = 0, ymin = 1, ymax = 0;

/* Vertical segments; tag = 0/1 denotes left/right borders */
static int m = 0;
static struct vert_seg {
    float x, y1, y2;
    bool tag;
} v[MAX_RECTS * 2];

/* Vertices of the resulting polygon */
static int n;
static struct point {
    float x, y;
} p[MAX_RECTS * 4];

void schnitt_apply(float x1, float y1, float x2, float y2)
{
    /* Sanitize input */
    x1 = max(0, min(1, x1)); y1 = max(0, min(1, y1));
    x2 = max(0, min(1, x2)); y2 = max(0, min(1, y2));
    float _x1 = min(x1, x2), _y1 = min(y1, y2);
    float _x2 = max(x1, x2), _y2 = max(y1, y2);
    if (_x1 == _x2 || _y1 == _y2) return;
    xmin = min(xmin, x1); ymin = min(ymin, y1);
    xmax = max(xmax, x2); ymax = max(ymax, y2);
    v[m++] = (struct vert_seg){_x1, _y1, _y2, false};
    v[m++] = (struct vert_seg){_x2, _y1, _y2, true};
}

static int vert_seg_cmp(const void *_a, const void *_b)
{
    struct vert_seg *a = (struct vert_seg *)_a, *b = (struct vert_seg *)_b;
    return (a->x == b->x ? (int)a->tag - (int)b->tag : sign(a->x - b->x));
}

/* Temporary store for all critical points' vertical coordinates at current `x`
 * tag = 0/1 denotes upper/lower borders */
static int t;
static struct crit_pt {
    float y;
    bool tag;
} q[MAX_RECTS * 2];

static inline void add_crit_pt(float y, bool tag)
{
    if (t == 0 || q[t - 1].y <= y) {
        q[t++] = (struct crit_pt){y, tag};
        return;
    }
    int i;
    for (i = t - 1; i >= 0; --i) {
        q[i + 1] = q[i];
        if (i == 0 || (q[i].y > y && q[i - 1].y <= y)) {
            q[i] = (struct crit_pt){y, tag};
            break;
        }
    }
    ++t;
}

static inline void del_crit_pt(float y, bool tag)
{
    int i;
    for (i = 0; i < t; ++i)
        if (q[i].y == y && q[i].tag == tag) break;
    for (; i < t - 1; ++i) q[i] = q[i + 1];
    --t;
}

/* For maintaining the difference between adjacent x's */
static int rn, rcnt[2];
static float r[2][MAX_RECTS * 2];

/* Goes through the list of critical points
 * and create turning points in the output list `p` */
static inline void process_crits(float x)
{
    /* Firstly, find out the ranges of the current vertcal line
     * whose right neighbourhoods are covered by some rectangle.
     * These are stored in `r[rn]`. */
    int i, layers = 1;
    float y_in = 0;
    rcnt[rn] = 0;
    for (i = 0; i < t; ++i) {
        if (layers == 1 && y_in != -1) {
            if (y_in != q[i].y) {
                r[rn][rcnt[rn]++] = y_in;
                r[rn][rcnt[rn]++] = q[i].y;
            }
            y_in = -1;  /* Prevent [+    -+-+-+    -] */
        }
        layers += (q[i].tag ? +1 : -1);
        if (layers == 1 && y_in == -1) y_in = q[i].y;
    }
    assert(layers == 1);
    if (x != 1 && y_in != -1 && y_in != 1) {
        r[rn][rcnt[rn]++] = y_in;
        r[rn][rcnt[rn]++] = 1;
    }

    /* The difference between two lists (`r`) is the set of turning points.
     * This is calculated with merge sort;
     * here all elements are granted to be distinct in each list. */
    int j;
    i = j = 0;
    while (i < rcnt[rn] || j < rcnt[rn ^ 1]) {
        if (j == rcnt[rn ^ 1] || (i != rcnt[rn] && r[rn][i] < r[rn ^ 1][j])) {
            p[n++] = (struct point){x, r[rn][i++]};
        } else if (i == rcnt[rn] || (j != rcnt[rn ^ 1] && r[rn][i] > r[rn ^ 1][j])) {
            p[n++] = (struct point){x, r[rn ^ 1][j++]};
        } else if (r[rn][i] == r[rn ^ 1][j]) {
            /* See test case #8 in `schnitt_test.c` */
            if ((i & 1) ^ (j & 1))
                p[n++] = (struct point){x, r[rn][i]};
            i++; j++;
        }
    }
    rn ^= 1;
}

/* Processes all cutting rectangles and stores the responses in `dx` and `dy`
 * in this order: up, left, right, down, ul, ur, dl, dr
 * In accordance with SDL, this assumes a left-handed coordinate system */
void schnitt_flush(float *dx, float *dy)
{
    /* Sort all vertical segments */
    qsort(v, m, sizeof v[0], vert_seg_cmp);

    /* Sweep line */
    n = t = 0;
    rn = 0;
    rcnt[rn ^ 1] = 0;
    if (m == 0 || v[0].x != 0) process_crits(0);
    int i;
    for (i = 0; i < m; ++i) {
        /* Update critical points */
        /* XXX: Speedup possible? */
        if (v[i].tag == false) {
            add_crit_pt(v[i].y1, false);
            add_crit_pt(v[i].y2, true);
        } else {
            del_crit_pt(v[i].y1, false);
            del_crit_pt(v[i].y2, true);
        }
        /* Check all critical points */
        float next_x = (i == m - 1 ? 1 : v[i + 1].x);
        if (v[i].x != next_x)
            process_crits(v[i].x);
    }
    /* At x = 1, the vertical line's right neighbourhood won't be covered
     * by any line, therefore `q` should be cleared (set `t` to 0). */
    t = 0;
    process_crits(1);

    /* Calculate responses */
    if (n == 0) {
        for (i = 0; i < 8; ++i) dx[i] = dy[i] = 0;
    } else {
        dy[0] = -1 + ymin; dx[0] = 0;
        dx[1] = -1 + xmin; dy[1] = 0;
        dx[2] = xmax; dy[2] = 0;
        dy[3] = ymax; dx[3] = 0;

        int idx_mx1, idx_mn1, idx_mx2, idx_mn2;
        float mx1 = -5, mn1 = 5, mx2 = -5, mn2 = 5, u;
        for (i = 0; i < n; ++i) {
            u = p[i].x + p[i].y;
            if (u > mx1) { mx1 = u; idx_mx1 = i; }
            if (u < mn1) { mn1 = u; idx_mn1 = i; }
            u = p[i].x - p[i].y;
            if (u > mx2) { mx2 = u; idx_mx2 = i; }
            if (u < mn2) { mn2 = u; idx_mn2 = i; }
        }
        dx[4] = p[idx_mx1].x - 1;
        dy[4] = p[idx_mx1].y - 1;
        dx[5] = p[idx_mn2].x;
        dy[5] = p[idx_mn2].y - 1;
        dx[6] = p[idx_mx2].x - 1;
        dy[6] = p[idx_mx2].y;
        dx[7] = p[idx_mn1].x;
        dy[7] = p[idx_mn1].y;
    }

    /* Cleanup */
    m = 0;
    xmin = 1, xmax = 0, ymin = 1, ymax = 0;
}

#ifdef SCHNITT_TEST
static int case_num = 0;
static float dx[8], dy[8];

bool schnitt_check(int _n, float *x, float *y)
{
    schnitt_flush(dx, dy);
    ++case_num;
    if (_n == -1) return true;

    int i, j;
    bool valid = true;
    if (_n != n) valid = false;
    else for (i = 0; i < n; ++i) {
        bool found = false;
        /* We assume that provided points do not coincide */
        for (j = 0; j < n; ++j)
            if (p[j].x == x[i] && p[j].y == y[i]) {
                found = true; break;
            }
        if (!found) { valid = false; break; }
    }

    if (!valid) {
        printf("[Case %d] Incorrect\n", case_num);
        printf("== Actual (%d) ==\n", n);
        for (i = 0; i < n; ++i) printf("%.4f %.4f\n", p[i].x, p[i].y);
        printf("== Expected (%d) ==\n", _n);
        for (i = 0; i < _n; ++i) printf("%.4f %.4f\n", x[i], y[i]);
        return false;
    }
    printf("[Case %d] Correct! ^ ^\n", case_num);
    return true;
}

bool schnitt_check_d(int dir, float dx0, float dy0)
{
    if (fabs(dx0 - dx[dir]) + fabs(dy0 - dy[dir]) >= 1e-6) {
        printf("[Case %d] Incorrect movement\n", case_num);
        printf("Actual   %.4f %.4f\n", dx[dir], dy[dir]);
        printf("Expected %.4f %.4f\n", dx0, dy0);
        return false;
    } else {
        printf("[Case %d] Correct movement ≥ ≤\n", case_num);
        return true;
    }
}
#endif
