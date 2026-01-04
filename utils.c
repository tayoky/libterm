#include <libterm.h>
#include <term-utils.h>

// a bunch of utils functions

void term_scroll(term_t *term, int amount) {
	// TODO : support negative scrolling
	if (amount <= 0) return;
	term->cursor.wrap_pending = 0;
	term_rect_t src = {
		.x = 0,
		.y = amount,
		.width  = term->width,
		.height = term->height - amount,
	};
	term_rect_t dest = {
		.x = 0,
		.y = 0,
		.width  = term->width,
		.height = term->height - amount,
	};
	term_move(term, &dest, &src);

	term_rect_t clear_rect = {
		.x = 0,
		.y = term->height - amount,
		.width  = term->width,
		.height = amount,
	};
	term_clear(term, &clear_rect);
}

void term_set_cursor(term_t *term, int x, int y) {
	// redraw the old cell to hide the old cursor
	term_draw_cell(term, CELL_AT(term, term->cursor.x, term->cursor.y), term->cursor.x, term->cursor.y);

	term->cursor.wrap_pending = 0;
	term->cursor.x = x;
	term->cursor.y = y;
	term_clamp_cursor(term);
	
	// now redraw the cursor
	term_draw_cursor(term, term->cursor.x, term->cursor.y);
}

void term_move_cursor(term_t *term, int x, int y) {
	term_set_cursor(term, term->cursor.x + x, term->cursor.y + y);
}

void term_clamp_cursor(term_t *term) {
	if (term->cursor.x < 0) term->cursor.x = 0;
	if (term->cursor.x >= term->width) term->cursor.x = term->width - 1;
	if (term->cursor.y < 0) term->cursor.y = 0;
	if (term->cursor.y >= term->height) term->cursor.y = term->height - 1;
}

void term_newline(term_t *term) {
	int x = 0;
	int y = term->cursor.y;
	if (y >= term->height - 1) {
		term_scroll(term, 1);
	} else {
		y++;
	}
	term_set_cursor(term, x, y);
}

void term_carriage_return(term_t *term) {
	term_set_cursor(term, 0, term->cursor.y);
}

void term_tab(term_t *term) {
	term_move_cursor(term, 8 - (term->cursor.x % 8), 0);
}

void term_reset(term_t *term) {
	term->state = TERM_STATE_GROUND;
	term->cursor.x = 0;
	term->cursor.y = 0;
	term->cursor.attr = 0;
	term->cursor.fg_color.type = TERM_COLOR_DEFAULT;
	term->cursor.bg_color.type = TERM_COLOR_DEFAULT;
	term->saved_cursor = term->cursor;
	term->dumb_saved_cursor = term->cursor;
	term_rect_t clear_rect = {
		.x = 0,
		.y = 0,
		.width  = term->width,
		.height = term->height,
	};
	term_clear(term, &clear_rect);
}
