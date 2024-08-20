# casual-event-service-log

Provides metrics for all service calls that har invoked within a domain

## configuration

```
DESCRIPTION
  log service call metrics

OPTIONS             c  value     vc  description                                                
------------------  -  --------  --  -----------------------------------------------------------
-f, --file          ?  <path>     1  where to log (default: "statistics.log")                   
-d, --delimiter     ?  <string>   1  delimiter between columns (default: '|')                   
--filter-inclusive  ?  <regex>    1  only services that matches the expression are logged       
--filter-exclusive  ?  <regex>    1  only services that does NOT match the expression are logged
--help              ?  <value>    *  use --help <option> to see further details 
```

### example

```yaml
domain:
  # ...
  executables: 
    - path: ${CASUAL_HOME}/bin/casual-event-service-log
      arguments: [ --file, logs/service.log]

```

## log format

Columns are separated by the provided `delimiter` option (default `|`)

column       | format    | description
-------------|-----------|------------------------
service      | string    | name of the invoked service
parent       | string    | name of the parent service, if any.
pid          | integer   | process id of the invoked instance 
execution    | uuid      | unique _execution id_, like _breadcrumbs_ 
trid         | xid       | transaction id
start        | integer   | when the service was invoked, `us` since epoch.
end          | integer   | when the service was done, `us` since epoch
pending      | integer   | how long caller had to wait for a non busy server, in `us`
code.result  | string    | outcome of the service call. `OK` if ok, otherwise the reported error from the service
order        | character | "order" of the service - sequential or concurrent, denoted by `S` or `C`. `S` reserves a process, `C` does not.
span         | hex       | 64b OpenTelemetry trace span
parent.span  | hex       | 64b OpenTelemetry parent trace span
code.user    | integer   | user return code from tpreturn


### example

```
some/service|some/parent/service|90053|0bd2c4b424e34e2296c25938766eedbf|10459c8a34e6422a9d5f584856db6701:a6f10817cdd94f3987985a8195e8f927:42:90053|1724141674147439|1724141674189439|6000|OK|S|cdb2b8d804d6e205|9bfeec3308a2a9cd|42
``` 



