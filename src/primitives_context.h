#ifndef PANITENT_PRIMITIVES_CONTEXT_H_
#define PANITENT_PRIMITIVES_CONTEXT_H_

#include "precomp.h"

#include "canvas.h"

typedef struct _primitives_context {
  void (*circle)(canvas_t* canvas, int cx, int cy, int radius);
  void (*line)(canvas_t* canvas, rect_t rc);
} primitives_context_t;

void draw_circle(canvas_t* canvas, int cx, int cy, int radius);
void draw_line(canvas_t* canvas, rect_t rc);
void draw_rectangle(canvas_t*, rect_t);

extern primitives_context_t g_primitives_context;

#endif  /* PANITENT_PRIMITIVES_CONTEXT_H_ */
