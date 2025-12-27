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

    // Test: Empty string
    char empty[] = "";
    url_decode_inplace(empty);
    assert(strcmp(empty, "") == 0);

    // Test: Partial encoding at end
    char partial[] = "test%";
    url_decode_inplace(partial);
    assert(strcmp(partial, "test%") == 0);

    char partial2[] = "test%2";
    url_decode_inplace(partial2);
    assert(strcmp(partial2, "test%2") == 0);

    // Test: NULL
    url_decode_inplace(NULL); // Should not crash
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

    // Test: Null new_tag
    result = xdg_tags_append_if_missing("existing", NULL, &added);
    assert(result == NULL);
    assert(!added);

    // Test: Substring match but not full token match
    // "tag" is a substring of "tags", but should be added as a new tag
    added = false;
    result = xdg_tags_append_if_missing("tags", "tag", &added);
    assert(result != NULL);
    assert(strcmp(result, "tags,tag") == 0);
    assert(added);
    free(result);

    // "tags" contains "tag" but not as a token
    added = false;
    result = xdg_tags_append_if_missing("mytag,other", "tag", &added);
    assert(result != NULL);
    assert(strcmp(result, "mytag,other,tag") == 0);
    assert(added);
    free(result);

    // Exact match in list
    added = false;
    result = xdg_tags_append_if_missing("alpha,tag,beta", "tag", &added);
    assert(result != NULL);
    assert(strcmp(result, "alpha,tag,beta") == 0);
    assert(!added);
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

    // Test 5: Empty/Null
    targets = parse_xattr_targets(NULL, &count);
    assert(count == 0);
    assert(targets == NULL);

    targets = parse_xattr_targets("", &count);
    assert(count == 0);
    assert(targets == NULL);
}

static void test_trim_token(void)
{
    char t1[] = "  hello  ";
    assert(strcmp(trim_token(t1), "hello") == 0);

    char t2[] = "world";
    assert(strcmp(trim_token(t2), "world") == 0);

    char t3[] = "   ";
    assert(strcmp(trim_token(t3), "") == 0);

    char t4[] = "";
    assert(strcmp(trim_token(t4), "") == 0);

    char t5[] = "\t \n mixed \t ";
    assert(strcmp(trim_token(t5), "mixed") == 0);
}

static void test_should_skip_path(void)
{
    const char *skip_list = "/mnt/skip,/tmp/ignore;/var/cache\n/home/user/private";

    // Exact matches
    assert(should_skip_path("/mnt/skip", skip_list));
    assert(should_skip_path("/tmp/ignore", skip_list));
    assert(should_skip_path("/var/cache", skip_list));
    assert(should_skip_path("/home/user/private", skip_list));

    // Prefix matches
    assert(should_skip_path("/mnt/skip/file.mp4", skip_list));
    assert(should_skip_path("/tmp/ignore_this", skip_list)); // Note: this matches because strncmp matches prefix even if it's not a directory boundary. Limitation of current implementation?

    // No match
    assert(!should_skip_path("/mnt/other", skip_list));
    assert(!should_skip_path("/usr/local", skip_list));
    assert(!should_skip_path("/home/user/public", skip_list));

    // Edge cases
    assert(!should_skip_path(NULL, skip_list));
    assert(!should_skip_path("/path", NULL));
    assert(!should_skip_path("/path", ""));
}

int main(void)
{
    test_decode_percent_sequence_valid();
    test_decode_percent_sequence_invalid();
    test_url_decode_inplace();
    test_xdg_tags_append_if_missing();
    test_parse_xattr_targets();
    test_trim_token();
    test_should_skip_path();

    printf("All tests passed\n");
    return 0;
}
