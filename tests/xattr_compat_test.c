#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "../compat.h"

#ifdef _WIN32
// Setup mock environment for Windows
#include "vlc_common.h"
#include "vlc_fs.h"
#endif

// We want to test xattr_compat.h, but it includes system headers.
// On non-Windows/non-Mac, it includes sys/xattr.h.
// We can test the wrapper logic.

#include "../xattr_compat.h"

// Simple test suite
void test_set_get_xattr(void) {
    const char *test_file = "test_xattr.txt";
    const char *attr_name = "user.test";
    const char *attr_value = "Hello World";

    // Create dummy file
    FILE *f = fopen(test_file, "w");
    if (!f) {
        perror("fopen");
        exit(1);
    }
    fprintf(f, "Test data");
    fclose(f);

    // Set xattr
    int ret = sys_setxattr(test_file, attr_name, attr_value, strlen(attr_value), 0);
    if (ret != 0) {
        // If xattr is not supported (e.g. tmpfs or some CI envs), we might get ENOTSUP.
        // On Windows with our compat layer, it should work via ADS.
        if (errno == ENOTSUP || errno == EOPNOTSUPP) {
            printf("xattr not supported on this filesystem. Skipping.\n");
            remove(test_file);
            return;
        }
        perror("sys_setxattr");
        remove(test_file);
        // Fail only if we expected it to work?
        // For CI cross-platform, Linux CI usually runs on overlayfs/ext4 which supports xattr.
        // Windows/macOS should work.
        exit(1);
    }

    // Get xattr size
    ssize_t len = sys_getxattr(test_file, attr_name, NULL, 0);
    if (len < 0) {
        perror("sys_getxattr size");
        remove(test_file);
        exit(1);
    }

    if (len != (ssize_t)strlen(attr_value)) {
        fprintf(stderr, "Size mismatch: expected %zu, got %ld\n", strlen(attr_value), (long)len);
        remove(test_file);
        exit(1);
    }

    // Get xattr value
    char *buf = malloc(len + 1);
    ssize_t read_len = sys_getxattr(test_file, attr_name, buf, len);
    if (read_len != len) {
        perror("sys_getxattr value");
        free(buf);
        remove(test_file);
        exit(1);
    }
    buf[len] = '\0';

    if (strcmp(buf, attr_value) != 0) {
        fprintf(stderr, "Value mismatch: expected '%s', got '%s'\n", attr_value, buf);
        free(buf);
        remove(test_file);
        exit(1);
    }

    printf("xattr test passed: %s = %s\n", attr_name, buf);
    free(buf);
    remove(test_file);

    #ifdef _WIN32
    // On Windows, cleanup the ADS if possible?
    // Removing the main file removes ADS too.
    #endif
}

int main(void) {
    printf("Running xattr_compat tests...\n");
    test_set_get_xattr();
    return 0;
}
