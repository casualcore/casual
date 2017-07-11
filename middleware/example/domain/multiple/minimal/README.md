# Minimal multiple domain example

## Create domains

Create a directory where you want your domains to "live".

In a production environment one probably wants to have a dedicated user for each domain and just use the "domain-user" home directory as the domain root.

Copy the domain setup from the example:

```bash
cd <your domains root directory>
cp -r $CASUAL_HOME/example/domain/multiple/minimal/domain* .
```
    
If you chose another base directore for this example, please update `CASUAL_DOMAIN_HOME` in the following files so they corresponds with your choice:
 
 * [domain1/domain.env](./domain1/domain.env)    
 * [domain2/domain.env](./domain2/domain.env) 

## Start

Open two terminal windows.

### Domain 1

In the first terminal:
    
```bash
cd domain1/
. domain.env
casual-admin domain --boot configuration/domain.yaml
```

### Domain 2

In the second terminal:

```bash
cd domain2/
. domain.env
casual-admin domain --boot configuration/domain.yaml
```

## Interact with the domains

### Domain 2
    
List the outbound connections with `casual-admin gateway --list-outbound`. Example output:
    
```
name                             id                                pid  type  runlevel  address
-------------------------------  --------------------------------  ---  ----  --------  -------
multiple-domain-example-domain1  766d151cabaa40d584ffe7e0fd7a869f  737  tcp   online
```
    
List services with `casual-admin broker --list-services`. Example output:

```
name                                  type  mode  timeout  requested  #  state
------------------------------------  ----  ----  -------  ---------  -  -----
.casual.broker.state                    10  none        0          1  1  *    
.casual.domain.scale.instances          10  none        0          0  1  +    
.casual.domain.shutdown                 10  none        0          0  1  +    
.casual.domain.state                    10  none        0          2  1  +    
.casual.gateway.state                   10  none        0          2  1  +    
.casual.transaction.state               10  none        0          0  1  +    
.casual.transaction.update.instances    10  none        0          0  1  +    
casual.example.echo                      0  join        0          0  1  -      
```
                     
`casual.example.echo` is a remote service in this domain.

### Domain 1

List the inbound connections with `casual-admin gateway --list-inbound`. Example output:

```
name                             id                                pid  type  runlevel  address       
-------------------------------  --------------------------------  ---  ----  --------  --------------
multiple-domain-example-domain2  bdd72ed739f94420bb28b7e98ee1a472  738  tcp   online    127.0.0.1:7771 
```

List services with `casual-admin broker --list-services`. Example output:

```
name                                  type  mode  timeout  requested  #  state
------------------------------------  ----  ----  -------  ---------  -  -----
.casual.broker.state                    10  none        0          1  1  *    
.casual.domain.scale.instances          10  none        0          0  1  +    
.casual.domain.shutdown                 10  none        0          0  1  +    
.casual.domain.state                    10  none        0          0  1  +    
.casual.gateway.state                   10  none        0          1  1  +    
.casual.transaction.state               10  none        0          0  1  +    
.casual.transaction.update.instances    10  none        0          0  1  +    
casual.example.echo                      0  join        0          0  1  + 
```

`casual.example.echo` is a local service in this domain.
