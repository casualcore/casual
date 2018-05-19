# call `casual CLI`

``` 
  call  0..1  (<service> [.binary,.ini,.json,.xml,.yaml]) [1..2]
        generic service call
        
        * service   name of the service to invoke
        * [buffer]  optional buffer type how to interpret the payload [.binary,.ini,.json,.xml,.yaml]
        
        reads from standard in and either treats the paylad as the provided format, or tries to deduce format based on payload
```
