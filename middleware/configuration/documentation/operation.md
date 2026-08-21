# configuration

`casual` has **one** configuration model, but it can be split into multiple files and aggregated
at runtime (including boot). Hence, it is possible to have a configuration file for transaction, queue, 
gateway, etc. as needed. Or possible even more useful, each application can have its own configuration files.


The configuration model is split into two parts, one that is system wide and one that is domain specific.

## system

* [system](system.operation.md) - general configuration for the system

## domain

* [general](domain.general.operation.md) - general configuration for a casual domain
* [queue](domain.queue.operation.md) - queue configuration for a casual domain
* [gateway](domain.gateway.operation.md) - gateway configuration for a casual domain
* [transaction](domain.transaction.operation.md) - transaction configuration for a casual domain




