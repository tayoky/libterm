#include <libterm.h>
#include <term-utils.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// really simple terminal manager

static void handle_esc(term_t *term, wint_t c) {
	if (c >= ' ' && c <= '/') {
		term->intermediate = c;
		term->state = TERM_STATE_ESCAPE_INTERMEDIATE;
		return;
	}
	switch (c) {
	case '[':
		term->state = TERM_STATE_CSI_ENTRY;
		term->params_count = 0;
		term->intermediate = 0;
		term->params[0] = -1;
		break;
	default:
		term->intermediate = 0;
		term_esc_dispatch(term, c);
		term->state = TERM_STATE_GROUND;
		break;
	}
}

static void handle_esc_intermediate(term_t *term, wint_t c) {
	if (c >= ' ' && c <= '/') {
		term->intermediate = c;
	} else {
		term_esc_dispatch(term, c);
		term->state = TERM_STATE_GROUND;
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
	case TERM_STATE_ESCAPE_INTERMEDIATE:
		handle_esc_intermediate(term, c);
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

static int utf8_read(term_t *term, const unsigned char **buf, size_t *size) {
	if (term->utf8_offset < 1) {
		if (*size < 1) return -1;
		term->utf8_buf[0] = *((*buf)++);
		(*size)--;
		term->utf8_offset=1;
	}
	int len;
	if (term->utf8_buf[0] <= 0x7f) {
		len = 1;
	} else if ((term->utf8_buf[0] & 0xe0) == 0xc0) {
		len = 2;
	} else if ((term->utf8_buf[0] & 0xf0) == 0xe0) {
		len = 3;
	} else if ((term->utf8_buf[0] & 0xf8) == 0xf0) {
		len = 4;
	} else {
		goto invalid;
	}

	while (term->utf8_offset < len) {
		if (*size < 1) return -1;
		term->utf8_buf[term->utf8_offset] = *((*buf)++);
		(*size)--;
		term->utf8_offset++;
	}

	int c = 0;
	switch (len) {
	case 1:
		c = term->utf8_buf[0];
		break;
	case 2:
		c = term->utf8_buf[0] & 0x1f;
		break;
	case 3:
		c = term->utf8_buf[0] & 0x0f;
		break;
	case 4:
		c = term->utf8_buf[0] & 0x07;
		break;
	}

	for (int i=1; i<len; i++) {
		if ((term->utf8_buf[i] & 0xc0) != 0x80) {
			goto invalid;
		}
		c <<= 6;
		c |= term->utf8_buf[i] & 0x3f;
	}

	// bound checks
	switch (len) {
	case 2:
		if (c <= 0x7f) goto invalid;
		break;
	case 3:
		if (c <= 0x7ff) goto invalid;
		break;
	case 4:
		if (c <= 0xffff)  goto invalid;
		if (c > 0x10ffff) goto invalid;
		break;
	}

	term->utf8_offset = 0;
	return c;
invalid:
	term->utf8_offset = 0;
	return '?';
}

void term_output(term_t *term, const char *buf, size_t size) {
	int c;
	while ((c = utf8_read(term, (const unsigned char**) &buf, &size)) >= 0) {
		putchar(c);
		output_char(term, c);
	}
	term_render(term);
}

void term_render(term_t *term) {
	for (int y=0; y<term->height; y++) {
		dirty_row_t *row = &term->dirty_rows[y];
		if (row->end_x <= row->start_x) continue;
		cell_t *cell = CELL_AT(term, row->start_x, y);
		for (int x=row->start_x; x<row->end_x; x++) {
			term_draw_cell(term, cell, x, y);
			if (term->cursor.x == x && term->cursor.y == y && (term->dec_mode & TERM_DEC_CURSOR)) {
				term_draw_cursor(term, x, y);
			}
			cell++;
		}
		row->end_x = -1;
		row->start_x = INT_MAX;
	}
}

int term_init(term_t *term) {
	term->screen = malloc(sizeof(cell_t) * term->width * term->height);
	term->dirty_rows = malloc(sizeof(dirty_row_t) * term->height);
	for (int i=0; i<term->height; i++) {
		term->dirty_rows[i].end_x = -1;
		term->dirty_rows[i].start_x = INT_MAX;
	}
	term_reset(term);
	return 0;
}

void term_cleanup(term_t *term) {
	free(term->screen);
}
