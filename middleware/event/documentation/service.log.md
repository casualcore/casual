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

column    | format  | description
----------|---------|------------------------
service   | string  | name of the invoked service
parent    | string  | name of the parent service, if any.
pid       | integer | process id of the invoked instance 
execution | uuid    | unique _execution id_, like _breadcrumbs_ 
trid      | xid     | transaction id
start     | integer | when the service was invoked, `us` since epoch.
end       | integer | when the service was done, `us` since epoch
code      | string  | outcome of the service call. `OK` if ok, otherwise the reported error from the service


