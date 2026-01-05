#include <libterm.h>
#include <term-utils.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// really simple terminal manager

static void handle_esc(term_t *term, wint_t c) {
	switch (c) {
	case '[':
		term->state = TERM_STATE_CSI_ENTRY;
		term->params_count = 0;
		term->intermediate = 0;
		term->params[0] = -1;
		break;
	default:
		term_esc_dispatch(term, c);
		term->state = TERM_STATE_GROUND;
		break;
	}
}

static void handle_csi_seq(term_t *term, wint_t c) {
	if (term->params[term->params_count] != -1) term->params_count++;

	term_csi_dispatch(term, c);

	term->state = TERM_STATE_GROUND;
}

static void handle_csi_param(term_t *term, wint_t c) {
	if ((c >= '<' && c <= '?') || c == ',') {
		term->state = TERM_STATE_CSI_IGNORE;
	} else if (c >= '0' && c <= '9') {
		if (term->params[term->params_count] == -1) {
			term->params[term->params_count] = c - '0';
		} else {
			term->params[term->params_count] *= 10;
			term->params[term->params_count] += c - '0';
		}
	} else if (c == ';') {
		term->params_count++;
		term->params[term->params_count] = -1;
	} else if (c >= '@' && c <= '~') {
		handle_csi_seq(term, c);
	}
	//TODO : support to switch to CSI_INTERMIDATE
}

static void handle_csi_entry(term_t *term, wint_t c) {
	if (c == ',') {
		term->state = TERM_STATE_CSI_IGNORE;
	} else if (c >= '<' && c <= '?') {
		term->state = TERM_STATE_CSI_PARAM;
		term->intermediate = c;
	} else if ((c >= '0' && c <= '9') || c == ';') {
		term->state = TERM_STATE_CSI_PARAM;
		handle_csi_param(term, c);
	} else if (c >= '@' && c <= '~') {
		handle_csi_seq(term, c);
	}
}

static void handle_c0(term_t *term, wint_t c) {
	switch (c) {
	case '\b': // backspace
		if (term->cursor.wrap_pending) {
			term->cursor.wrap_pending = 0;
			break;
		}
		if (term->cursor.x > 0) term_move_cursor(term, -1, 0);
		break;
	case '\r':
		term_carriage_return(term);
		break;
	case '\n':
	case '\v':
	case '\f':
		term_newline(term);
		break;
	case '\t':
		term_tab(term);
		break;
	case 0x18: // cancel
	case 0x1a: // substitute
		term->state = TERM_STATE_GROUND;
		break;
	case '\033': // escape char
		term->state = TERM_STATE_ESCAPE;
		return;
	}
}

void term_print(term_t *term, wint_t c) {
	if (term->cursor.wrap_pending && (term->dec_mode & TERM_DEC_AUTOWRAP)) {
		term_newline(term);
	}
	cell_t *cell = CELL_AT(term, term->cursor.x, term->cursor.y);
	cell->c        = c;
	cell->attr     = term->cursor.attr;
	cell->fg_color = term->cursor.fg_color;
	cell->bg_color = term->cursor.bg_color;
	term_invalidate_cell(term, term->cursor.x, term->cursor.y);
	if (term->cursor.x >= term->width - 1) {
		term->cursor.wrap_pending = 1;
	} else {
		term_move_cursor(term, 1, 0);
	}
}

static void output_char(term_t *term, wint_t c) {
	if ((c >= 0x00 && c <= 0x1f) || c == 0x7f) {
		handle_c0(term, c);
		return;
	}

	switch (term->state) {
	case TERM_STATE_GROUND:
		term_print(term, c);
		break;
	case TERM_STATE_ESCAPE:
		handle_esc(term, c);
		break;
	case TERM_STATE_CSI_ENTRY:
		handle_csi_entry(term, c);
		break;
	case TERM_STATE_CSI_PARAM:
		handle_csi_param(term, c);
		break;
	case TERM_STATE_CSI_IGNORE:
		if (c >= '@' && c <= 0x7e) {
			term->state = TERM_STATE_GROUND;
		}
		break;
	}
}

void term_output_char(term_t *term, wint_t c) {
	output_char(term, c);
	term_render(term);
}

void term_output(term_t *term, const char *buf, size_t size) {
	// TODO : utf8 support
	while (size > 0) {
		output_char(term, *buf);
		buf++;
		size--;
	}
	term_render(term);
}


void term_render(term_t *term) {
	for (int y=0; y<term->height; y++) {
		dirty_row_t *row = &term->dirty_rows[y];
		if (row->end_x < row->start_x) continue;
		row->end_x = -1;
		row->start_x = INT_MAX;
		cell_t *cell = CELL_AT(term, row->start_x, y);
		for (int x=row->start_x; x<row->end_x; x++) {
			term_draw_cell(term, cell, x, y);
			if (term->cursor.x == x && term->cursor.y == y && (term->dec_mode & TERM_DEC_CURSOR)) {
				term_draw_cursor(term, x, y);
			}
			cell++;
		}
	}
}

int term_init(term_t *term) {
	term->screen = malloc(sizeof(cell_t) * term->width * term->height);
	term->dirty_rows = malloc(sizeof(dirty_row_t) * term->height);
	term_reset(term);
	return 0;
}

void term_cleanup(term_t *term) {
	free(term->screen);
}
