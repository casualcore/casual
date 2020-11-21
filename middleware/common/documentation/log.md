# log

## overview

We are confident that a hierarchic log system doesn't really work on most systems in practice. Either you don't get the logs you want, or you get way too many logs

Our experience shows that a category model works much better. For example:

* error
* warning
* debug
* information

There are no levels involved, and no category implicitly brings in another category, hence one can be very specific what should log.

Sometimes you still might need some levels of log granularity. For example:

* casual.transaction
* casual.transaction.verbose

So you can log basic transaction stuff to `casual.transaction` and keep the nitty gritty low level stuff to `casual.transaction.verbose`   

This gives us, in our opinion, the great benefit of category semantics, but still gives possibilities to create hierarchical relations between categories, if wanted.


### [development](log.development.md)

### [operation](log.operation.md)

### [maintenance](log.maintenance.md)
