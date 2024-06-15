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
#include <sys/types.h>
#include <sys/xattr.h>

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

struct current_item_t {
    vlc_tick_t  i_start;            /**< playing start    */
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
//        var_DelCallback(p_sys->p_input, "intf-event", PlayingChange, p_intf);
        vlc_object_release(p_sys->p_input);
    }
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
        p_sys->p_input = NULL;
    }

    if (p_input == NULL)
        return VLC_SUCCESS;

    input_item_t *p_item = input_GetItem(p_input);
    if (p_item == NULL)
        return VLC_SUCCESS;

    if (var_CountChoices(p_input, "video-es"))
    {
        msg_Dbg(p_this, "Not an audio-only input, not submitting");
        return VLC_SUCCESS;
    }

    p_sys->p_current_item.i_start = mdate();

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
            // Convert VLC URI to a regular file path
            char file_path[1024];
            snprintf(file_path, sizeof(file_path), "%s", psz_uri + 7); // Remove "file://"
            free(psz_uri);

            // Decode URL-encoded characters (if any)
            // Simple decoding assuming no complex cases
            for (char *p = file_path; *p; p++) {
                if (*p == '%') {
                    int c;
                    sscanf(p + 1, "%2x", &c);
                    *p = (char)c;
                    memmove(p + 1, p + 3, strlen(p + 3) + 1);
                }
            }

            char list[XATTR_SIZE];
            ssize_t list_len;

            // Get the list of extended attributes
            list_len = listxattr(file_path, list, XATTR_SIZE);
            if (list_len == -1) {
                perror("listxattr");
                exit(EXIT_FAILURE);
            }

            char *newTag = "seen";
            char *userXdgTags = strdup(newTag);

            // Print each attribute and its value
            int found = false;
            for (char *attr = list; attr < list + list_len; attr += strlen(attr) + 1) {
                char value[XATTR_SIZE];
                ssize_t value_len = getxattr(file_path, attr, value, XATTR_SIZE);
                if (value_len == -1) {
                    perror("getxattr");
                    continue;
                }
                if (strcasecmp("user.xdg.tags", attr) == 0) {
                    int c = 0;
                    for (int i = 0; i <= value_len; i++) {
                        switch (value[i]) {
                        case '\0': case ',':
                                if (strncmp(newTag, &value[c], i - c) == 0) {
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
                    userXdgTags = strndup(value, value_len);
                    if (!found) {
                        userXdgTags = realloc(userXdgTags, value_len + 2 + strlen(newTag));
                        userXdgTags = strcat(userXdgTags, ",");
                        userXdgTags = strcat(userXdgTags, newTag);
                    }
                }
            }

            if (!found) {
                printf("Adding a extended attribute %s\n", newTag);
                int ret = setxattr(file_path, "user.xdg.tags", userXdgTags, strlen(userXdgTags), 0);
                if (ret == -1) {
                    perror("setxattr");
                }
            }
            if (userXdgTags != NULL) {
                free(userXdgTags);
            }
        }
    }
    return VLC_SUCCESS;
}
