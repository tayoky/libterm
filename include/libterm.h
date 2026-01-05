#ifndef LIBTERM_H
#define LIBTERM_H

#include <wchar.h>
#include <stdint.h>

struct term;

typedef struct term_color {
	uint8_t type;
	union {
		uint8_t r;
		uint8_t index;
	};
	uint8_t g;
	uint8_t b;
} term_color_t;

#define TERM_COLOR_DEFAULT 0
#define TERM_COLOR_ANSI    1
#define TERM_COLOR_RGB     2

typedef struct cell {
	wint_t c;
	term_color_t fg_color;
	term_color_t bg_color;
	int attr;
} cell_t;

typedef struct term_rect {
	int x;
	int y;
	int width;
	int height;
} term_rect_t;

typedef struct term_cursor {
	int x;
	int y;
	int wrap_pending;
	int attr;
	term_color_t fg_color;
	term_color_t bg_color;
} term_cursor_t;

typedef struct term_ops {
	void (*draw_cell)(struct term *, struct cell *cell, int x, int y);
	void (*draw_cursor)(struct term *, int x, int y);
	void (*clear)(struct term *, term_rect_t *rect);
	void (*move)(struct term *, term_rect_t *dest, term_rect_t *src);
} term_ops_t;

typedef struct dirty_row {
	int start_x;
	int end_x;
} dirty_row_t;

typedef struct term {
	void *data;
	term_ops_t *ops;
	cell_t *screen;
	dirty_row_t *dirty_rows;
	int params[16];
	int params_count;
	int state;
	char intermediate;
	int width;
	int height;
	term_cursor_t cursor;
	term_cursor_t saved_cursor;
	term_cursor_t dumb_saved_cursor;
	uint32_t dec_mode;
} term_t;

#define TERM_STATE_GROUND              0
#define TERM_STATE_ESCAPE              1
#define TERM_STATE_ESCAPE_INTERMEDIATE 2
#define TERM_STATE_CSI_ENTRY           3
#define TERM_STATE_CSI_PARAM           4
#define TERM_STATE_CSI_INTERMEDIATE    5
#define TERM_STATE_CSI_IGNORE          6

#define TERM_ATTR_BOLD       (1 << 1)
#define TERM_ATTR_ITALIC     (1 << 3)
#define TERM_ATTR_UNDERLINE  (1 << 4)
#define TERM_ATTR_BLINK      (1 << 5)
#define TERM_ATTR_INVERSE    (1 << 7)

#define TERM_DEC_AUTOWRAP  (1 << 0)
#define TERM_DEC_CURSOR    (1 << 1)
#define TERM_DEC_ALTBUFFER (1 << 2)

#define CELL_AT(term, x, y) (&term->screen[(y) * term->width + (x)])

void term_output_char(term_t *term, wint_t c);
void term_output(term_t *term, const char *buf, size_t size);
void term_render(term_t *term);
int term_init(term_t *term);
void term_cleanup(term_t *term);

#endif
