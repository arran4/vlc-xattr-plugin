#ifndef COMPAT_H
#define COMPAT_H

#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
    #include <BaseTsd.h>
    typedef SSIZE_T ssize_t;

    // MSVC deprecates strdup
    #define strdup _strdup
    #define strcasecmp _stricmp
    #define strncasecmp _strnicmp

    // Implement strndup for Windows
    static inline char *strndup(const char *s, size_t n) {
        size_t len = strnlen(s, n);
        char *new_s = (char *)malloc(len + 1);
        if (new_s == NULL) return NULL;
        new_s[len] = '\0';
        return (char *)memcpy(new_s, s, len);
    }

    // Implement strtok_r for Windows
    static inline char *strtok_r(char *str, const char *delim, char **saveptr) {
        return strtok_s(str, delim, saveptr);
    }
#else
    // POSIX systems usually have these
    #include <unistd.h>
    #include <strings.h> // for strcasecmp
#endif

#endif // COMPAT_H
