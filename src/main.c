#include <stdio.h>
#include "def.h"
#include "utils.h"

// Yeah baby lets go.
int main(int argc, char **argv) {
    char *json = utils_slurp("example.json");
    def_t *def = def_fromJSON(json);
    char const *paths[2] = {"state", "func"};
    char const *subPaths[1] = {"state.param"};
    def_cursor_t *cursor = def_cursor_make(def, paths, 2);
    if (cursor != NULL) {
        while (def_cursor_hasMore(cursor)) {
            char const *value = def_cursor_get(cursor, "state.name");
            char const *funcName = def_cursor_get(cursor, "func.name");
            printf("state.name is %s %s\n", value, funcName);
            def_cursor_t *subCursor = def_cursor_makeChild(cursor, subPaths, 1);
            if (subCursor) {
                while (def_cursor_hasMore(subCursor)) {
                    char const *pType = def_cursor_get(subCursor, "state.param.type");
                    char const *pValue = def_cursor_get(subCursor, "state.param.name");
                    printf("  %s %s\n", pType, pValue);
                    def_cursor_inc(subCursor);
                }
                def_cursor_free(subCursor);
            } else {
                printf("failed to get sub cursor\n");
            }
            def_cursor_inc(cursor);
        }
        def_cursor_free(cursor);
    }
    def_free(def);
    return 0;
}