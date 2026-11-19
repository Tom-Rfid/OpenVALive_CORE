/* The firmware's version as people read it: on the boot screen, in MAP CONTROL. */
#ifndef APP_VERSION
#define APP_VERSION "DEV"
#endif

/* "0.0.3" -> "V_0.0.03" (the patch on two digits); anything else as it is, after "V_" */
static void app_version_text(char *out, uint32_t size)
{
    const char *v = APP_VERSION;
    uint32_t num[3] = {0, 0, 0}, part = 0, n = 0, i;
    int plain = 1;                                     /* only digits and two dots */
    const char *s;
    for (s = v; *s && plain; s++) {
        if (*s == '.' && part < 2u)
            part++;
        else if (*s >= '0' && *s <= '9' && num[part] < 1000u)
            num[part] = num[part] * 10u + (uint32_t)(*s - '0');
        else
            plain = 0;
    }
    out[n++] = 'V';
    out[n++] = '_';
    if (!plain || part != 2u || s == v) {
        for (s = v; *s && n + 1u < size; s++)
            out[n++] = *s;
    } else {
        for (i = 0; i < 3u && n + 6u < size; i++) {
            if (i == 2u && num[i] < 10u)
                out[n++] = '0';
            if (num[i] >= 100u)
                out[n++] = (char)('0' + num[i] / 100u % 10u);
            if (num[i] >= 10u)
                out[n++] = (char)('0' + num[i] / 10u % 10u);
            out[n++] = (char)('0' + num[i] % 10u);
            if (i < 2u)
                out[n++] = '.';
        }
    }
    out[n] = 0;
}
