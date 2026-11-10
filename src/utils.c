#include "utils.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Tells you if the provided character is a letter.
static bool isLetter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

char *utils_slurp(char const *filepath) {
    FILE *f = fopen(filepath, "r");
    if (f == NULL) {
        printf("Couldn't open file %s for some reason", filepath);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buffer = malloc(size * sizeof(char));
    if (buffer == NULL) {
        printf("OOM trying to read in file %s", filepath);
        return NULL;
    }
    fread(buffer, sizeof(char), size, f);
    fclose(f);
    return buffer;
}

int utils_pathcmp(char const *a, char const *b) {
    while (*a == *b) {
        a++;
        b++;
        if (*a == 0 || *a == '.') return 0;
    }
    return (int)*a - (int)*b;
}

char const *utils_pathBody(char const *path) {
    while (*path != '.' && *path != 0) path++;
    if (*path == '.') path++;
    return path;
}

char const *utils_stripBase(char const *base, char const *full) {
    while (*base == *full && base != 0) {
        base++;
        full++;
    }
    if (*full != '.' && *full != 0) return NULL;
    if (*full == '.') full++;
    return full;
}

bool utils_validatePath(char const *path) {
    if (*path == 0) return true;
    bool allowPeriod = false;
    int i;
    for (i = 0; path[i]; i++) {
        if (path[i] == '.') {
            if (allowPeriod) allowPeriod = false;
            return false;
        } else if (!isLetter(path[i])) {
            return false;
        } else {
            allowPeriod = true;
        }
    }
    return path[i - 1] != '.';
}