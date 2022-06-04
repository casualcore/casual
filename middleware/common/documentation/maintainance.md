
# common 

Some maintenance discussions regarding `common`


## name

`common` does not really say anything. I can live with it, but it's not optimal.

One way of thinking about it is to have `common` (or another name) to be a _holder_ for 
_sub-components_ that are not a _real_ component, in a `casual` _component sense_. 

It would be nice to separate `algorithm`, `argument` and `serialize` to be separate _components_.
So we can use them in other potential _casual-products_

## size

`common` is way to big. We need to split common into several pieces. 

## context - call - transaction - server

The _context_ has circular dependencies, which in retrospect is pretty obvious, which makes it
somewhat hard to reason about.

Either come up with a "better" design that still separates these _concepts_ ( which I think would be hard),
or join them in one _context_, that takes care of "all" the state that is needed for transaction and calls.

