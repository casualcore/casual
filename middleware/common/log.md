# log

## objectives

We are confident that a hierarchic log system doesn't really work on most systems in practice. Either you don't get the logs you want, or you get way to much logs.

Our experience shows that a category model works much better. For example:

* error
* warning
* debug
* information

There is no levels involved, and no category implicitly brings in another category, hence one can be very specific what should log.

Sometimes you still might need some levels of log granularity. For example:

* casual.transaction
* casual.transaction.verbose

So you can log basic transaction stuff to `casual.transaction` and keep the nitty gritty low level stuff to `casual.transaction.verbose`   

## solution

We use the arbitrary category model with regular expression selection.

This gives us, in our opinion, the great benefit of category semantics, but still gives possibilities to create hierarchic relations between categories, if one wants.    


## casual log

casual has the following categories for internal logging today.

category             | description
---------------------|----------------------------------
error                | logs any kind of error, allways on
warning              | should not be used, either it's an error or it's not
information          | logs information about "big things", 'domain has started', and so on...
casual.ipc           | logs details about ipc stuff
casual.tcp           | logs details about tcp stuff
casual.gateway       | logs details what gateway is doing
casual.transaction   | logs details about transactions, including TM
casual.queue         | logs details about casual-queue
casual.debug         | logs general debug stuff.


This is all based on just the category model, before the insight with regular expression selection, and we'll most likely start to introduce levels of verbosity.


### example

All categories to log:
```bash
>$ export CASUAL_LOG=".*"
```

All casual internal to log:
```bash
>$ export CASUAL_LOG="^casual.*"
```

Only gateway to log:
```bash
>$ export CASUAL_LOG="^casual[.]gateway$"
```

Gateway and transaction:
```bash
>$ export CASUAL_LOG="^casual[.](gateway|transaction)$"
```


## create new categories


```c++

// header file

namespace log
{
   extern casual::log::Category gateway;

   namespace verbose
   {
      extern casual::log::Category gateway;
   }
}

// TU

namespace log
{
   casual::log::Category gateway{ "casual.gateway"};

   namespace verbose
   {
      casual::log::Category gateway{ "casual.gateway.verbose"};
   }
}

```

Then just start logging to that ostream.
