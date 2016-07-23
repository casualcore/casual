
# API definitions of all the administration services casual have

## Describe services
To describe a casaul-sf-service just use the following command:

```bash
>$ casual-service-describe --services <services>
```

## service example
To get an example of a service just use the following command:

```bash
>$ casual-service-example --services <services> --format json
```

Will produce an example of input and output based on the actual service in the supplied format.

```bash
>$ casual-service-example --help
NAME
   casual-service-example

DESCRIPTION
   Describes a casual service

OPTIONS
   -s, --services <value> 1..*
      services to exemplify

   -f, --format <value>
      format to use [json,yaml,xml,ini] default: json

   --help
      Shows this help

```

## modules

The following modules has an administrative api:

* [domain](../../domain/documentation/api.md)
* [broker](../../broker/documentation/api.md)
* [transaction](../../transaction/documentation/api.md)




## Services
The following administrative services is exposed:

* [.casual.broker.state](../../broker/documentation/api.md#markdown-header-casualbrokerstate)
* [.casual.domain.state](../../domain/documentation/api.md#markdown-header-casualdomainstate)
* [.casual.domain.scale.instances](../../domain/documentation/api.md#markdown-header-casualdomainscaleinstances)
* [.casual.domain.shutdown](../../domain/documentation/api.md#markdown-header-casualdomainshutdown)
* [.casual.transaction.state](../../transaction/documentation/api.md#markdown-header-casualtransactionstate)
* [.casual.transaction.update.instances](../../transaction/documentation/api.md#markdown-header-casualtransactionupdateinstances)






