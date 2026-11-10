# Lambs Compiler
Lambs is a utf8 clean template language designed specifically for code
generation with the goal of being readable enough that the template itself can
also serve as documentation of the code generated.

More specifically, this is a basic python implementation of a lambs compiler
program which takes templates written in the lambs language and combines
them with a set of definitions provided as JSON and outputs the generated text
to stdout.

I might write a faster implementation with less dependencies in C or whatever
at a later date but I'll probably change a lot of stuff as I go so starting out
with a working python implementation seems wise.

Also keep in mind that given the "template is documentation" ethos, we do not
take pains to make the generated code be formatted nicely, and the language does
not at present handle whitespace very well (so trying to generate correct python
would be difficult). 

## Commandline Interface
The compiler program takes two positional arguments:
```
program template-path defs-path
```
**template-path** is the path to the Lambs file to compile.

**defs-path** is the path to the json file containing definitions for the
template to use.

Either **template-path** or **defs-path** can be set to "-" to indicate that the
content should be read from stdin, but both cannot be set this way of course.

The generated output will be sent to stdout.

## Lambs Language
### Units
The lambs language is not tied to generating any format of text and as such has
no knowledge of the structure of the content it is generating. Instead, it
operates on blocks of text known as units. Top level units are seperated from
one another by lines that are empty or contain only whitespace.

```
Top level unit A

Top level unit B
    erg
Still top level unit B

Top level unit C etc etc
```

### Definitions
In order to transform units into final output, lambs relies on a set of
definitions which must be provided in order to compile a template into output
text. The template declares that it requires a definition with a given name
with the following syntax:
```
<{variable}>
```

We can also require nested definitions like so:
<{variable} <{subVariableA}> <{subvariableB}>>

Definitions which contain sub definitions have no value in their own right
other than being associated with their nested definitions. When a definition
does have a value in it's own right, it is either a piece of text, or a list of
pieces of text (there is no meaningful distinction between a single piece of
text and a list with one element).

We can also add extra keywords to a definition declaration which specify
additional rules, and these can be combined. Since definitions with
subdefinitions have no value of their own these cannot be applied to them.
 - **SINGLE** definition must be exactly one piece of text.
 - **ALPHANUM** definition values must be alphanumeric.

```
//variable must be defined and only one value.
<{variable} SINGLE> 
// you can have lots of definitions of big, but each one has only one name and
// it is alphanumeric.
<{big} <{name} SINGLE ALPHANUM>>
```

You might argue SINGLE would be useful for parent definitions as well but an
important aspect of the language is not making things more long winded than
necessary, so the way to do that in this language would be to elide the parent.

Declarations do technically exist inside units but obviously they don't generate
any text or get substituted or anything. You probably just wanna set them at the
top.

### Substitutions and sub units
The way substitution is performed is that for every unit, substitutions using
the `<definition>` syntax are identified, and the full set of definitions that
the unit relies on is determined. Then, a copy of the unit is added to the
output text for every possible combination of values. Take the following simple
template

```
<{type} ALPHANUM>
<type> add_<type>s(<type> a, <type> b) {
    return a + b;
}
```

If we were to define type as the list (int, float), the generated text would be

```
int add_ints(int a, int b) {
    return a + b;
}
```

So in this example there was only one definition depended upon, but it was a
list of two values and thus two versions of the unit generated. In order to
keep this relatively simple, a top level definition can only depend on top
level definitions (but it can access child definitions marked SINGLE while doing
so). If we want to rely on child definitions, then we need to rely on sub
blocks using the `<()>` syntax, which provide us with a functionality similar to
loops. Lets look at another example:
```
<{type}
    <{name} SINGLE ALPHANUM>
    <{arg} <{name} SINGLE ALPHANUM>>>

</ Addition function for a given type with a user configured number of
</ arguments. The return value will be all of them added together using normal
</ addition.
<type.name> add_<type.name>s(<(,)<type.name> <arg.name>>) {
    return <(+)<arg.name>>;
}
```
And lets say we generated it with the following definitions:
```
{
    type = [
        {name = int, arg = [{name = a, name = b}]},
        {name = float, arg = [{name = a, name = b, name = c}]}
    ]
}
```
Then we now get:
```
int add_ints(int a, int b) {
    return a+b;
}

float add_floats(float a, float b, float c) {
    return a+b+c;
}
```

The way the syntax works is that the text inside the parenthesis is inserted in
between the text generated for each possible version of the sub unit. If () is
given then newlines will be inserted.

While the top level unit could only depend on type (and by extension use
type.name because it is SINGLE), the sub units are able to depend on type.arg,
of course the arg values in question will always correspond to value of type
for which the parent is currently being expanded.

Also, a sub block can depend on sub definitions of definitions that the parent
doesn't appear to depend on if they are sub definitions of a definition that the
parent could in theory depend on, and the parent will then be considered to
depend on it implicitly.

## Compilation process
This is an implementation detail rather than part of the language, but the way
that compilation works is that we iterate the text in a recursive descent style.

We start in a general state that eats whitespace and this continues until we hit
a non whitespace character at which point the unit state is entered.

The unit state iterates until it hits an unescaped < character, if the character
after that is { it enters the declaration state, if it's ? it enters the
conditional state, if it's ( it enters the sub block state, and otherwise it
enters the substitution state. For the purposes of python instead of writing
down the characters of that block it will insert something like "%s" into the
text to substitute in the value. If it's / it enters the comment state.

Most states other than the comment state produce an AST node as output which
should be made a child of the parent state's AST node. The possible AST nodes
are:
 - unit: any block of text that can have substitutions in it and be output. In
   the interests of simplicity sub units use the same node but have the
   seperator field that top level units lack. By the way the seperator is plain
   text you can't perform substitutions in there.
 - conditional: has 3 or 4 units as children and depending on various things
   will only expand 1. A conditional is essentially also a type of unit, or
   at least it can exist in all the same places as a unit.
 - substitution: a unit contains text as well as potentially many substitutions,
   so this represents just a single substitution and it's parent should consider
   whatever it substitutes in a dependency of itself.
 - declaration: has a name, a collection of keywords, and a set of child nodes
   which must be declarations.

### How are dependencies modelled?
When we parse declarations we create AST nodes like for anything else. When we
walk those nodes, all we do is validate that the input definitions match the
declarations.

When we parse a the unit `hi <person.name>!` we find that the substitution
references "person.name" and the dependencies of this substitution would be
handed to it's parent which is a top level unit. In the most basic case this
would cause an error because a top level unit can only depend on top level
definitions.
In order to be able to understand that `person.name` is declared `SINGLE` and
thus only creates a dependency on it's parent when used, we need access to the
declaration info. This leads to two possibilities:
 - we maintain declaration info outside the AST while parsing.
 - dependencies aren't resolved until we walk the AST.

Yeah lets go with the AST walking approach. So basically a unit doesn't store
any dependency info at all, only substitutions do. An unit's render function
takes as input a table of definitions, and then the unit has a method to get
all of it's dependencies, and then there is a function you call on the
definition table with the list of dependencies to get all permutations.

This also works recursively but instead of passing in the top level table of
definitions you pass in the current permutation as it is of the same type.

This table object of course has functionality to resolve a dependency stated as
"state.param.name" or whatever and will complain if it doesn't contain state or
something like that.

And of course sub units also report their dependencies to parent units so that
should be fine.

Man this document just gets longer and longer and more confused hahaha.


### Implementing definition tables.
A definition table can be the top level user provided table which sets the
values of all fields, but it can also be a permutation of that table in which
certain list values have been reduced down to a single value. When you try to
read the value of a list item is when it will throw an error.

No fields in the definition table are allowed to be accessed until a declaration
AST node has been processed which references it, as this allows us to know other
important information about it like if it is SINGLE. This is done through an
API which takes the name qualified with .s like "state.name" or whatever, and
then it also adds the keywords like "SINGLE", "ALPHANUM" etc which can either
be stored to change behaviour or can run some immediate validation which can in
theory crash the program.

When you want to use a definition table to create permutations of a unit, you
must first tigthen the scope of the definition table to only those definitions
you are interested in, and then it is expanded into a whole bunch of definition
tables which are each permutation of every top level list value still in the
table after removing irrelevant entries. These list values are now single values
in each instance of the table.

You can now iterate over each table and use it to generate code. Since the top
level list items are now single values you can access them without issue.

Implementation wise it would probably be good to use some sort of generator or
otherwise iterative approach to creating these permutations of the table since
the number could easily explode if you are depending on a few different things.

If we were writing it in C++ I don't think it would be that hard to create some
kind of iterator for iterating over permutations as long as the state of the
main thingy doesn't change while we are working.


### short version
Unit: Encloses all other types. Delimited by whitespace only lines. Begins at
start of line including whitespace. Contains no dependency information but
determines dependencies based on children and generates textual copy of itself
and it's children for every permutation. copies are on different lines but
nothing else added in between.

<{}> declaration: provides info about structure of user provided info. Like
everything else, exists in a unit but a given declaration AST node does nothing
the second time it is walked and never produces text.

<> substitution: takes the name of a definition potentially using periods for
child definitions. Produces an AST node which when executed will return the
textual value of the definition given the provided def table. Reports
dependency on the thing it substitutes to it's parent unit.

<()> sub unit: Same as a unit except that any text inside the parenthesis is
copied between permutations of body text. the stuff in the parenthesis is
purely textual (and you gotta escape \) to use it of course).

<?> conditional: contains three or four units inside pointy brackets. Any text
outside them is ignored. For each permutation if the text of the first unit
matches that of the second then the text of the third unit is produced, and
otherwise if a fourth exists then it is produced or nothing is.
