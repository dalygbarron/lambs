#pragma once

// Stores all of the input user data and provides facilities for iterating
// over it without the use of array indices by iterating over permutations of a
// given set of lists within the data.
// A path is a string containing names seperated by period characters which
// allows one to refer to children of children. Where a list is referred to, we
// work with it one element at a time by calculating permutations of the set of
// defs we are interested in.
// A name is a set of ascii text characters with no symbols or whitespace which
// refers to a specific child element of an object definition.

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define def_MAX_CURSORS 8
#define def_MAX_TEXT_LENGTH 4096

// forward declarations.
typedef struct def_t_ def_t;
typedef struct def_cursor_t_ def_cursor_t;

// Loads in a tree of defs from a string of JSON data.
def_t *def_fromJSON(char const *json);

// Deletes the def and all it's children etc. Make sure you don't have any
// cursor sets or anything hanging around because the whole ship is going down.
void def_free(def_t *d);

// Tries to get a value for the given path if possible. If it is not possible
// then it returns NULL. For any def_t that this gets called on, it should
// always assume that the first name of the given path is a reference to a child
// element, and thus a text definition will only actually return it's value if
// the passed path points directly to a null terminator.
char const *def_get(def_t *d, char const *path);

// Same as def_get but it gets the node rather than the text, and can return obj
// and list nodes.
def_t *def_getNode(def_t *d, char const *path);

// Recursively print out a definition.
void def_print(def_t *d);

// Creates a cursor object which can be used to iterate over all permutations
// of the given paths. The idea is that we only reference things by name and
// never array index, and so if you want to iterate over some named list, you
// provide it's name and this allows you to iterate over all values. You can
// also iterate over multiple named lists at the same time, thus getting all
// possible combinations of entries.
// If you pass more than def_MAX_CURSORS paths the later ones will be ignored.
// Also this will return NULL if any of the paths couldn't be found.
def_cursor_t *def_cursor_make(def_t *d, char const **paths, size_t nPaths);

// Creates a set of child cursors based on the current state of the provided
// parent cursor set. All paths provided must be able to be resolved
// unambiguously in relation to the state of the parent cursor set or from the
// top level definitions tree.
// If you pass more than def_MAX_CURSORS paths the later ones will be ignored.
def_cursor_t *def_cursor_makeChild(
    def_cursor_t *parent,
    char const **paths,
    size_t nPaths
);

// Tries to get a textual value for the given path using the current permutation
// of the cursor. If it can't do that for some reason or other then NULL will be
// returned.
char const *def_cursor_get(def_cursor_t *c, char const *path);

// Tries to resolve the given path to a def node based on the current state of
// the top level def and the given cursor set.
// The cursor set lets us resolve values across lists which are normally
// ambiguous by focusing on one permutation at a time. However, the cursor set
// only has info for the lists we're actually permutating, so if the path
// queried isn't relevant to that then this function will also try to get it the
// normal way.
def_t *def_cursor_getNode(def_cursor_t *c, char const *path);

// Increments the cursor to be operating on the next permutation.
void def_cursor_inc(def_cursor_t *c);

// Tells you if the cursor currently points to a valid permutation or if it has
// gone over everything already.
bool def_cursor_hasMore(def_cursor_t const *c);

// Frees the memory in a cursor object.
void def_cursor_free(def_cursor_t *c);