The readme has gotten too long with craziness in there so I am escaping. What
I want to describe is simply the set of main pieces that the codebase will
consist of and what each of them does.
 - The parser. Pretty straightforward, doesn't do much besides creating an AST.
 - AST nodes. Each one does it's own thing but the long and short of it is that
   it takes a def table and returns created text.
 - the def table is the most complex piece. It contains the structure expected
   of user input data which is built up as successive declaration AST nodes are
   processed. It also handles dependency of units on items of user data and
   produces narrowed and permutated versions of itself.

```
<{name} <{text} SINGLE> <{language} <{greeting} SINGLE>>>
<name.language.greeting> my name is <name.text>
```
becomes (unit (sub name.language.greeting name.text)) this means the
sub depends on name.language.greeting, and name.text.

the process of walking that top level unit node is:
 - get dependencies from children (just name.text)
 - create narrow version of top level def table that only contains name.text
 - iterate all permutations of narrowed table and pass those in to exec child
   nodes and get resultant text. sub will call DefTable.get("name.text") or
   something like that, and for the given permutation this should yield a
   lovely value on account of the fact that blah blah blah.
 - spit everything out.

```
Hello my name is george
dobry den my name is george
Hi my name is sally
Hello my name is funny man
```


# C data structures.
Writing this in C++ sucks and writing it in python sucks I think C will be
the best.

The main data structures that we need to deal with are the thingy that stores
all the info that was read in from JSON, and the thingy that stores the
declared form of the user data according to the template and is also used for
dealing with scopes and generating permutations etc.

The JSON data holder is created first and then the declaration one is
constructed iteratively but should be usable at all times.

JSON data being a bunch of nested hashmaps is fairly reasonable.

The other one also being a bunch of nested hashmaps makes sense I think.




When we create a dectree or parts of it, we should set values which only have
one value to the corresponding value in the deftree, and if it is not possible
this indicates an error in the user provided JSON info.

So by default this would only really set the value of top level SINGLE
declarations but when you create permutations again any declaration that you
are capable of getting the value of should have a value set.







