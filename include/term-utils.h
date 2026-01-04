#ifndef TERM_UTILS_H
#define TERM_UTILS_H

#include <libterm.h>

void term_scroll(term_t *term, int amount);
void term_set_cursor(term_t *term, int x, int y);
void term_move_cursor(term_t *term, int x, int y);
void term_clamp_cursor(term_t *term);
void term_carriage_return(term_t *term);
void term_newline(term_t *term);
void term_tab(term_t *term);

void term_reset(term_t *term);

void term_clear(term_t *term, term_rect_t *rect);
void term_move(term_t *term, term_rect_t *dest, term_rect_t *src);
void term_draw_cell(term_t *term, cell_t *cell, int x, int y);
void term_draw_cursor(term_t *term, int x, int y);
void term_csi_dispatch(term_t *term, wint_t final);
void term_esc_dispatch(term_t *term, wint_t final);

#endif
