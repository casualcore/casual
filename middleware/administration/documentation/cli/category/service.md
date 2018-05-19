# service `casual CLI`

``` 
  service  0..1
        service related administration

    SUB OPTIONS
      -ls, --list-services  0..1
            list services

      --list-services-legend  0..1
            legend for --list-services output

      -li, --list-instances  0..1
            list instances

      -mr, --metric-reset  0..1  (<service name>...) [0..*]
            reset metrics for provided services, if no services provided, all metrics will be reset

      --list-admin-services  0..1
            list casual administration services

      --state  0..1  ([json,yaml,xml,ini]) [0..1]
            service state
```