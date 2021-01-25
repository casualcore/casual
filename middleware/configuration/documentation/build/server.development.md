# configuration build-server

Defines user configuration when building a casual server.

### services

Defines which services the server has (and advertises on startup). The actual `xatmi` conformant
function that is bound to the service can have a different name.

Each service can have different `transaction` semantics:

type         | description
-------------|----------------------------------------------------------------------
automatic    | join transaction if present else start a new transaction (default type)
join         | join transaction if present else execute outside transaction
atomic       | start a new transaction regardless
none         | execute outside transaction regardless

### resources

Defines which `xa` resources to link and use runtime. If a name is provided for a given
resource, then startup configuration phase will ask for resource configuration for that 
given name. This is the preferred way, since it is a lot more explicit.

## examples 

Below follows examples in human readable formats that `casual` can handle

### yaml
```` yaml
---
server:
  default:
    service:
      transaction: "join"
      category: "some.category"
  resources:
    - key: "rm-mockup"
      name: "resource-1"
      note: "the runtime configuration for this resource is correlated with the name 'resource-1' - no group is needed for resource configuration"
  services:
    - name: "s1"
    - name: "s2"
      transaction: "auto"
    - name: "s3"
      function: "f3"
    - name: "s4"
      function: "f4"
      category: "some.other.category"
...

````
### json
```` json
{
    "server": {
        "default": {
            "service": {
                "transaction": "join",
                "category": "some.category"
            }
        },
        "resources": [
            {
                "key": "rm-mockup",
                "name": "resource-1",
                "note": "the runtime configuration for this resource is correlated with the name 'resource-1' - no group is needed for resource configuration"
            }
        ],
        "services": [
            {
                "name": "s1"
            },
            {
                "name": "s2",
                "transaction": "auto"
            },
            {
                "name": "s3",
                "function": "f3"
            },
            {
                "name": "s4",
                "function": "f4",
                "category": "some.other.category"
            }
        ]
    }
}
````
### ini
```` ini

[server]

[server.default]

[server.default.service]
category=some.category
transaction=join

[server.resources]
key=rm-mockup
name=resource-1
note=the runtime configuration for this resource is correlated with the name 'resource-1' - no group is needed for resource configuration

[server.services]
name=s1

[server.services]
name=s2
transaction=auto

[server.services]
function=f3
name=s3

[server.services]
category=some.other.category
function=f4
name=s4

````
### xml
```` xml
<?xml version="1.0"?>
<server>
 <default>
  <service>
   <transaction>join</transaction>
   <category>some.category</category>
  </service>
 </default>
 <resources>
  <element>
   <key>rm-mockup</key>
   <name>resource-1</name>
   <note>the runtime configuration for this resource is correlated with the name 'resource-1' - no group is needed for resource configuration</note>
  </element>
 </resources>
 <services>
  <element>
   <name>s1</name>
  </element>
  <element>
   <name>s2</name>
   <transaction>auto</transaction>
  </element>
  <element>
   <name>s3</name>
   <function>f3</function>
  </element>
  <element>
   <name>s4</name>
   <function>f4</function>
   <category>some.other.category</category>
  </element>
 </services>
</server>

````
