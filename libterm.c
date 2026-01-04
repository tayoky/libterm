#include <libterm.h>
#include <term-utils.h>
#include <stdlib.h>
#include <string.h>

// really simple terminal manager

static void handle_esc(term_t *term, wint_t c) {
	switch (c) {
	case '[':
		term->state = TERM_STATE_CSI_ENTRY;
		term->params_count = 0;
		term->intermediate = 0;
		term->params[0] = -1;
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
		if (term->wrap_pending) {
			term->wrap_pending = 0;
			break;
		}
		if (term->cursor_x > 0) term_move_cursor(term, -1, 0);
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

void term_output_char(term_t *term, wint_t c) {
	if ((c >= 0x00 && c <= 0x1f) || c == 0x7f) {
		handle_c0(term, c);
		return;
	}

	switch (term->state) {
	case TERM_STATE_GROUND:
		if (term->wrap_pending) {
			term_newline(term);
		}
		cell_t *cell = CELL_AT(term, term->cursor_x, term->cursor_y);
		cell->c        = c;
		cell->attr     = term->attr;
		cell->fg_color = term->fg_color;
		cell->bg_color = term->bg_color;
		term_draw_cell(term, cell, term->cursor_x, term->cursor_y);
		if (term->cursor_x >= term->width - 1) {
			term->wrap_pending = 1;
		} else {
			term_move_cursor(term, 1, 0);
		}
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

void term_output(term_t *term, const char *buf, size_t size) {
	// TODO : utf8 support
	while (size > 0) {
		term_output_char(term, *buf);
		buf++;
		size--;
	}
}

int term_init(term_t *term) {
	memset(term, 0, sizeof(term_t));
	term->screen = malloc(sizeof(cell_t) * term->width * term->height);
	return 0;
}

void term_cleanup(term_t *term) {
	free(term->screen);
}
