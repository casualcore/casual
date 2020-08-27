# casual service

```shell
host# casual --help service

  service [0..1]
        service related administration

    SUB OPTIONS
      -ls, --list-services [0..1]

      -li, --list-instances [0..1]
            list instances

      --list-routes [0..1]
            list service routes

      -mr, --metric-reset [0..1]  (<service name>...) [0..*]
            reset metrics for provided services, if no services provided, all metrics will be reset

      --list-admin-services [0..1]
            list casual administration services

      --legend [0..1]  (list-admin-services, list-service) [1]
            the legend for the supplied option
            
            Documentation and description for abbreviations and acronyms used as columns in output
            
            note: not all options has legend, use 'auto complete' to find out which legends are supported.
                  

      --information [0..1]
            collect aggregated information about services in this domain

      --state [0..1]  (json, yaml, xml, ini) [0..1]
            service state

```