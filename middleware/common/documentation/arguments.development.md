# arguments development


## bash completion

To help implement bash completion (or other completion stuff) `common::Arguments` 
provide an option to print an easy to parse list of all available option.

### format

```
<option> <cardinality-min-value> <cardinality-max-value>
```

* Exactly one white space between the values
* If an option has several _aliases_ the last one is used
* The cardinality represents the number of values the specific option expects

### example:

```bash
$ casual casual-bash-completion
domain 1 18446744073709551615
service 1 18446744073709551615
queue 1 18446744073709551615
transaction 1 18446744073709551615
gateway 1 18446744073709551615
help 0 0
casual-bash-completion 0 18446744073709551615
```

### enhancement

The option casual-bash-completion takes `0..*` values that represent the _context_. 
The plan is to use this context to narrow down possible options, hence only suggest
relevant options to the user.

It is probably best to just pass all _whole_ arguments that the user has typed. I'm 
not sure what the semantics are if the current _half_ argument that is _tabbed on_ is
also passed. Well, right now it does not matter since we don't do anything with the 
_context_. but when we implement this, it might give strange result.


