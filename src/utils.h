#pragma once

#include <stddef.h>
#include <stdint.h>

// loads the entire content of some file into a string which you can then delete
// at your leisure.
// Return NULL if the file doesn't exist or we get an OOM or a third plane hits
// the world trade center.
char *utils_slurp(char const *filepath);

// Like strcmp but it treats period characters as if it were another kind of
// null terminator.
int utils_pathcmp(char const *a, char const *b);

// Takes a path and removes the lowest level name from it by simply incrementing
// the pointer. If there are no further names in the path then the return value
// will point directly to the NULL terminator.
char const *utils_pathBody(char const *path);

// Takes two paths, with the first one expected to be a subset of the second.
// It returns a pointer to first name in full that doesn't exist in base. If the
// two paths conflict though it will instead return NULL.
char const *utils_stripBase(char const *base, char const *full);

// Checks that a path is formed correctly. In effect this means that it only
// contains periods seperated by ascii letters, or it is empty.
// Returns true if good.
bool utils_validatePath(char const *path);