#ifndef FILE_H_INCLUDED
#define FILE_H_INCLUDED

/* ==========================================
 * File I/O Utilities
 * 
 * Cross-platform file operations.
 * Integrates with standard C library, providing:
 * - Convenience wrappers for common operations
 * - Platform-specific optimizations where useful
 * - Dynamic library loading
 * 
 * Note: For basic file I/O, prefer standard fopen/fread/fwrite.
 * These utilities are for convenience and platform abstraction.
 * 
 * Requires: macros.h
 * ========================================== */

#include "../macros.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ==========================================
 * PLATFORM INCLUDES
 * ========================================== */

#if defined(PLATFORM_WINDOWS)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#elif defined(POSIX)
    #include <sys/stat.h>
    #include <unistd.h>
    #include <errno.h>
    #include <dlfcn.h>
#endif

/* ==========================================
 * FILE READING (convenience wrappers)
 * ========================================== */

/* Read entire file into memory (caller must free) */
static force_inline void* file_read_all(const char* path, size_t* out_size) {
    if (out_size) *out_size = 0;
    
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    
    /* Get size */
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size <= 0) {
        fclose(f);
        return NULL;
    }
    
    /* Allocate and read */
    void* data = malloc((size_t)size);
    if (!data) {
        fclose(f);
        return NULL;
    }
    
    size_t bytes_read = fread(data, 1, (size_t)size, f);
    fclose(f);
    
    if (out_size) *out_size = bytes_read;
    return data;
}

/* Read file as null-terminated string (caller must free) */
static force_inline char* file_read_text(const char* path, size_t* out_size) {
    size_t size = 0;
    void* data = file_read_all(path, &size);
    if (!data) return NULL;
    
    char* text = (char*)realloc(data, size + 1);
    if (!text) {
        free(data);
        return NULL;
    }
    text[size] = '\0';
    
    if (out_size) *out_size = size;
    return text;
}

/* ==========================================
 * FILE WRITING
 * ========================================== */

/* Write buffer to file (overwrites) */
static force_inline int file_write_all(const char* path, const void* data, size_t size) {
    FILE* f = fopen(path, "wb");
    if (!f) return -1;
    
    size_t written = fwrite(data, 1, size, f);
    fclose(f);
    
    return (written == size) ? 0 : -1;
}

/* Append buffer to file */
static force_inline int file_append(const char* path, const void* data, size_t size) {
    FILE* f = fopen(path, "ab");
    if (!f) return -1;
    
    size_t written = fwrite(data, 1, size, f);
    fclose(f);
    
    return (written == size) ? 0 : -1;
}

/* ==========================================
 * FILE INFO
 * ========================================== */

/* Check if file/path exists */
static force_inline int file_exists(const char* path) {
#if defined(PLATFORM_WINDOWS)
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES);
#elif defined(POSIX)
    struct stat st;
    return stat(path, &st) == 0;
#else
    FILE* f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
#endif
}

/* Check if path is a directory */
static force_inline int file_is_dir(const char* path) {
#if defined(PLATFORM_WINDOWS)
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
#elif defined(POSIX)
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
#else
    UNUSED(path);
    return 0;
#endif
}

/* Get file size (-1 on error) */
static force_inline int64_t file_size(const char* path) {
#if defined(PLATFORM_WINDOWS)
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &fad)) return -1;
    LARGE_INTEGER size;
    size.HighPart = (LONG)fad.nFileSizeHigh;
    size.LowPart = fad.nFileSizeLow;
    return size.QuadPart;
#elif defined(POSIX)
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return (int64_t)st.st_size;
#else
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);
    return (int64_t)size;
#endif
}

/* Delete file */
static force_inline int file_delete(const char* path) {
#if defined(PLATFORM_WINDOWS)
    return DeleteFileA(path) ? 0 : -1;
#elif defined(POSIX)
    return unlink(path);
#else
    return remove(path);
#endif
}

/* ==========================================
 * DIRECTORY OPERATIONS
 * ========================================== */

/* Create directory (and parents if needed) */
static force_inline int dir_create(const char* path) {
#if defined(PLATFORM_WINDOWS)
    char tmp[MAX_PATH];
    strncpy(tmp, path, MAX_PATH - 1);
    tmp[MAX_PATH - 1] = '\0';
    
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';
            CreateDirectoryA(tmp, NULL);
            *p = '\\';
        }
    }
    return (CreateDirectoryA(path, NULL) || GetLastError() == ERROR_ALREADY_EXISTS) ? 0 : -1;
#elif defined(POSIX)
    char tmp[4096];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    return (mkdir(path, 0755) == 0 || errno == EEXIST) ? 0 : -1;
#else
    UNUSED(path);
    return -1;  /* Not supported */
#endif
}

/* ==========================================
 * PATH UTILITIES
 * ========================================== */

/* Get file extension (returns pointer into path, or empty string) */
static force_inline const char* path_ext(const char* path) {
    const char* dot = strrchr(path, '.');
    const char* sep = strrchr(path, '/');
#if defined(PLATFORM_WINDOWS)
    const char* bsep = strrchr(path, '\\');
    if (bsep && (!sep || bsep > sep)) sep = bsep;
#endif
    if (dot && (!sep || dot > sep)) {
        return dot + 1;
    }
    return "";
}

/* Get filename from path (returns pointer into path) */
static force_inline const char* path_filename(const char* path) {
    const char* sep = strrchr(path, '/');
#if defined(PLATFORM_WINDOWS)
    const char* bsep = strrchr(path, '\\');
    if (bsep && (!sep || bsep > sep)) sep = bsep;
#endif
    return sep ? sep + 1 : path;
}

/* Get directory from path (writes to out_dir, max len bytes) */
static force_inline int path_dirname(const char* path, char* out_dir, size_t len) {
    const char* filename = path_filename(path);
    size_t dir_len = (size_t)(filename - path);
    
    if (dir_len == 0) {
        if (len < 2) return -1;
        out_dir[0] = '.';
        out_dir[1] = '\0';
        return 0;
    }
    
    if (dir_len >= len) return -1;
    
    memcpy(out_dir, path, dir_len);
    /* Remove trailing separator */
    while (dir_len > 1 && (out_dir[dir_len - 1] == '/' || out_dir[dir_len - 1] == '\\')) {
        dir_len--;
    }
    out_dir[dir_len] = '\0';
    return 0;
}

/* Join path components (writes to out_path, max len bytes) */
static force_inline int path_join(const char* dir, const char* file, char* out_path, size_t len) {
    size_t dir_len = strlen(dir);
    size_t file_len = strlen(file);
    
    if (dir_len + 1 + file_len >= len) return -1;
    
    memcpy(out_path, dir, dir_len);
    
    /* Add separator if needed */
    if (dir_len > 0 && dir[dir_len - 1] != '/' && dir[dir_len - 1] != '\\') {
#if defined(PLATFORM_WINDOWS)
        out_path[dir_len++] = '\\';
#else
        out_path[dir_len++] = '/';
#endif
    }
    
    memcpy(out_path + dir_len, file, file_len);
    out_path[dir_len + file_len] = '\0';
    return 0;
}

/* ==========================================
 * DYNAMIC LIBRARY LOADING
 * ========================================== */

typedef void* lib_handle;

static force_inline lib_handle lib_open(const char* path) {
#if defined(PLATFORM_WINDOWS)
    return (lib_handle)LoadLibraryA(path);
#elif defined(POSIX)
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#else
    UNUSED(path);
    return NULL;
#endif
}

static force_inline void lib_close(lib_handle lib) {
    if (!lib) return;
#if defined(PLATFORM_WINDOWS)
    FreeLibrary((HMODULE)lib);
#elif defined(POSIX)
    dlclose(lib);
#endif
}

static force_inline void* lib_symbol(lib_handle lib, const char* name) {
    if (!lib) return NULL;
#if defined(PLATFORM_WINDOWS)
    return (void*)GetProcAddress((HMODULE)lib, name);
#elif defined(POSIX)
    return dlsym(lib, name);
#else
    UNUSED(name);
    return NULL;
#endif
}

#endif /* FILE_H_INCLUDED */
