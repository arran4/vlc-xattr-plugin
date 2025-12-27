#include "tag_utils.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static int hex_value(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    return -1;
}

bool decode_percent_sequence(const char *p_src, char *p_decoded)
{
    if (p_src == NULL || p_decoded == NULL)
        return false;

    if (p_src[0] != '%' || !isxdigit((unsigned char)p_src[1]) || !isxdigit((unsigned char)p_src[2]))
        return false;

    int hi = hex_value(p_src[1]);
    int lo = hex_value(p_src[2]);
    if (hi < 0 || lo < 0)
        return false;

    *p_decoded = (char)((hi << 4) | lo);
    return true;
}

void url_decode_inplace(char *p_str)
{
    if (p_str == NULL)
        return;

    char *p_read = p_str;
    char *p_write = p_str;

    while (*p_read != '\0') {
        if (*p_read == '%' && p_read[1] != '\0' && p_read[2] != '\0') {
            char decoded;
            if (decode_percent_sequence(p_read, &decoded)) {
                *p_write++ = decoded;
                p_read += 3;
                continue;
            }
        }
        *p_write++ = *p_read++;
    }

    *p_write = '\0';
}

char *xdg_tags_append_if_missing(const char *existing_tags, const char *new_tag,
                                 bool *out_added)
{
    if (out_added)
        *out_added = false;

    if (new_tag == NULL || *new_tag == '\0')
        return NULL;

    if (existing_tags == NULL || *existing_tags == '\0') {
        char *result = strdup(new_tag);
        if (result && out_added)
            *out_added = true;
        return result;
    }

    const size_t existing_len = strlen(existing_tags);
    const size_t new_len = strlen(new_tag);

    const char *cursor = existing_tags;
    while (*cursor != '\0') {
        const char *next_delim = strchr(cursor, ',');
        size_t token_len = next_delim ? (size_t)(next_delim - cursor) : strlen(cursor);
        if (token_len == new_len && strncmp(cursor, new_tag, new_len) == 0) {
            char *result = strndup(existing_tags, existing_len);
            return result;
        }
        if (!next_delim)
            break;
        cursor = next_delim + 1;
    }

    size_t combined_len = existing_len + 1 /* comma */ + new_len + 1 /* NUL */;
    char *result = malloc(combined_len);
    if (result == NULL)
        return NULL;

    memcpy(result, existing_tags, existing_len);
    result[existing_len] = ',';
    memcpy(result + existing_len + 1, new_tag, new_len);
    result[combined_len - 1] = '\0';

    if (out_added)
        *out_added = true;

    return result;
}
