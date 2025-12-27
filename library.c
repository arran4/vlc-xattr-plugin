#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
#include <vlc_input.h>
#include <vlc_meta.h>
#include <vlc_stream.h>
#include <vlc_threads.h>
#include <vlc_playlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>

#include "tag_utils.h"
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <errno.h>

#ifndef N_
#define N_(str) (str)
#endif

#ifndef MODULE_STRING
#define MODULE_STRING "xattrplaying_plugin"
#endif

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef N_
# define N_(str) (str)
#endif

#define XATTR_SIZE 10000  // Maximum size of an extended attribute value
#define DEFAULT_TAG_NAME "seen"

static int Open(vlc_object_t *);
static void Close(vlc_object_t *);
static int PlayingChange(vlc_object_t *p_this, const char *psz_var,
                         vlc_value_t oldval, vlc_value_t newval, void *p_data);
static int PositionChange(vlc_object_t *p_this, const char *psz_var,
                         vlc_value_t oldval, vlc_value_t newval, void *p_data);
static int ItemChange(vlc_object_t *p_this, const char *psz_var,
                      vlc_value_t oldval, vlc_value_t newval, void *p_data);

static const char *xattr_error_reason(int err)
{
    switch (err) {
        case EACCES:
        case EPERM:
            return "Insufficient permissions to write extended attributes";
        case EROFS:
            return "Cannot modify extended attributes on a read-only filesystem";
        case ENOSPC:
            return "No space left on device to store extended attributes";
        case EDQUOT:
            return "Quota exceeded while writing extended attributes";
#if defined(ENOTSUP) && (!defined(EOPNOTSUPP) || EOPNOTSUPP != ENOTSUP)
        case ENOTSUP:
            return "Filesystem does not support extended attributes";
#endif
#if defined(EOPNOTSUPP) && (!defined(ENOTSUP) || EOPNOTSUPP != ENOTSUP)
        case EOPNOTSUPP:
            return "Filesystem does not support extended attributes";
#endif
        default:
            return NULL;
    }
}
struct current_item_t {
    // vlc_tick_t  i_start;            /**< playing start    */
};

struct intf_sys_t {
    struct current_item_t   p_current_item;     /**< song being played      */
    input_thread_t         *p_input;            /**< current input thread   */
    input_item_t *p_item;                       /**< Previous item */
    bool b_tagging_enabled;                     /**< Whether to write xattrs */
    char *psz_xattr_key;                        /**< Xattr key to use */
    xattr_target_t *targets;                    /**< Configured targets */
    int i_target_count;                         /**< Number of targets */
    bool *b_target_applied;                     /**< Flags for applied targets for current item */
    char *psz_current_path;                     /**< Current file path being played */
    char *psz_skip_paths;                       /**< Comma/newline-separated path prefixes to skip */
};

vlc_module_begin()
    set_shortname("XAttrPlayInfo")
    set_description("XAttr Play Info writes play information to extended file system attributes")
    set_category(CAT_INTERFACE)
    set_subcategory(SUBCAT_INTERFACE_CONTROL)
    set_capability("interface", 1)
    /* Module options:
     *  - xattr-tagging-enabled: master switch to enable/disable xattr writes.
     *  - xattr-tag-name: tag to append to user.xdg.tags (default: "seen").
     *  - xattr-skip-paths: comma/newline separated absolute path prefixes to skip.
     */
    add_bool("xattr-tagging-enabled", true,
             N_("Enable tagging"),
             N_("Write the configured tag to user.xdg.tags when playback starts."),
             false)
    add_string("xattr-key", "user.xdg.tags",
               N_("XAttr Key"),
               N_("The extended attribute key to write tags to (default: user.xdg.tags)."),
               false)
    add_string("xattr-targets", "",
               N_("Targets"),
               N_("Comma-separated list of tags to apply at specific percentages (e.g., 'seen@90,started@0'). overrides tag-name if set."),
               false)
    add_string("xattr-tag-name", DEFAULT_TAG_NAME,
               N_("Tag name"),
               N_("Tag to append to the user.xdg.tags extended attribute."),
               false)
    add_string("xattr-skip-paths", "",
               N_("Skip paths"),
               N_("Comma- or newline-separated list of absolute path prefixes that should not be tagged."),
               false)
    set_callbacks(Open, Close)
vlc_module_end()


static int Open(vlc_object_t *p_this)
{
    intf_thread_t   *p_intf     = (intf_thread_t*) p_this;
    p_intf->p_sys = calloc(1, sizeof(intf_sys_t));
    printf("Report Playing extension activated\n");
    msg_Info(p_this, "Report Playing extension activated");

    if (p_intf->p_sys == NULL)
        return VLC_ENOMEM;

    p_intf->p_sys->b_tagging_enabled = var_InheritBool(p_intf, "xattr-tagging-enabled");
    p_intf->p_sys->psz_xattr_key = var_InheritString(p_intf, "xattr-key");
    p_intf->p_sys->psz_skip_paths = var_InheritString(p_intf, "xattr-skip-paths");

    char *psz_targets = var_InheritString(p_intf, "xattr-targets");
    if (psz_targets && *psz_targets) {
        p_intf->p_sys->targets = parse_xattr_targets(psz_targets, &p_intf->p_sys->i_target_count);
    } else {
        // Fallback to xattr-tag-name with 0%
        char *psz_tag_name = var_InheritString(p_intf, "xattr-tag-name");
        if (psz_tag_name && *psz_tag_name) {
             p_intf->p_sys->targets = calloc(1, sizeof(xattr_target_t));
             p_intf->p_sys->targets[0].name = psz_tag_name; // Ownership transferred
             p_intf->p_sys->targets[0].percent = 0;
             p_intf->p_sys->i_target_count = 1;
        } else {
            free(psz_tag_name);
        }
    }
    free(psz_targets);

    if (p_intf->p_sys->i_target_count > 0) {
        p_intf->p_sys->b_target_applied = calloc(p_intf->p_sys->i_target_count, sizeof(bool));
    }

    var_AddCallback(pl_Get(p_intf), "input-current", ItemChange, p_intf);

    return VLC_SUCCESS;
}

static void Close(vlc_object_t *p_this)
{
    intf_thread_t               *p_intf = (intf_thread_t*) p_this;
    intf_sys_t                  *p_sys  = p_intf->p_sys;
    msg_Info(p_this, "Report Playing extension deactivated");
    var_DelCallback(pl_Get(p_intf), "input-current", ItemChange, p_intf);
    if (p_sys->p_input != NULL)
    {
        var_DelCallback(p_sys->p_input, "intf-event", PlayingChange, p_intf);
        var_DelCallback(p_sys->p_input, "position", PositionChange, p_intf);
        vlc_object_release(p_sys->p_input);
        p_sys->p_input = NULL;
    }
    free_xattr_targets(p_sys->targets, p_sys->i_target_count);
    free(p_sys->b_target_applied);
    free(p_sys->psz_xattr_key);
    free(p_sys->psz_skip_paths);
    free(p_sys->psz_current_path);
    free(p_sys);
    p_intf->p_sys = NULL;
}

static void WriteTag(vlc_object_t *p_this, const char *psz_path, const char *newTag, const char *psz_xattr_key);

/*****************************************************************************
 * ItemChange: Playlist item change callback
 *****************************************************************************/
static int PositionChange(vlc_object_t *p_this, const char *psz_var,
                         vlc_value_t oldval, vlc_value_t newval, void *p_data);

static int ItemChange(vlc_object_t *p_this, const char *psz_var,
                      vlc_value_t oldval, vlc_value_t newval, void *p_data)
{
    intf_thread_t  *p_intf  = p_data;
    intf_sys_t     *p_sys   = p_intf->p_sys;
    input_thread_t *p_input = newval.p_address;

    VLC_UNUSED(psz_var);
    VLC_UNUSED(oldval);

    if (p_sys->p_input != NULL)
    {
        var_DelCallback(p_sys->p_input, "intf-event", PlayingChange, p_intf);
        var_DelCallback(p_sys->p_input, "position", PositionChange, p_intf);
        vlc_object_release(p_sys->p_input);
        p_sys->p_item = NULL;
        p_sys->p_input = NULL;
    }

    if (p_input == NULL)
    {
        p_sys->p_item = NULL;
        return VLC_SUCCESS;
    }

    input_item_t *p_item = input_GetItem(p_input);
    if (p_item == NULL)
        return VLC_SUCCESS;

    if (var_CountChoices(p_input, "video-es"))
    {
        msg_Dbg(p_this, "Not an audio-only input, not submitting");
        return VLC_SUCCESS;
    }

    p_sys->p_input = vlc_object_hold(p_input);
    var_AddCallback(p_input, "intf-event", PlayingChange, p_intf);
    var_AddCallback(p_input, "position", PositionChange, p_intf);

    return VLC_SUCCESS;
}

static int PositionChange(vlc_object_t *p_this, const char *psz_var,
                         vlc_value_t oldval, vlc_value_t newval, void *p_data)
{
    input_thread_t *p_input_thread = (input_thread_t *)p_this;
    intf_thread_t  *p_intf  = p_data;
    intf_sys_t     *p_sys   = p_intf->p_sys;

    VLC_UNUSED(p_input_thread);
    VLC_UNUSED(psz_var);
    VLC_UNUSED(oldval);

    if (!p_sys->b_tagging_enabled || p_sys->i_target_count == 0)
        return VLC_SUCCESS;

    if (p_sys->psz_current_path == NULL)
        return VLC_SUCCESS;

    if (should_skip_path(p_sys->psz_current_path, p_sys->psz_skip_paths))
         return VLC_SUCCESS;

    float position = newval.f_float;
    int percent = (int)(position * 100);

    for (int i = 0; i < p_sys->i_target_count; i++) {
        if (!p_sys->b_target_applied[i] && percent >= p_sys->targets[i].percent) {
             WriteTag((vlc_object_t*)p_intf, p_sys->psz_current_path, p_sys->targets[i].name, p_sys->psz_xattr_key);
             p_sys->b_target_applied[i] = true;
        }
    }

    return VLC_SUCCESS;
}

static void WriteTag(vlc_object_t *p_this, const char *psz_path, const char *newTag, const char *psz_xattr_key)
{
    char list[XATTR_SIZE];
    char *list_dynamic = NULL;
    ssize_t list_len;

    // Get the list of extended attributes
    list_len = listxattr(psz_path, list, XATTR_SIZE);
    if (list_len == -1 && errno == ERANGE) {
        list_len = listxattr(psz_path, NULL, 0);
        if (list_len == -1) {
            perror("listxattr");
            return;
        }
        list_dynamic = malloc(list_len);
        if (list_dynamic == NULL) {
            perror("malloc");
            return;
        }
        list_len = listxattr(psz_path, list_dynamic, list_len);
    }
    if (list_len == -1) {
        msg_Err(p_this, "Failed to list xattrs for %s: %s", psz_path, strerror(errno));
        return;
    }

    char *list_buffer = list_dynamic ? list_dynamic : list;
    char *userXdgTags = strdup(newTag);

    // Print each attribute and its value
    bool xattr_key_found = false;
    bool tag_added = false;
    for (char *attr = list_buffer; attr < list_buffer + list_len; attr += strlen(attr) + 1) {
        char value[XATTR_SIZE];
        char *value_dynamic = NULL;
        ssize_t value_len = getxattr(psz_path, attr, value, XATTR_SIZE);
        if (value_len == -1 && errno == ERANGE) {
            value_len = getxattr(psz_path, attr, NULL, 0);
            if (value_len == -1) {
                perror("getxattr");
                continue;
            }
            value_dynamic = malloc(value_len);
            if (value_dynamic == NULL) {
                perror("malloc");
                continue;
            }
            value_len = getxattr(psz_path, attr, value_dynamic, value_len);
        }
        if (value_len == -1) {
            perror("getxattr");
            free(value_dynamic);
            continue;
        }
        char *value_buffer = value_dynamic ? value_dynamic : value;
        if (strcasecmp(psz_xattr_key, attr) == 0) {
            xattr_key_found = true;
            free(userXdgTags);
            char *value_copy = strndup(value_buffer, value_len);
            userXdgTags = xdg_tags_append_if_missing(value_copy, newTag, &tag_added);
            free(value_copy);
        }
        free(value_dynamic);
    }

    if ((!xattr_key_found || tag_added) && userXdgTags != NULL) {
        printf("Adding a extended attribute %s to key %s\n", newTag, psz_xattr_key);

        size_t tag_len = strlen(userXdgTags);
        size_t required_size = tag_len + 1; // Ensure space for null terminator
        char *resized_tags = realloc(userXdgTags, required_size);
        if (resized_tags == NULL) {
            msg_Err(p_this, "Failed to resize buffer for %s on %s", psz_xattr_key, psz_path);
        } else {
            userXdgTags = resized_tags;
            userXdgTags[tag_len] = '\0';

            int ret = setxattr(psz_path, psz_xattr_key, userXdgTags, required_size, 0);
            if (ret == -1) {
                int err = errno;
                const char *psz_reason = xattr_error_reason(err);
                if (psz_reason != NULL) {
                    msg_Err(p_this, "Failed to set xattr %s on %s: %s (%s)",
                            psz_xattr_key, psz_path, strerror(err), psz_reason);
                } else {
                    msg_Err(p_this, "Failed to set xattr %s on %s: %s", psz_xattr_key, psz_path,
                            strerror(err));
                }
            }
        }
    }
    if (userXdgTags != NULL) {
        free(userXdgTags);
    }
    free(list_dynamic);
}

static int PlayingChange(vlc_object_t *p_this, const char *psz_var,
                         vlc_value_t oldval, vlc_value_t newval, void *p_data)
{
    input_thread_t *p_input_thread = (input_thread_t *)p_this;
    intf_thread_t  *p_intf  = p_data;
    intf_sys_t     *p_sys   = p_intf->p_sys;
    input_item_t *p_item = input_GetItem(p_input_thread);

    if (p_item && p_item != p_sys->p_item) {
        p_sys->p_item = p_item;

        // Reset applied flags
        if (p_sys->b_target_applied) {
            memset(p_sys->b_target_applied, 0, sizeof(bool) * p_sys->i_target_count);
        }

        // Resolve path once
        free(p_sys->psz_current_path);
        p_sys->psz_current_path = NULL;

        char *psz_uri = input_item_GetURI(p_item);
        if (psz_uri) {
            const char *psz_scheme_end = strstr(psz_uri, "://");
            if (psz_scheme_end != NULL) {
                size_t scheme_len = psz_scheme_end - psz_uri;
                if (scheme_len == 4 && strncasecmp(psz_uri, "file", 4) == 0) {
                     const char *psz_path_start = psz_scheme_end + 3; // Skip "://"
                     if (*psz_path_start != '\0') {
                         p_sys->psz_current_path = strdup(psz_path_start);
                         if (p_sys->psz_current_path) {
                            if (p_sys->psz_current_path[0] == '/' && isalpha((unsigned char)p_sys->psz_current_path[1]) && p_sys->psz_current_path[2] == ':') {
                                memmove(p_sys->psz_current_path, p_sys->psz_current_path + 1, strlen(p_sys->psz_current_path) + 1);
                            }
                            url_decode_inplace(p_sys->psz_current_path);
                         }
                     }
                }
            }
            free(psz_uri);
        }

        char *psz_name = input_item_GetTitleFbName(p_item);
        if (psz_name) {
            msg_Info(p_this, "Now playing: %s", psz_name);
            free(psz_name);
        }
    }
    return VLC_SUCCESS;
}
