#pragma once

#include <stddef.h>
#include <stdint.h>

// Like strcmp but it treats period characters as if it were another kind of
// null terminator.
int utils_pathcmp(char const *a, char const *b) {
    while (*a == *b++) {
        a++;
        if (*a == 0 || *a == '.') return 0;
    }
    return (int)*a - ((int)*b - 1);
}

// Takes a path and removes the lowest level name from it by simply incrementing
// the pointer. If there are no further names in the path then the return value
// will point directly to the NULL terminator.
char const *utils_pathBody(char const *path) {
    while (*path != '.' && *path != 0) path++;
    if (*path == '.') path++;
    return path;
}

// Takes two paths, with the first one expected to be a subset of the second.
// It returns a pointer to first name in full that doesn't exist in base. If the
// two paths conflict though it will instead return NULL.
char const *utils_stripBase(char const *base, char const *full) {
    while (*base == *full && base != 0) {
        base++;
        full++;
    }
    if (*full != '.' && *full != 0) return NULL;
    if (*full == '.') full++;
    return full;
}