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
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <errno.h>

#ifndef MODULE_STRING
#define MODULE_STRING "xattrplaying_plugin"
#endif

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define XATTR_SIZE 10000  // Maximum size of an extended attribute value

static int Open(vlc_object_t *);
static void Close(vlc_object_t *);
static int PlayingChange(vlc_object_t *p_this, const char *psz_var,
                         vlc_value_t oldval, vlc_value_t newval, void *p_data);
static int ItemChange(vlc_object_t *p_this, const char *psz_var,
                      vlc_value_t oldval, vlc_value_t newval, void *p_data);
static int hex_value(char c);
static bool decode_percent_sequence(const char *p_src, char *p_decoded);
static void url_decode_inplace(char *p_str);

struct current_item_t {
    // vlc_tick_t  i_start;            /**< playing start    */
};

struct intf_sys_t {
    struct current_item_t   p_current_item;     /**< song being played      */
    input_thread_t         *p_input;            /**< current input thread   */
    input_item_t *p_item;                       /**< Previous item */
};

vlc_module_begin()
    set_shortname("XAttrPlayInfo")
    set_description("XAttr Play Info writes play information to extended file system attributes")
    set_category(CAT_INTERFACE)
    set_subcategory(SUBCAT_INTERFACE_CONTROL)
    set_capability("interface", 1)
//    add_bool( "XAttrPlayInfo-append", 1, "XAttrPlayInfo-APPEND_TEXT2", "XAttrPlayInfo-APPEND_LONGTEXT2", false ) // TODO make the tag configurable (And based on %)
    set_callbacks(Open, Close)
vlc_module_end()

static int Open(vlc_object_t *p_this)
{
    intf_thread_t   *p_intf     = (intf_thread_t*) p_this;
    p_intf->p_sys = calloc(1, sizeof(intf_sys_t));
    printf("Report Playing extension activated\n");
    msg_Info(p_this, "Report Playing extension activated");

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
        vlc_object_release(p_sys->p_input);
        p_sys->p_input = NULL;
    }
    free(p_sys);
    p_intf->p_sys = NULL;
}

/*****************************************************************************
 * ItemChange: Playlist item change callback
 *****************************************************************************/
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

    // p_sys->p_current_item.i_start = mdate(); TODO - switch on multiple different tags based on progress.

    p_sys->p_input = vlc_object_hold(p_input);
    var_AddCallback(p_input, "intf-event", PlayingChange, p_intf);

    return VLC_SUCCESS;
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
        char *psz_name = input_item_GetTitleFbName(p_item);
        if (psz_name) {
            msg_Info(p_this, "Now playing: %s", psz_name);
            free(psz_name);
        }

        char *psz_uri = input_item_GetURI(p_item);
        if (psz_uri) {
            char *psz_path = NULL;
            const char *psz_scheme_end = strstr(psz_uri, "://");

            if (psz_scheme_end == NULL) {
                msg_Dbg(p_this, "Skipping URI without scheme: %s", psz_uri);
                free(psz_uri);
                return VLC_SUCCESS;
            }

            size_t scheme_len = psz_scheme_end - psz_uri;
            if (scheme_len != 4 || strncasecmp(psz_uri, "file", 4) != 0) {
                msg_Dbg(p_this, "Skipping non-file URI: %s", psz_uri);
                free(psz_uri);
                return VLC_SUCCESS;
            }

            const char *psz_path_start = psz_scheme_end + 3; // Skip "://"
            if (*psz_path_start == '\0') {
                msg_Dbg(p_this, "Skipping file URI without a path: %s", psz_uri);
                free(psz_uri);
                return VLC_SUCCESS;
            }

            psz_path = strdup(psz_path_start);
            if (psz_path == NULL) {
                msg_Err(p_this, "Failed to allocate memory for path");
                free(psz_uri);
                return VLC_SUCCESS;
            }

            if (psz_path[0] == '/' && isalpha((unsigned char)psz_path[1]) && psz_path[2] == ':') {
                memmove(psz_path, psz_path + 1, strlen(psz_path) + 1);
            }

            url_decode_inplace(psz_path);

            char list[XATTR_SIZE];
            char *list_dynamic = NULL;
            ssize_t list_len;

            // Get the list of extended attributes
            list_len = listxattr(psz_path, list, XATTR_SIZE);
            if (list_len == -1 && errno == ERANGE) {
                list_len = listxattr(psz_path, NULL, 0);
                if (list_len == -1) {
                    perror("listxattr");
                    exit(EXIT_FAILURE);
                }
                list_dynamic = malloc(list_len);
                if (list_dynamic == NULL) {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
                list_len = listxattr(psz_path, list_dynamic, list_len);
            }
            if (list_len == -1) {
                msg_Err(p_this, "Failed to list xattrs for %s: %s", psz_path, strerror(errno));
                free(psz_uri);
                free(psz_path);
                return VLC_SUCCESS;
            }

            char *list_buffer = list_dynamic ? list_dynamic : list;

            char *newTag = "seen";
            char *userXdgTags = strdup(newTag);

            // Print each attribute and its value
            bool found = false;
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
                if (strcasecmp("user.xdg.tags", attr) == 0) {
                    int c = 0;
                    for (int i = 0; i <= value_len; i++) {
                        switch (value_buffer[i]) {
                        case '\0': case ',':
                                if (strncmp(newTag, &value_buffer[c], i - c) == 0) {
                                    found = true;
                                }
                                c = i + 1;
                            break;
                        }
                        if (found) {
                            break;
                        }
                    }
                    free(userXdgTags);
                    userXdgTags = strndup(value_buffer, value_len);
                    if (!found) {
                        userXdgTags = realloc(userXdgTags, value_len + 2 + strlen(newTag));
                        userXdgTags = strcat(userXdgTags, ",");
                        userXdgTags = strcat(userXdgTags, newTag);
                    }
                }
                free(value_dynamic);
            }

            if (!found) {
                printf("Adding a extended attribute %s\n", newTag);
                int ret = setxattr(psz_path, "user.xdg.tags", userXdgTags, strlen(userXdgTags), 0);
                if (ret == -1) {
                    perror("setxattr");
                }
            }
            if (userXdgTags != NULL) {
                free(userXdgTags);
            }
            free(list_dynamic);
            free(psz_path);
            free(psz_uri);
        }
    }
    return VLC_SUCCESS;
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

static bool decode_percent_sequence(const char *p_src, char *p_decoded)
{
    if (p_src[0] != '%' || !isxdigit((unsigned char)p_src[1]) || !isxdigit((unsigned char)p_src[2]))
        return false;

    int hi = hex_value(p_src[1]);
    int lo = hex_value(p_src[2]);
    if (hi < 0 || lo < 0)
        return false;

    *p_decoded = (char)((hi << 4) | lo);
    return true;
}

static void url_decode_inplace(char *p_str)
{
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
