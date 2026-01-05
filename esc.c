#include <libterm.h>
#include <term-utils.h>

// esc + char codes handling

void term_esc_dispatch(term_t *term, wint_t final) {
	if (term->intermediate) return;
	switch (final) {
	case 'E': // NEL : next line
		term_newline(term);
		break;
	case 'c': // RIS : reset
		term_reset(term);
		break;
	case '7': //DECSC : save cursor
		term->saved_cursor = term->cursor;
		break;
	case '8': //DECRC : save cursor
		// force a cursor refresh
		term_set_cursor(term, term->saved_cursor.x, term->saved_cursor.y);
		term->cursor = term->saved_cursor;
		break;
	}
}
