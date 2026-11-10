#include "def.h"
#include "utils.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// The types of def that there are.
typedef enum {
    def_type_OBJ,
    def_type_LIST,
    def_type_TEXT
} def_type_e;

// Option bit flags that a def can have a combination of.
typedef enum: uint32_t {
    def_opt_SINGLE = 0b1,
    def_opt_ALPHANUM = 0b10
} def_opt_e;

// lookup entry to find a child struct. Manages the text memory itself so that
// needs to be a copy.
typedef struct {
    char *text;
    def_t *def;
} lookup_t;

// Manages a set of cursors for iterating over permutations.
// Paths are associated with each cursor in order to aid in the creation of
// child cursor sets.
// You will note that we have set a maximum of def_MAX_CURSORS on the number of
// cursors in a set. This is partly laziness but 8 cursors can easily produce
// trillions of unique permutations or more so I really don't think anybody
// would want more on purpose.
// TODO: should stored paths be references to strings or copies?
struct def_cursor_t_ {
    def_t *defs;
    size_t n;
    bool valid;
    struct {
        char const *path;
        def_t *start;
        def_t *end;
        def_t *current;
    } cursors[def_MAX_CURSORS];
};

// Compares a text string in a with the text of a def_lookup_t in b for the
// purposes of a binary search or some such thing.
static int lookup_find(void const *a, void const *b) {
    char const *name = a;
    lookup_t const *item = b;
    return utils_pathcmp(a, item->text);
}

static int lookup_comp(void const *a, void const *b) {
    lookup_t const *left = a;
    lookup_t const *right = b;
    return utils_pathcmp(left->text, right->text);
}

static def_t *lookup(lookup_t *table, size_t n, char const *name) {
    lookup_t *found = bsearch(
        name,
        table,
        n,
        sizeof(lookup_t),
        lookup_find
    );
    if (found == NULL) return NULL;
    return found->def;
}

// Main struct of a single definition. They exist in a tree structure.
// Children of a list node are contiguous in memory as are children of an object
// node but in the case of the object node there also exists an intermediate
// lookup table.
struct def_t_ {
    def_type_e type;
    union {
        struct {
            size_t n;
            lookup_t *refs;
        } obj;
        struct {
            size_t n;
            def_t *items;
        } list;
        char const *text;
    };
};

char const *def_get(def_t *d, char const *path) {
    switch (d->type) {
        case def_type_OBJ:
            def_t *child = lookup(d->obj.refs, d->obj.n, path);
            if (child == NULL) return NULL;
            return def_get(child, utils_pathBody(path));
        case def_type_LIST:
            return NULL;
        case def_type_TEXT:
            return (path[0] == 0) ? d->text : NULL;
    }
}

def_t *def_getNode(def_t *d, char const *path) {
    if (*path == 0) return d;
    if (d->type != def_type_OBJ) return NULL;
    def_t *child = lookup(d->obj.refs, d->obj.n, path);
    return def_getNode(d, utils_pathBody(path));
}

def_cursor_t *def_cursor_make(def_t *d, char const **paths, size_t nPaths) {
    if (nPaths > def_MAX_CURSORS) nPaths = def_MAX_CURSORS;
    def_cursor_t *cursor = malloc(sizeof(def_cursor_t));
    cursor->defs = d;
    cursor->valid = true;
    int slot = 0;
    for (int i = 0; i < nPaths; i++) {
        def_t *item = def_getNode(d, paths[i]);
        if (!item) {
            free(cursor);
            return NULL;
        }
        if (item->type != def_type_LIST) break;
        cursor->cursors[slot].path = paths[i];
        cursor->cursors[slot].start = item->list.items;
        cursor->cursors[slot].current = item->list.items;
        cursor->cursors[slot].end = item->list.items + item->list.n;
        slot++;
    }
    cursor->n = slot;
    return cursor;
}

def_cursor_t *def_cursor_makeChild(
    def_cursor_t *parent,
    char const **paths,
    size_t nPaths
) {
    if (nPaths > def_MAX_CURSORS) nPaths = def_MAX_CURSORS;
    def_cursor_t *child = malloc(sizeof(def_cursor_t));
    child->defs = parent->defs;
    child->valid = true;
    int slot = 0;
    for (int i = 0; i < nPaths; i++) {
        def_t *item = def_cursor_getNode(parent, paths[i]);
        if (!item) {
            free(child);
            return NULL;
        }
        if (item->type != def_type_LIST) break;
        child->cursors[slot].path = paths[i];
        child->cursors[slot].start = item->list.items;
        child->cursors[slot].current = item->list.items;
        child->cursors[slot].end = item->list.items + item->list.n;
        slot++;
    }
    child->n = slot;
    return child;
}

char const *def_cursor_get(def_cursor_t *c, char const *path) {
    assert(c->valid);
    for (int i = 0; i < c->n; i++) {
        char const *remainder = utils_stripBase(c->cursors[i].path, path);
        if (remainder == NULL) continue;
        return def_get(c->cursors[i].current, remainder);
    }
    return NULL;
}

def_t *def_cursor_getNode(def_cursor_t *c, char const *path) {
    assert(c->valid);
    for (int i = 0; i < c->n; i++) {
        char const *remainder = utils_stripBase(c->cursors[i].path, path);
        if (remainder == NULL) continue;
        return def_getNode(c->cursors[i].current, remainder);
    }
    return NULL;
}

void def_cursor_inc(def_cursor_t *c) {
    if (c->n == 0 || c->valid == false) return;
    for (int i = 0; i < c->n; i++) {
        c->cursors[i].current++;
        if (c->cursors[i].current < c->cursors[i].end) {
            break;
        } else {
            c->cursors[i].current = c->cursors[i].start;
            if (i == c->n - 1) {
                c->valid = false;
            }
        }
    }
}

bool def_cursor_hasMore(def_cursor_t const *c) {
    return c->valid;
}

void def_cursor_free(def_cursor_t *c) {
    // TODO: at present there is nothing else to do because string paths are
    // held by reference but if that changes then so will this.
    free(c);
}


// Right, so the cursor type is actually a list of cursors, where each one
// points to some list, and records the bounds etc. When we call inc on the
// cursor it increments them collectively until we've covered every combination
// of values for each cursor.

// To create a set of cursors from the top level, you need to provide a set of
// paths which resolve to lists and of course cannot cross a list.

// If you want to cross a list, you need to create a cursor for that list, and
// then we have a similar function that takes a set of paths again but it is
// passed to a cursor set rather than the top level def, and so we create a
// cursor set which iterates the permutations of the given lists for the current
// permutation of it's parent. Each cursor inside the set stores the path that
// it references so that it knows if it can help in the creation of a child
// cursor, basically it just gets the part of queried path that isn't shared and
// looks it up in the children of the list index it currently points to. If it
// is able to find it unambiguously then it can create a child cursor over that
// list, but if it's not a list or can't be found it bombs.

// Actually there are three outcomes when trying to create a specific child
// cursor:
// - 1 - child can be found unambiguously and is a list. Success.
// - 2 - child can be found but isn't a list. In this case there is no need to
//       iterate on it so we can just ignore the request.
// - 3 - child cannot be found unambiguously. Failure.

// Now, one issue is that if we remove redundant paths from the cursor set, it
// also stops potentially relevant children from being used in a child cursor
// set. In order to fix this we need to make sure that child cursor sets can
// also reference things that the parent set doesn't as long as they're
// unambiguous at top level.



// Ok. I think that this accurately describes how this data structure works.
// I don't really care about questions of "what if the scope ...." not my
// problem right now. We build this first.