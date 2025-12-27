#ifndef XATTR_COMPAT_H
#define XATTR_COMPAT_H

#include <stddef.h>
#include <sys/types.h>

/*
 * Cross-platform xattr wrappers.
 *
 * Supported platforms:
 * - Linux: Uses sys/xattr.h (getxattr, setxattr)
 * - macOS: Uses sys/xattr.h (getxattr, setxattr with extra args)
 * - Windows: Uses NTFS Alternate Data Streams (ADS)
 */

#if defined(__linux__)
    #include <sys/xattr.h>

    static inline ssize_t sys_getxattr(const char *path, const char *name, void *value, size_t size) {
        return getxattr(path, name, value, size);
    }

    static inline int sys_setxattr(const char *path, const char *name, const void *value, size_t size, int flags) {
        return setxattr(path, name, value, size, flags);
    }

#elif defined(__APPLE__)
    #include <sys/xattr.h>

    static inline ssize_t sys_getxattr(const char *path, const char *name, void *value, size_t size) {
        // macOS getxattr takes position and options. 0, 0 is standard.
        return getxattr(path, name, value, size, 0, 0);
    }

    static inline int sys_setxattr(const char *path, const char *name, const void *value, size_t size, int flags) {
        // macOS setxattr takes position and options.
        return setxattr(path, name, value, size, 0, 0);
    }

#elif defined(_WIN32)
    #include <stdio.h>
    #include <errno.h>
    #include <vlc_common.h>
    #include <vlc_fs.h>

    static inline ssize_t sys_getxattr(const char *path, const char *name, void *value, size_t size) {
        if (!path || !name) {
            errno = EINVAL;
            return -1;
        }

        int len = snprintf(NULL, 0, "%s:%s", path, name);
        if (len < 0) {
            errno = EINVAL;
            return -1;
        }
        char *ads_path = malloc(len + 1);
        if (!ads_path) {
            errno = ENOMEM;
            return -1;
        }
        snprintf(ads_path, len + 1, "%s:%s", path, name);

        FILE *f = vlc_fopen(ads_path, "rb");
        free(ads_path);

        if (!f) {
            // Map ENOENT to ENODATA/ENOATTR if possible, but ENOENT is standard for file not found
            // Linux getxattr returns ENODATA (or ENOATTR on some systems) if attribute doesn't exist.
            // But ENOENT is acceptable if the file itself doesn't exist.
            // If the ADS doesn't exist, Windows returns generic file not found.
            // We'll leave errno as is (likely ENOENT).
            return -1;
        }

        fseek(f, 0, SEEK_END);
        long file_size = ftell(f);
        if (file_size < 0) {
            int err = errno;
            fclose(f);
            errno = err;
            return -1;
        }

        if (size == 0) {
            fclose(f);
            return (ssize_t)file_size;
        }

        if ((size_t)file_size > size) {
            fclose(f);
            errno = ERANGE;
            return -1;
        }

        fseek(f, 0, SEEK_SET);
        size_t read_bytes = fread(value, 1, file_size, f);
        int err = errno;
        fclose(f);

        if (read_bytes != (size_t)file_size && !feof(f)) {
             // Should not happen unless error
             errno = err;
             return -1;
        }

        return (ssize_t)read_bytes;
    }

    static inline int sys_setxattr(const char *path, const char *name, const void *value, size_t size, int flags) {
         if (!path || !name) {
            errno = EINVAL;
            return -1;
        }

        int len = snprintf(NULL, 0, "%s:%s", path, name);
        if (len < 0) {
            errno = EINVAL;
            return -1;
        }
        char *ads_path = malloc(len + 1);
        if (!ads_path) {
            errno = ENOMEM;
            return -1;
        }
        snprintf(ads_path, len + 1, "%s:%s", path, name);

        // flags: XATTR_CREATE, XATTR_REPLACE could be supported by checking existence first.
        // For simplicity, we ignore flags for now or just do overwrite "wb".
        // To support XATTR_CREATE (fail if exists) / XATTR_REPLACE (fail if not exists),
        // we would need to try opening "rb" first.

#if defined(XATTR_CREATE)
        if (flags & XATTR_CREATE) {
            FILE *check = vlc_fopen(ads_path, "rb");
            if (check) {
                fclose(check);
                free(ads_path);
                errno = EEXIST;
                return -1;
            }
        }
#endif
#if defined(XATTR_REPLACE)
        if (flags & XATTR_REPLACE) {
             FILE *check = vlc_fopen(ads_path, "rb");
            if (!check) {
                free(ads_path);
                errno = ENODATA; // or ENOATTR
                return -1;
            }
            fclose(check);
        }
#endif

        FILE *f = vlc_fopen(ads_path, "wb");
        free(ads_path);

        if (!f) {
            return -1;
        }

        size_t written = fwrite(value, 1, size, f);
        int err = errno;
        fclose(f);

        if (written != size) {
            errno = err;
            return -1;
        }

        return 0;
    }

#else
    // Fallback for other systems: stub
    #include <errno.h>
    static inline ssize_t sys_getxattr(const char *path, const char *name, void *value, size_t size) {
        errno = ENOTSUP;
        return -1;
    }
    static inline int sys_setxattr(const char *path, const char *name, const void *value, size_t size, int flags) {
        errno = ENOTSUP;
        return -1;
    }
#endif

#endif // XATTR_COMPAT_H
