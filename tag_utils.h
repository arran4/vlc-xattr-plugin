#ifndef TAG_UTILS_H
#define TAG_UTILS_H

#include <stdbool.h>

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

#endif // TAG_UTILS_H
