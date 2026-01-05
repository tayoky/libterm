#include <libterm.h>
#include <term-utils.h>
#include <string.h>

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
	if (dest->width <= 0 || dest->height <= 0) return;
	if (dest->width == term->width) {
		memmove(CELL_AT(term, dest->x, dest->y), CELL_AT(term, src->x, src->y), dest->width * dest->height * sizeof(cell_t));
	} else {
		// FIXME : might not work if src y  < dest y
		for (int i=0; i<dest->height; i++) {
			memmove(CELL_AT(term, dest->x, dest->y + i), CELL_AT(term, src->x, src->y + i), dest->width * sizeof(cell_t));
		}
	}
	if (term->ops && term->ops->move) {
		int end_y = dest->y + dest->height;
		int src_y = src->y;
		int off_x = dest->x - src->x;
		for (int dest_y=dest->y; dest_y<end_y; dest_y++, src_y++) {
			// TODO : maybee clip the row
			term_invalidate_row(term, dest_y, term->dirty_rows[src_y].start_x + off_x, term->dirty_rows[src_y].end_x + off_x);
		}
		term->ops->move(term, dest, src);
	} else {
		term_invalidate_rect(term, dest);
	}
}

void term_clear(term_t *term, term_rect_t *rect) {
	if (rect->width <= 0 || rect->height <= 0) return;
	for (int i=0; i<rect->height; i++) {
		cell_t *cell = CELL_AT(term, rect->x, rect->y + i);
		for (int j=0; j<rect->width; j++) {
			cell->c        = ' ';
			cell->attr     = term->cursor.attr;
			cell->fg_color = term->cursor.fg_color;
			cell->bg_color = term->cursor.bg_color;
			cell++;
		}
	}
	if (term->ops && term->ops->clear) {
		term->ops->clear(term, rect);
		term_validate_rect(term, rect);
	} else {
		term_invalidate_rect(term, rect);
	}
}
