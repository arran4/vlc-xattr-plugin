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

static void test_parse_xattr_targets(void)
{
    int count = 0;
    xattr_target_t *targets;

    // Test 1: Simple case
    targets = parse_xattr_targets("seen@90", &count);
    assert(count == 1);
    assert(strcmp(targets[0].name, "seen") == 0);
    assert(targets[0].percent == 90);
    free_xattr_targets(targets, count);

    // Test 2: Multiple tags
    targets = parse_xattr_targets("seen@90,started@0", &count);
    assert(count == 2);
    assert(strcmp(targets[0].name, "seen") == 0);
    assert(targets[0].percent == 90);
    assert(strcmp(targets[1].name, "started") == 0);
    assert(targets[1].percent == 0);
    free_xattr_targets(targets, count);

    // Test 3: Whitespace handling
    targets = parse_xattr_targets(" seen @ 50 ,  watched ", &count);
    assert(count == 2);
    assert(strcmp(targets[0].name, "seen") == 0);
    assert(targets[0].percent == 50);
    assert(strcmp(targets[1].name, "watched") == 0);
    assert(targets[1].percent == 0); // Default
    free_xattr_targets(targets, count);

    // Test 4: Invalid percentage
    targets = parse_xattr_targets("seen@200,bad@-1", &count);
    assert(count == 2);
    assert(targets[0].percent == 100); // Clamped
    assert(targets[1].percent == 0);   // Clamped
    free_xattr_targets(targets, count);
}

int main(void)
{
    test_decode_percent_sequence_valid();
    test_decode_percent_sequence_invalid();
    test_url_decode_inplace();
    test_xdg_tags_append_if_missing();
    test_parse_xattr_targets();

    printf("All tests passed\n");
    return 0;
}
