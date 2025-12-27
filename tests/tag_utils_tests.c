#include "../tag_utils.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_decode_percent_sequence_valid(void)
{
    char decoded = '\0';
    assert(decode_percent_sequence("%20", &decoded));
    assert(decoded == ' ');

    assert(decode_percent_sequence("%2F", &decoded));
    assert(decoded == '/');
}

static void test_decode_percent_sequence_invalid(void)
{
    char decoded = 'x';
    assert(!decode_percent_sequence("20%", &decoded));
    assert(decoded == 'x');

    assert(!decode_percent_sequence("%2Z", &decoded));
    assert(decoded == 'x');

    assert(!decode_percent_sequence("%", &decoded));
    assert(decoded == 'x');
}

static void test_url_decode_inplace(void)
{
    char path[] = "file%3A%2F%2Ftmp%2Fvideo%20file.mp4";
    url_decode_inplace(path);
    assert(strcmp(path, "file://tmp/video file.mp4") == 0);

    char no_encoding[] = "plain_text";
    url_decode_inplace(no_encoding);
    assert(strcmp(no_encoding, "plain_text") == 0);
}

static void test_xdg_tags_append_if_missing(void)
{
    bool added = false;
    char *result = xdg_tags_append_if_missing("alpha,beta", "gamma", &added);
    assert(result != NULL);
    assert(strcmp(result, "alpha,beta,gamma") == 0);
    assert(added);
    free(result);

    added = false;
    result = xdg_tags_append_if_missing("alpha,beta", "beta", &added);
    assert(result != NULL);
    assert(strcmp(result, "alpha,beta") == 0);
    assert(!added);
    free(result);

    added = false;
    result = xdg_tags_append_if_missing(NULL, "first", &added);
    assert(result != NULL);
    assert(strcmp(result, "first") == 0);
    assert(added);
    free(result);
}

int main(void)
{
    test_decode_percent_sequence_valid();
    test_decode_percent_sequence_invalid();
    test_url_decode_inplace();
    test_xdg_tags_append_if_missing();

    printf("All tests passed\n");
    return 0;
}
