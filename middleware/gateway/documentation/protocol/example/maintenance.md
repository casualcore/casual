# gateway interdomain protocol


* [definition](../protocol.maintenance.md)

It is possible to produce the markdown on the fly via `casual-gateway-markdown-protocol`

Example: 

```bash 
$ $CASUAL_HOME/example/bin/casual-gateway-markdown-protocol > protocol.md
```

## binary examples

Use the binary dump example generator.

```bash
$ example/bin/casual-gateway-binary-protocol --help
NAME
   casual-gateway-binary-protocol

DESCRIPTION
  binary dump examples for interdomain protocol

OPTIONS     c  value                                  vc  description                                             
----------  -  -------------------------------------  --  --------------------------------------------------------
-b, --base  ?                                          1  base path for the generated files                       
--format    ?  [ini, jsn, json, log, xml, yaml, yml]   1  format for optional descriptive generated representation
--help      ?                                          *  use --help <option> to see further details     

```

Example: 

```bash
$ $CASUAL_HOME/example/bin/casual-gateway-binary-protocol --base 'path/to/destination' --format json
```

### naming convention

`<message-name>.<protocol-version>.<message-type>.bin`



