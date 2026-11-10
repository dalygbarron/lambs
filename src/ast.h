#pragma once

// Defines the structure and use of AST nodes, and the running of the actual
// compilation which occurs through the walking of the tree of AST nodes.
// It also contains the parsing functions which produce AST nodes since there
// aren't very many of them.

#include <stdio.h>

// forward declaration of main type.
typedef struct ast_t_ ast_t;

// Parses the given lambs source code and returns a tree of ast nodes. The ast
// will not reference text in the source code so you can free it after.
ast_t *ast_parse(char const *source);

// Walks a tree of ast nodes and writes generated text directly to the provided
// file.
bool ast_walk(ast_t *a, FILE *out);

// Frees the provided ast and anything it's holding onto.
void ast_free(ast_t *a);