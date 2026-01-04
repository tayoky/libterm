#include <libterm.h>
#include <term-utils.h>

void term_draw_cell(term_t *term, cell_t *cell, int x, int y) {
	if (term->ops && term->ops->draw_cell) {
		term->ops->draw_cell(term, cell, x, y);
	}
}

void term_draw_cursor(term_t *term, int x, int y) {
	if (term->ops && term->ops->draw_cursor) {
		term->ops->draw_cursor(term, x, y);
	}
}

void term_move(term_t *term, term_rect_t *dest, term_rect_t *src) {
	if (dest->width == term->width) {
		memmove(CELL_AT(term, dest->x, dest->y), CELL_AT(term, src->x, src->y), dest->width * dest->height * sizeof(cell_t));
	} else {
		// FIXME : might not work if src y  < dest y
		for (int i=0; i<dest->height; i++) {
			memmove(CELL_AT(term, dest->x, dest->y + i), CELL_AT(term, src->x, src->y + i), dest->width * sizeof(cell_t));
		}
	}
	if (term->ops && term->ops->move) {
		term->ops->move(term, dest, src);
	} else {
		for (int i=0; i<dest->height; i++) {
			cell_t *cell = CELL_AT(term, dest->x, dest->y + i);
			for (int j=0; j<dest->width; j++) {
				term_draw_cell(term, cell, dest->x + j, dest->y + i);
				cell++;
			}
		}
	}
}

void term_clear(term_t *term, term_rect_t *rect) {
	if (rect->width <= 0 || rect->height <= 0) return;
	for (int i=0; i<rect->height; i++) {
		cell_t *cell = CELL_AT(term, rect->x, rect->y + i);
		for (int j=0; j<rect->width; j++) {
			cell->c        = ' ';
			cell->attr     = term->attr;
			cell->fg_color = term->fg_color;
			cell->bg_color = term->bg_color;
			cell++;
		}
	}
	if (term->ops && term->ops->clear) {
		term->ops->clear(term, rect);
	} else {
		cell_t *cell = CELL_AT(term, rect->x, rect->y);
		for (int i=0; i<rect->width; i++) {
			for (int j=0; j<rect->height; j++) {
				term_draw_cell(term, cell, rect->x + i, rect->y + j);
			}
		}
	}
}
