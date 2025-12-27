#ifndef TAG_UTILS_H
#define TAG_UTILS_H

#include <stdbool.h>

typedef struct {
    char *name;
    int percent;
} xattr_target_t;

/**
 * Decode a percent-encoded triplet (e.g. "%20") into its byte value.
 *
 * \param p_src Pointer to the beginning of the percent sequence.
 * \param p_decoded Output for the decoded character when decoding succeeds.
 * \return true when the sequence is valid and was decoded, false otherwise.
 */
bool decode_percent_sequence(const char *p_src, char *p_decoded);

/**
 * Decode percent-encoded sequences inside the given string in place.
 * The function stops at the first null terminator and always leaves the
 * string null-terminated.
 */
void url_decode_inplace(char *p_str);

/**
 * Ensure that \p new_tag exists inside the comma-separated list in
 * \p existing_tags. The returned buffer must be freed by the caller.
 *
 * \param existing_tags Existing comma-separated tag list (may be NULL or empty).
 * \param new_tag Tag to append when missing.
 * \param out_added Optional output set to true when the tag was appended.
 * \return Newly allocated string containing the resulting tag list, or NULL on
 *         allocation failure.
 */
char *xdg_tags_append_if_missing(const char *existing_tags, const char *new_tag,
                                 bool *out_added);

/**
 * Parse a configuration string into a list of xattr_target_t.
 * Format: "name@percent,name2@percent2"
 * Example: "seen@90,started@0"
 *
 * \param config_str The configuration string.
 * \param count Output pointer for the number of targets found.
 * \return Array of xattr_target_t (caller must free using free_xattr_targets).
 */
xattr_target_t *parse_xattr_targets(const char *config_str, int *count);

/**
 * Free the array of targets returned by parse_xattr_targets.
 */
void free_xattr_targets(xattr_target_t *targets, int count);

/**
 * Trim whitespace from both ends of a string in place.
 */
char *trim_token(char *psz_token);

/**
 * Check if the given path matches any of the prefixes in the skip list.
 *
 * \param psz_path The path to check (e.g., "/mnt/data/video.mp4").
 * \param psz_skip_list Comma/semicolon/newline separated list of path prefixes.
 * \return true if a match is found, false otherwise.
 */
bool should_skip_path(const char *psz_path, const char *psz_skip_list);

#endif // TAG_UTILS_H
