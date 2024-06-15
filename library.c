#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_input.h>
#include <vlc_demux.h>
#include <vlc_meta.h>
#include <stdio.h>

#define UNUSED_VAR(x) (void)(x)

static int Open(vlc_object_t *);
static void Close(vlc_object_t *);
static void ReportMedia(vlc_object_t *p_this);

vlc_module_begin()
set_shortname("Report Playing")
set_description("Reports currently playing media to stdout")
set_capability("misc", 0)
set_callbacks(Open, Close)
vlc_module_end()

        static int Open(vlc_object_t *p_this)
{
    UNUSED_VAR(p_this);
    msg_Info(p_this, "Report Playing extension activated");

    vlc_object_t *p_input = vlc_object_find(NULL, VLC_OBJECT_INPUT, FIND_ANYWHERE);
    if (p_input != NULL) {
        ReportMedia(p_input);
        vlc_object_release(p_input);
    } else {
        msg_Err(p_this, "Failed to find input object");
        return VLC_EGENERIC;
    }

    return VLC_SUCCESS;
}

static void Close(vlc_object_t *p_this)
{
    UNUSED_VAR(p_this);
    msg_Info(p_this, "Report Playing extension deactivated");
}

static void ReportMedia(vlc_object_t *p_input)
{
    input_thread_t *p_input_thread = (input_thread_t *)p_input;

    if (p_input_thread == NULL) {
        msg_Err(p_input, "Invalid input object");
        return;
    }

    input_item_t *p_item = input_GetItem(p_input_thread);
    if (p_item) {
        char *psz_name = input_item_GetTitleFbName(p_item);
        if (psz_name) {
            msg_Info(p_input, "Now playing: %s", psz_name);
            printf("Now playing: %s\n", psz_name);
            free(psz_name);
        }

        char *psz_uri = input_item_GetURI(p_item);
        if (psz_uri) {
            // Convert VLC URI to a regular file path
            char file_path[1024];
            snprintf(file_path, sizeof(file_path), "%s", psz_uri + 7); // Remove "file://"

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

            msg_Info(p_input, "File path: %s", file_path);
            printf("File path: %s\n", file_path);

            // Add your custom logic to read and print extended attributes here

            free(psz_uri);
        }
    }
}
