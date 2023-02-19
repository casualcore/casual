# log operation

## environment

An arbitrary category model is used and can be activated with regular expression selection.

`casual` has, as of today, the following categories for internal logging:

category             | description
---------------------|----------------------------------
error                | logs any kind of error, always on
warning              | should not be used, either it's an error or it's not
information          | logs information about "big things", 'domain has started', and so on...
parameter            | logs input/output parameters for a casual-sf service
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
$ export CASUAL_LOG=".*"
```

All casual internal to log:
```bash
$ export CASUAL_LOG="^casual.*"
```

Only gateway to log:
```bash
$ export CASUAL_LOG="^casual[.]gateway$"
```

Gateway and transaction:
```bash
$ export CASUAL_LOG="^casual[.](gateway|transaction)$"
```


## format

Each row has the following parts, separated by the delimiter `|`

part           |  description
---------------|------------------
timestamp      | iso-8601 extended date-time with time offset from UTC - example: `2020-03-25T16:36:35.550566+0100`
domain name    | name of the domain that wrote the line
execution id   | uuid that correlates an execution path
process id     | pid of the process that wrote the line
thread id      | id of the thread that wrote the line
process name   | configured alias of server/executable or _basename_ of the executable that wrote the line
trid           | id of the transaction that was active when the line was logged
parent service | the service that is the caller to the current service
service        | name of the current invoked service
log category   | category of the logged line, described above
message        | the actual logged message  

## tools

