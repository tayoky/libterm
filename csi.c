#include <libterm.h>
#include <term-utils.h>
#include <string.h>

// csi codes handling

#define GET_PARAM(index, val) term->params[index] <= 0 ? val : term->params[index]

static void handle_dec(term_t *term, wint_t final) {
	int mode = 0;
	switch (term->params[0]) {
	case 7:
		mode = TERM_DEC_AUTOWRAP;
		break;
	case 25:
		mode = TERM_DEC_CURSOR;
		break;
	case 47:
	case 1047:
		mode = TERM_DEC_ALTBUFFER;
		break;
	default:
		return;
	}
	switch (final) {
	case 'h': // DECSET : set dec mode
		term->dec_mode |= mode;
		break;
	case 's': // DECRST : reset dec mode
		term->dec_mode &= ~mode;
		break;
	}

	// mode specific stuff
	switch (mode) {
	case TERM_DEC_CURSOR:
		// force cursor redraw
		term_move_cursor(term, 0, 0);
		break;
	}
}

static void handle_cursor_move(term_t *term, wint_t final) {
	int n = GET_PARAM(0, 1);
	switch (final) {
	case 'A':
		term_move_cursor(term, 0, -n);
		break;
	case 'B':
		term_move_cursor(term, 0, n);
		break;
	case 'C':
		term_move_cursor(term, n, 0);
		break;
	case 'D':
		term_move_cursor(term, -n, 0);
		break;
	case 'E':
		term_carriage_return(term);
		term_move_cursor(term, 0, n);
		break;
	case 'F':
		term_carriage_return(term);
		term_move_cursor(term, 0, -n);
		break;
	case 'G':
		term_set_cursor(term, n - 1, term->cursor.y);
		break;
	case 'H':
	case 'f':
		term_set_cursor(term, GET_PARAM(0, 1) - 1, GET_PARAM(1, 1) - 1);
		break;
	case 'I':
		term_move_cursor(term, (n - 1) * 8, 0);
		term_tab(term);
		break;
	}
}

static void handle_erase_line(term_t *term, int param) {
	term_rect_t clear_rect = {
		.y = term->cursor.y,
		.height = 1,
	};
	switch (param) {
	case 0: // from cursor to end of line
		clear_rect.x     = term->cursor.x;
		clear_rect.width = term->width - term->cursor.x;
		break;
	case 1: // start of line to cursor
		clear_rect.x     = 0;
		clear_rect.width = term->cursor.x + 1;
		break;
	case 2: // whole line
		clear_rect.x     = 0;
		clear_rect.width = term->width;
		break;
	}
	term_clear(term, &clear_rect);
}

static void handle_erase_screen(term_t *term, int param) {
	term_rect_t clear_rect = {
		.x = 0,
		.width = term->width,
	};
	switch (param) {
	case 0:
		clear_rect.y      = term->cursor.y + 1;
		clear_rect.height = term->height - term->cursor.y - 1;
		handle_erase_line(term, 0);
		break;
	case 1:
		clear_rect.y      = 0;
		clear_rect.height = term->cursor.y;
		handle_erase_line(term, 1);
		break;
	case 2:
	case 3:
		clear_rect.y      = 0;
		clear_rect.height = term->height;
		break;
	}
	term_clear(term, &clear_rect);
}

static int handle_long_color(int *params, term_color_t *color) {
	switch (params[0]) {
	case 2:
		// TODO : rgb color
		return 4;
	case 5:
		color->type = TERM_COLOR_ANSI;
		color->index = params[1];
		return 2;
	default:
		return 0;
	}
}

static void handle_sgr(term_t *term) {
	for (int i=0; i<term->params_count; i++) {
		switch (term->params[i]) {
		case -1:
		case 0:
			// reset
			term->cursor.attr = 0;
			memset(&term->cursor.fg_color, 0, sizeof(term_color_t));
			memset(&term->cursor.bg_color, 0, sizeof(term_color_t));
			break;
		case 1:
			term->cursor.attr |= TERM_ATTR_BOLD;
			break;
		case 4:
			term->cursor.attr |= TERM_ATTR_UNDERLINE;
			break;
		case 5:
			term->cursor.attr |= TERM_ATTR_BLINK;
			break;
		case 7:
			term->cursor.attr |= TERM_ATTR_INVERSE;
			break;
		case 22:
			term->cursor.attr &= ~TERM_ATTR_BOLD;
			break;
		case 24:
			term->cursor.attr &= ~TERM_ATTR_UNDERLINE;
			break;
		case 25:
			term->cursor.attr &= ~TERM_ATTR_BLINK;
			break;
		case 27:
			term->cursor.attr &= ~TERM_ATTR_INVERSE;
			break;
		case 30:
		case 31:
		case 32:
		case 33:
		case 34:
		case 35:
		case 36:
		case 37:
			term->cursor.fg_color.type  = TERM_COLOR_ANSI;
			term->cursor.fg_color.index = term->params[i] - 30;
			break;
		case 38:
			if (term->params_count < i + 3) break;
			i += handle_long_color(&term->params[i+1], &term->cursor.fg_color);
			break;
		case 39:
			term->cursor.fg_color.type = TERM_COLOR_DEFAULT;
			break;
		case 40:
		case 41:
		case 42:
		case 43:
		case 44:
		case 45:
		case 46:
		case 47:
			term->cursor.bg_color.type  = TERM_COLOR_ANSI;
			term->cursor.bg_color.index = term->params[i] - 40;
			break;
		case 48:
			if (term->params_count < i + 3) break;
			i += handle_long_color(&term->params[i+1], &term->cursor.bg_color);
			break;
		case 49:
			term->cursor.bg_color.type = TERM_COLOR_DEFAULT;
			break;
		case 90:
		case 91:
		case 92:
		case 93:
		case 94:
		case 95:
		case 96:
		case 97:
			term->cursor.fg_color.type  = TERM_COLOR_ANSI;
			term->cursor.fg_color.index = term->params[i] - 90 + 8;
			break;
		case 100:
		case 101:
		case 102:
		case 103:
		case 104:
		case 105:
		case 106:
		case 107:
			term->cursor.bg_color.type  = TERM_COLOR_ANSI;
			term->cursor.bg_color.index = term->params[i] - 100 + 8;
			break;
		}
	}
}

void term_csi_dispatch(term_t *term, wint_t final) 
{
	if (term->intermediate == '?') {
		handle_dec(term, final);
		return;
	}

	switch (final) {
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'f':
	case 'I':
		handle_cursor_move(term, final);
		break;
	case 'J':
		handle_erase_screen(term, GET_PARAM(0, 0));
		break;
	case 'K':
		handle_erase_line(term, GET_PARAM(0, 0));
		break;
	case 'm':
		handle_sgr(term);
		break;
	case 's':
		term->dumb_saved_cursor = term->cursor;
		break;
	case 'u':
		term_set_cursor(term, term->dumb_saved_cursor.x, term->dumb_saved_cursor.y);
		break;
	}
}
