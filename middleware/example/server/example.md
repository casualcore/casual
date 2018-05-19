# casual-example-server

An `XATMI` example server that provides a few general services that serves 
as an example how to produce a server and services.

These services can also be used in test/exploration scenarios.

# services

## casual/example/echo
Returns the provided buffer
   
## casual/example/sink
Discard the provided buffer and returns a _null buffer_

## casual/example/rollback
Returns the provided buffer with `TPFAIL`

## casual/example/conversation
_same as `echo` for now_

## casual/example/uppercase
Applies `::toupper` to every byte in the buffer and returns it.

Hence, it will only make sense to provide a buffer with `X_OCTET` with string data,
or something similar


## casual/example/lowercase
Applies `::tolower` to every byte in the buffer and returns it.

Hence, it will only make sense to provide a buffer with `X_OCTET` with string data,
or something similar


## casual/example/sleep
The service `sleeps` for a fixed amount of time.

The _sleep amount_ is provided via server argument `--sleep` and the unit 
is default `seconds`, but can be provided. 

Exeample:
```
  --sleep 4
  --sleep 4s
  --sleep 1h
  --sleep 34us
  --sleep 13min 
```



## casual/example/work
The service `works` for a fixed amount of time.

The work consist of trying to utilize one core of the cpu as much as possible for
the duration of the `work` 

The _work amount_ is provided via server argument `--work` and the unit
is default `seconds`, but can be provided. 

Exeample:
```
  --work 4
  --work 4s
  --work 1h
  --work 34us
  --work 13min 
```


## casual/example/terminate
The service just calls `std::terminate`, and the server, well, _terminates_

Useful in test-/explore-cases to see what happens when a server _crashes_, what 
happens with possible transactions and so on. 