# casual discovery

[//]: # (Attention! this is a generated markdown from casual-administration-cli-documentation - do not edit this file!)

```console
host# casual --help discovery

discovery [0..1]
     responsible for discovery stuff

   SUB OPTIONS:

      --list-providers [0..1]
           INCUBATION - list current discovery providers
           
           These are the providers that has registered them self with discovery abilities
           
           @attention INCUBATION - might change during, or in between minor version.

      --services [0..1]  (<value>) [1..*]
           force discover of provided services
           
           Will try to find provided services in other domains.

      --queues [0..1]  (<value>) [1..*]
           force discover of provided queues
           
           Will try to find provided queues in other domains.

      --rediscover [0..1]
           rediscover all 'discoverable' agents

      --metric [0..1]
           list metrics
           
           List counts of _discovery tasks_ the domain-discovery has in-flight and completed
           
           * discovery-out: 
              Requests to other domains
           * discovery-in: 
              Requests from other domains
           * lookup: 
              Requests to local providers that has service/queues
           * fetch-known:
              Requests to local providers about the total set of known service/queues
           
           @attention INCUBATION - might change during, or in between minor version.

      [deprecated] --metric-message-count [0..1]
           @removed use `casual internal --message-count <pid>` instead

      --state [0..1]  (json, yaml, xml, ini, line) [0..1]
           prints state in the provided format to stdout

```
