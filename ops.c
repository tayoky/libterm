#include <libterm.h>

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

void term_clear(term_t *term, int x, int y, int width, int height) {
	if (term->ops && term->ops->clear) {
		term->ops->clear(term, x, y, width, height);
	} else {
		for (int i=0; i<width; i++) {
			for (int j=0; j<height; j++) {
				// TODO
			}
		}
	}
}
