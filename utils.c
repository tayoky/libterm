#include <libterm.h>
#include <term-utils.h>

// a bunch of utils functions

void term_scroll(term_t *term, int amount) {
	// TODO : do an actual scrolling
	term->wrap_pending = 0;
}

void term_set_cursor(term_t *term, int x, int y) {
	// redraw the old cell to hide the old cursor
	term_draw_cell(term, CELL_AT(term, term->cursor_x, term->cursor_y), term->cursor_x, term->cursor_y);

	term->wrap_pending = 0;
	term->cursor_x = x;
	term->cursor_y = y;
	term_clamp_cursor(term);
	
	// now redraw the cursor
	term_draw_cursor(term, term->cursor_x, term->cursor_y);
}

void term_move_cursor(term_t *term, int x, int y) {
	term_set_cursor(term, term->cursor_x + x, term->cursor_y + y);
}

void term_clamp_cursor(term_t *term) {
	if (term->cursor_x < 0) term->cursor_x = 0;
	if (term->cursor_x >= term->width) term->cursor_x = term->width - 1;
	if (term->cursor_y < 0) term->cursor_y = 0;
	if (term->cursor_y >= term->height) term->cursor_y = term->height - 1;
}

void term_newline(term_t *term) {
	int x = 0;
	int y = term->cursor_y;
	if (y >= term->height - 1) {
		term_scroll(term, 1);
	} else {
		y++;
	}
	term_set_cursor(term, x, y);
}

void term_carriage_return(term_t *term) {
	term_set_cursor(term, 0, term->cursor_y);
}

void term_tab(term_t *term) {
	term_move_cursor(term, 8 - (term->cursor_x % 8), 0);
}
