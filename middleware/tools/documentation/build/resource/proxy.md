# build-resource-proxy

Tool that compile and link a resource proxy for a specific `XA` resource. 

_XA specification uses the term resource proxy, hence `casual` uses the same term. If you have experience with `Tuxedo`, this is the same thing as `TMS`_




## prerequisite

`casual` is installed and `CASUAL_HOME` is set to the install path.

The wanted resources has to be configured in `$CASUAL_HOME/configuration/resources.(yaml|json|xml...)`. See [casual/middleware/configuration/example](../../../../configuration/example/readme.md) for more information.




## example

Lets say we have configured a db2 resource in `$CASUAL_HOME/configuration/resources.yaml`:

```yaml
resources:
  - key: db2
    server: rm-proxy-db2-static
    xa_struct_name: db2xa_switch_static_std
    libraries:
      - db2
    paths:
      library:
        - ${DB2DIR}/lib64
```

We just use the key to _look up_ the configuration, and use this to compile and link the wanted `resource-proxy`

```bash
host$ casual-build-resource-proxy --resource-key db2
```

Will produce a server (_resource-proxy_) with the name `rm-proxy-db2-static`. The `transaction-manager` will use the same configuration to know which _resource-proxy_ to spawn for a specific _key_ in _domain-configuration_.  

## options

There are a few tweaks one can use when building a `resource-proxy`. The most common is i probably to provide _compiler-directives_ and _link-directives_ for your specific platform. 

```bash
host$ casual-build-resource-proxy --help
NAME
   casual-build-resource-proxy

DESCRIPTION

OPTIONS
   -o, --output <value>
      name of the resulting resource proxy

   -k, --resource-key <value>
      key of the resource

   -c, --compiler <value>
      compiler to use

   -c, --compile-directives <value>
      additional compile directives

   -l, --link-directives <value>
      additional link directives

   -p, --resource-properties <value>
      path to resource properties file

   -v, --verbose
      verbose output

   -s, --keep-source
      keep the generated source file

   --help
      Shows this help
```

## regarding this documentation

This information is targeted for all _roles_  