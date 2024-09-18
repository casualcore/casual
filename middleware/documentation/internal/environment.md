# internal environment variables

`casual` has a few internal environment variables that is used to be able to 
'configure' and 'tweak' runtime behavior. 
This is not something that `casual` should have, since one of the key objective
with `casual` is to have _zero performance configuration_.
But, we need to have some mechanics to alter some of the _magic values_ to gain 
knowledge of what the _magic values_ should be.

In essence, these environment variables are only valuable to determine the 
actual static _magic value_, and when this is done, the environment variable is
removed.

name                                            | type
------------------------------------------------|----------------------------------
`CASUAL_INTERNAL_DISCOVERY_ACCUMULATE_REQUESTS` | integer
`CASUAL_INTERNAL_DISCOVERY_ACCUMULATE_TIMEOUT`  | duration (ex 3ms)

### CASUAL_INTERNAL_DISCOVERY_ACCUMULATE_REQUESTS

In `domain-discovery`. The limit of discovery requests we have in flight until
we start to accumulate request. Default is `20`.


### CASUAL_INTERNAL_DISCOVERY_ACCUMULATE_TIMEOUT

In `domain-discovery`. The duration for witch we accumulate request until we
send the (accumulated) request downstream. Default is `4ms`


## unittest

The following environment variables are used only in unittest

name                                            | type
------------------------------------------------|----------------------------------
`CASUAL_INTERNAL_UNITTEST_CONTEXT`              | if set, we're in unittest context  
`CASUAL_INTERNAL_GATEWAY_PROTOCOL_VERSION`      | integer (1002 for 1.2)

### CASUAL_INTERNAL_UNITTEST_CONTEXT

Only used internal to know if we're in _unittest context_. 
For example, this is used to set a connect timeout for a gateway outbound 
connection to a few `ms` instead of the default of `3s`. Otherwise unittest with
outbound connections could take way to much time.


### CASUAL_INTERNAL_GATEWAY_PROTOCOL_VERSION

Only used internal during unittest to emulate and test that we can connect and
communicate with older versions of the interdomain protocol.




