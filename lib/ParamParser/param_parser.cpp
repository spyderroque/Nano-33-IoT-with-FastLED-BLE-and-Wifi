#include "param_parser.h"

#include <stdlib.h>
#include <string.h>

int parseParam(const char *req, const char *key) {
    if (req == NULL || key == NULL) {
        return -1;
    }

    const size_t keyLen = strlen(key);

    for (const char *p = strstr(req, key); p != NULL; p = strstr(p + 1, key)) {
        // The character immediately after the key must be '='.
        if (p[keyLen] != '=') {
            continue;
        }
        // The key must be delimited on its left by '?' or '&'. This is what
        // stops a short key (e.g. "r") from matching inside a longer one
        // (e.g. "br"), and stops any match inside the request path itself.
        if (p == req || (p[-1] != '?' && p[-1] != '&')) {
            continue;
        }

        const char *valStart = p + keyLen + 1;
        const char *valEnd = valStart;
        while (*valEnd != '\0' && *valEnd != '&' && *valEnd != ' ' && *valEnd != '\r') {
            valEnd++;
        }
        if (valEnd == valStart) {
            return -1;  // key present but value empty
        }
        return atoi(valStart);
    }

    return -1;
}
