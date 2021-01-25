# configuration resource-property

Defines machine global configuration of resources. It's used when building casual servers
and executables, and also by `casual-transaction-manager` to deduce which _xa-resource-proxy-server_ it should start.

### resources

Defines which `xa` resources that are available on a particular _machine_.

properties     | description
---------------|----------------------------------------------------
key            | user supplied _key_ of the resource, used to correlate the resources in other configurations
xa_struct_name | the name of the `xa` struct for the particular resource implementation 
server         | name of the _resource proxy server_ that `transaction manager` delegates _prepare, commit, rollback_ to.
libraries      | libraries that is used link with the resource _build time_ 
paths          | include and library paths, during _build time_


## examples 

Below follows examples in human readable formats that `casual` can handle

### yaml
```` yaml
---
resources:
  - key: "db2"
    server: "rm-proxy-db2-static"
    xa_struct_name: "db2xa_switch_static_std"
    libraries:
      - "db2"
    paths:
      include:
        []
      library:
        - "${DB2DIR}/lib64"
  - key: "rm-mockup"
    server: "rm-proxy-casual-mockup"
    xa_struct_name: "casual_mockup_xa_switch_static"
    libraries:
      - "casual-mockup-rm"
    paths:
      include:
        []
      library:
        - "${CASUAL_HOME}/lib"
...

````
### json
```` json
{
    "resources": [
        {
            "key": "db2",
            "server": "rm-proxy-db2-static",
            "xa_struct_name": "db2xa_switch_static_std",
            "libraries": [
                "db2"
            ],
            "paths": {
                "include": [],
                "library": [
                    "${DB2DIR}/lib64"
                ]
            }
        },
        {
            "key": "rm-mockup",
            "server": "rm-proxy-casual-mockup",
            "xa_struct_name": "casual_mockup_xa_switch_static",
            "libraries": [
                "casual-mockup-rm"
            ],
            "paths": {
                "include": [],
                "library": [
                    "${CASUAL_HOME}/lib"
                ]
            }
        }
    ]
}
````
### ini
```` ini

[resources]
key=db2
libraries=db2
server=rm-proxy-db2-static
xa_struct_name=db2xa_switch_static_std

[resources.paths]
library=${DB2DIR}/lib64

[resources]
key=rm-mockup
libraries=casual-mockup-rm
server=rm-proxy-casual-mockup
xa_struct_name=casual_mockup_xa_switch_static

[resources.paths]
library=${CASUAL_HOME}/lib

````
### xml
```` xml
<?xml version="1.0"?>
<resources>
 <element>
  <key>db2</key>
  <server>rm-proxy-db2-static</server>
  <xa_struct_name>db2xa_switch_static_std</xa_struct_name>
  <libraries>
   <element>db2</element>
  </libraries>
  <paths>
   <include />
   <library>
    <element>${DB2DIR}/lib64</element>
   </library>
  </paths>
 </element>
 <element>
  <key>rm-mockup</key>
  <server>rm-proxy-casual-mockup</server>
  <xa_struct_name>casual_mockup_xa_switch_static</xa_struct_name>
  <libraries>
   <element>casual-mockup-rm</element>
  </libraries>
  <paths>
   <include />
   <library>
    <element>${CASUAL_HOME}/lib</element>
   </library>
  </paths>
 </element>
</resources>

````
