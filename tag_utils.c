#include "tag_utils.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *trim_token(char *psz_token)
{
    while (*psz_token && isspace((unsigned char)*psz_token))
        psz_token++;

    char *psz_end = psz_token + strlen(psz_token);
    while (psz_end > psz_token && isspace((unsigned char)*(psz_end - 1)))
        *(--psz_end) = '\0';

    return psz_token;
}

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

xattr_target_t *parse_xattr_targets(const char *config_str, int *count)
{
    *count = 0;
    if (config_str == NULL || *config_str == '\0')
        return NULL;

    char *str_copy = strdup(config_str);
    if (!str_copy) return NULL;

    // First pass: count items
    int capacity = 0;
    char *saveptr;
    char *tmp = strdup(config_str);
    char *tok = strtok_r(tmp, ",", &saveptr);
    while (tok) {
        capacity++;
        tok = strtok_r(NULL, ",", &saveptr);
    }
    free(tmp);

    if (capacity == 0) {
        free(str_copy);
        return NULL;
    }

    xattr_target_t *targets = calloc(capacity, sizeof(xattr_target_t));
    if (!targets) {
        free(str_copy);
        return NULL;
    }

    tok = strtok_r(str_copy, ",", &saveptr);
    while (tok) {
        tok = trim_token(tok);
        if (*tok) {
            char *at_sign = strchr(tok, '@');
            int percent = 0;
            char *name = NULL;

            if (at_sign) {
                *at_sign = '\0';
                name = strdup(tok);
                percent = atoi(at_sign + 1);
                if (percent < 0) percent = 0;
                if (percent > 100) percent = 100;
            } else {
                name = strdup(tok);
                percent = 0; // Default to 0 if no percentage specified
            }

            const char *trimmed_name = trim_token(name); // trim name again just in case

            if (trimmed_name && *trimmed_name) {
                targets[*count].name = strdup(trimmed_name);
                targets[*count].percent = percent;
                (*count)++;
            }
            free(name);
        }
        tok = strtok_r(NULL, ",", &saveptr);
    }

    free(str_copy);
    return targets;
}

void free_xattr_targets(xattr_target_t *targets, int count)
{
    if (!targets) return;
    for (int i = 0; i < count; i++) {
        free(targets[i].name);
    }
    free(targets);
}

bool should_skip_path(const char *psz_path, const char *psz_skip_list)
{
    if (psz_skip_list == NULL || *psz_skip_list == '\0')
        return false;

    if (psz_path == NULL)
        return false;

    char *psz_copy = strdup(psz_skip_list);
    if (psz_copy == NULL)
        return false;

    bool b_match = false;
    char *saveptr = NULL;
    for (char *psz_token = strtok_r(psz_copy, ",;\n", &saveptr);
         psz_token != NULL;
         psz_token = strtok_r(NULL, ",;\n", &saveptr))
    {
        psz_token = trim_token(psz_token);
        if (*psz_token == '\0')
            continue;

        size_t len = strlen(psz_token);
        if (strncmp(psz_path, psz_token, len) == 0) {
            b_match = true;
            break;
        }
    }

    free(psz_copy);
    return b_match;
}
