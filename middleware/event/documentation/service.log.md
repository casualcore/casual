# casual-event-service-log

Provides metrics for all service calls that har invoked within a domain

## configuration

```
NAME
   casual-event-service-log

DESCRIPTION
  service log

OPTIONS          c  value  vc  description
---------------  -  -----  --  -------------------------------------------
-f, --file       ?          1  path to log-file (default: 'statistics.log')
-d, --delimiter  ?          1  delimiter between columns (default: '|')
--help           ?          *  use --help <option> to see further details

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

Kolumns are separated by the provided `delimeter` option (default `|`)

kolumn    | format  | description
----------|---------|------------------------
service   | string  | name of the invoked service
parent    | string  | name of the parent service, if any.
pid       | integer | process id of the invoked instance 
execution | uuid    | unique _execution id_, like _breadcrumbs_ 
trid      | xid     | transaction id
start     | integer | when the service was invoked, `us` since epoc.
end       | integer | when the service was done, `us` since epoc
code      | string  | outcoume of the service call `OK` if ok, otherwise the reported error from the service


