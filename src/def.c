#include "def.h"
#include "utils.h"
#include "cJSON.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// The types of def that there are.
typedef enum {
    def_type_OBJ = 0,
    def_type_LIST = 1,
    def_type_TEXT = 2
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

// Compares a text string in a with the text of a def_lookup_t in b for the
// purposes of a binary search or some such thing.
static int lookup_find(void const *a, void const *b) {
    char const *name = a;
    lookup_t const *item = b;
    return utils_pathcmp(a, item->text);
}

// Compares two lookup structs together by their base paths.
static int lookup_comp(void const *a, void const *b) {
    lookup_t const *left = a;
    lookup_t const *right = b;
    return utils_pathcmp(left->text, right->text);
}

// Looks for a def inside a lookup table with size n by looking for the given
// base path.
static def_t *lookup(lookup_t *table, size_t n, char const *path) {
    lookup_t *found = bsearch(
        path,
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
            def_t *items;
        } obj;
        struct {
            size_t n;
            def_t *items;
        } list;
        char *text;
    };
};

// Internal freeing function for defs which doesn't call the top level free so
// that it can be used internally.
static void def_internalFree(def_t *d) {
    switch (d->type) {
        case def_type_LIST:
            for (int i = 0; i < d->list.n; i++) {
                def_internalFree(d->list.items + i);
            }
            free(d->list.items);
            break;
        case def_type_OBJ:
            for (int i = 0; i < d->obj.n; i++) {
                def_internalFree(d->obj.items + i);
            }
            free(d->obj.refs);
            free(d->obj.items);
            break;
        case def_type_TEXT:
            free(d->text);
            break;
    }
}

// Initialises an already allocated text def node by giving it a copy of some
// text.
static void def_initText(def_t *d, char const *text) {
    d->type = def_type_TEXT;
    d->text = strndup(text, def_MAX_TEXT_LENGTH);
}

// Initialises a single already defined def_t. Returns true if it worked and
// false if not. One ought to be able to assume that if the function returns
// false then no additional memory has been used up.
static bool def_init(def_t *d, cJSON *cj) {
    char const *text;
    if (cJSON_IsInvalid(cj)) {
        text = "";
    } else if (cJSON_IsTrue(cj)) {
        text = "true";
    } else if (cJSON_IsFalse(cj)) {
        text = "false";
    } else if (cJSON_IsNull(cj)) {
        text = "";
    } else if (cJSON_IsString(cj)) {
        text = cj->valuestring;
    } else if (cJSON_IsNumber(cj)) {
        printf("Haven't added json number support yet sorry\n");
        return false;
    } else if (cJSON_IsArray(cj)) {
        int size = cJSON_GetArraySize(cj);
        def_t *children = malloc(sizeof(def_t) * size);
        if (children == NULL) {
            printf("OOM initialising def\n");
            return false;
        }
        cJSON *jChild = cj->child;
        for (int i = 0; i < size; i++) {
            if (!def_init(children + i, jChild)) {
                // TODO: not enough to ensure no memory leak.
                free(children);
                return false;
            }
            jChild = jChild->next;
        }
        d->type = def_type_LIST;
        d->list.items = children;
        d->list.n = size;
        return true;
    } else if (cJSON_IsObject(cj)) {
        int size = cJSON_GetArraySize(cj);
        def_t *children = malloc(sizeof(def_t) * size);
        lookup_t *lookups = malloc(sizeof(lookup_t) * size);
        if (children == NULL || lookups == NULL) {
            printf("OOM initialising def\n");
            return false;
        }
        cJSON *jChild = cj->child;
        for (int i = 0; i < size; i++) {
            if (!def_init(children + i, jChild)) {
                // TODO: not enough to ensure no memory leak.
                free(children);
                return false;
            }
            lookups[i] = (lookup_t) {
                .text = strndup(jChild->string, def_MAX_TEXT_LENGTH),
                .def = children + i
            };
            jChild = jChild->next;
        }
        qsort(lookups, size, sizeof(lookup_t), lookup_comp);
        d->type = def_type_OBJ;
        d->obj.items = children;
        d->obj.refs = lookups;
        d->obj.n = size;
        return true;
    }
    d->type = def_type_TEXT;
    d->text = strndup(text, def_MAX_TEXT_LENGTH);
    return true;
}

// Manages a set of cursors for iterating over permutations.
// Paths are associated with each cursor in order to aid in the creation of
// child cursor sets.
// You will note that we have set a maximum of def_MAX_CURSORS on the number of
// cursors in a set. This is partly laziness but 8 cursors can easily produce
// trillions of unique permutations or more so I really don't think anybody
// would want more on purpose.
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

// Shared body of implementation for def_cursor_make and def_cursor_makeChild.
// the def_t must always be provided but the parent cursor can be null to say
// that it is top level.
// Can return NULL on error.
static def_cursor_t *def_cursor_makeAll(
    def_t *d,
    def_cursor_t *parent,
    char const **paths,
    size_t nPaths
) {
    if (nPaths > def_MAX_CURSORS) nPaths = def_MAX_CURSORS;
    def_cursor_t *child = malloc(sizeof(def_cursor_t));
    if (child == NULL) {
        printf("OOM allocating child cursor\n");
        return NULL;
    }
    child->defs = d;
    child->valid = true;
    int slot = 0;
    for (int i = 0; i < nPaths; i++) {
        def_t *item;
        if (parent) item = def_cursor_getNode(parent, paths[i]);
        else item = def_getNode(d, paths[i]);
        if (!item) {
            free(child);
            return NULL;
        }
        if (item->type != def_type_LIST) continue;
        if (item->list.n == 0) continue;
        child->cursors[slot].path = paths[i];
        child->cursors[slot].start = item->list.items;
        child->cursors[slot].current = item->list.items;
        child->cursors[slot].end = item->list.items + item->list.n;
        slot++;
    }
    if (slot == 0) {
        free(child);
        printf("Cursor with no valid thingies\n");
        return NULL;
    }
    child->n = slot;
    return child;
}

def_t *def_fromJSON(char const *json) {
    cJSON *cj = cJSON_Parse(json);
    def_t *root = malloc(sizeof(def_t));
    if (root == NULL) {
        printf("OOM allocating root def\n");
        return NULL;
    }
    if (!def_init(root, cj)) {
        free(root);
        return NULL;
    }
    return root;
}

void def_free(def_t *d) {
    def_internalFree(d);
    free(d);
}

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
    if (child == NULL) return NULL;
    return def_getNode(child, utils_pathBody(path));
}

void def_print(def_t *d) {
    char const *type;
    switch (d->type) {
        case def_type_LIST:
            type = "list";
            break;
        case def_type_OBJ:
            type = "obj";
            break;
        case def_type_TEXT:
            type = "text";
            break;
    };
    printf("(%s ", type);
    if (d->type == def_type_LIST) {
        for (int i = 0; i < d->list.n; i++) {
            def_print(d->list.items + i);
        }
    } else if (d->type == def_type_OBJ) {
        for (int i = 0; i < d->obj.n; i++) {
            printf("%s: ", d->obj.refs[i].text);
            def_print(d->obj.refs[i].def);
        }
    } else if (d->type == def_type_TEXT) {
        printf("%s ", d->text);
    }
    printf(")");
}

def_cursor_t *def_cursor_make(def_t *d, char const **paths, size_t nPaths) {
    return def_cursor_makeAll(d, NULL, paths, nPaths);
}

def_cursor_t *def_cursor_makeChild(
    def_cursor_t *parent,
    char const **paths,
    size_t nPaths
) {
    return def_cursor_makeAll(parent->defs, parent, paths, nPaths);
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