
#include <libterm.h>
#include <term-utils.h>

// esc + char codes handling

void term_esc_dispatch(term_t *term, wint_t final) {
	// TODO : support for more stuff
	switch (final) {
	case 'E': // NEL : next line
		term_newline(term);
		break;
	}
}
