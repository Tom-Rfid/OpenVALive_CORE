/*
 * OpenVALive desktop - reading single key presses from the terminal.
 *
 * Windows: the console's _kbhit / _getch.
 * Linux / macOS: the terminal in raw mode (no echo, no line buffering).
 * Arrow keys are translated to KEY_UP / KEY_DOWN / KEY_LEFT / KEY_RIGHT.
 */
#include "desktop.h"

#ifdef _WIN32
#include <conio.h>
#include <windows.h>

void keyboard_begin(void)
{
    /* French (and Belgian French) keyboards are AZERTY: switch the note keys */
    HKL layout = GetKeyboardLayout(0);
    if (PRIMARYLANGID(LOWORD((DWORD_PTR)layout)) == LANG_FRENCH) {
        app.azerty = 1;
    }
}
void keyboard_end(void) {}

int keyboard_poll(void)
{
    if (!_kbhit()) {
        return KEY_NONE;
    }
    int c = _getch();
    if (c == 0 || c == 0xE0) {             /* arrows come as two codes */
        switch (_getch()) {
        case 72: return KEY_UP;
        case 80: return KEY_DOWN;
        case 75: return KEY_LEFT;
        case 77: return KEY_RIGHT;
        default: return KEY_NONE;
        }
    }
    if (c == '\r') {
        return KEY_ENTER;
    }
    return c;
}

#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

static struct termios saved;
static int raw_mode;

void keyboard_begin(void)
{
    if (!isatty(STDIN_FILENO) || tcgetattr(STDIN_FILENO, &saved) != 0) {
        return;
    }
    struct termios raw = saved;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;                    /* non-blocking reads */
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    raw_mode = 1;
}

void keyboard_end(void)
{
    if (raw_mode) {
        tcsetattr(STDIN_FILENO, TCSANOW, &saved);
        raw_mode = 0;
    }
}

static int read_byte(void)
{
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        return c;
    }
    return -1;
}

int keyboard_poll(void)
{
    int c = read_byte();
    if (c < 0) {
        return KEY_NONE;
    }
    if (c == 27) {                         /* ESC alone, or the start of an arrow: ESC [ A */
        sleep_ms(2);
        int c2 = read_byte();
        if (c2 != '[') {
            return KEY_ESC;
        }
        switch (read_byte()) {
        case 'A': return KEY_UP;
        case 'B': return KEY_DOWN;
        case 'C': return KEY_RIGHT;
        case 'D': return KEY_LEFT;
        default:  return KEY_NONE;
        }
    }
    if (c == '\n') {
        return KEY_ENTER;
    }
    return c;
}
#endif
