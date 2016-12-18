
# minimal multiple domain example

## pre requirements

see [domain example]( ../../domain.md)


## create domains

Create a directory where you want your domains to "live" 

*in production one probably wants to have a dedicated user for each domain and just use the "domain-user" home directory as the domain root*

The following will be used in this example.

    >$ mkdir -p $HOME/casual/example/domain/multiple/minimal
    
    
Copy the domains setup from the example:

    >$ cp -r $CASUAL_HOME/example/domain/multiple/minimal/* $HOME/casual/example/domain/multiple/minimal/

If you chose another base directore for this example, please update the following files so they corresponds with your choice 
 
 * [domain1/domain.env](domain1/domain.env)    
 * [domain2/domain.env](domain2/domain.env) 
 

## start domain1

In terminal 1    
    
    >$ cd $HOME/casual/example/domain/multiple/minimal/domain1
    >$ source domain.env
    >$ casual-admin domain --boot 
    
    
## start domain2

In terminal 2

    >$ cd $HOME/casual/example/domain/multiple/minimal/domain2
    >$ source domain.env
    >$ casual-admin domain --boot 


## interact with the domain

### In terminal 2
    
List the outbound connection
    
    >$ casual-admin gateway --list-outbound
    >$ name                             id                                pid  type  runlevel  address
    >$ -------------------------------  --------------------------------  ---  ----  --------  -------
    >$ multiple-domain-example-domain1  766d151cabaa40d584ffe7e0fd7a869f  737  tcp   online
    
List services

    >$ casual-admin broker --list-services
    >$ name                                  type  mode  timeout  requested  #  state
    >$ ------------------------------------  ----  ----  -------  ---------  -  -----
    >$ .casual.broker.state                    10  none        0          1  1  *    
    >$ .casual.domain.scale.instances          10  none        0          0  1  +    
    >$ .casual.domain.shutdown                 10  none        0          0  1  +    
    >$ .casual.domain.state                    10  none        0          2  1  +    
    >$ .casual.gateway.state                   10  none        0          2  1  +    
    >$ .casual.transaction.state               10  none        0          0  1  +    
    >$ .casual.transaction.update.instances    10  none        0          0  1  +    
    >$ casual.example.echo                      0  join        0          0  1  -      
                     
                     
`casual.example.echo` is a remote service in this domain
               

### In terminal 1

    >$ casual-admin gateway --list-inbound
    >$ name                             id                                pid  type  runlevel  address       
    >$ -------------------------------  --------------------------------  ---  ----  --------  --------------
    >$ multiple-domain-example-domain2  bdd72ed739f94420bb28b7e98ee1a472  738  tcp   online    127.0.0.1:7771 

List services

    
    >$ casual-admin broker --list-services
    >$ name                                  type  mode  timeout  requested  #  state
    >$ ------------------------------------  ----  ----  -------  ---------  -  -----
    >$ .casual.broker.state                    10  none        0          1  1  *    
    >$ .casual.domain.scale.instances          10  none        0          0  1  +    
    >$ .casual.domain.shutdown                 10  none        0          0  1  +    
    >$ .casual.domain.state                    10  none        0          0  1  +    
    >$ .casual.gateway.state                   10  none        0          1  1  +    
    >$ .casual.transaction.state               10  none        0          0  1  +    
    >$ .casual.transaction.update.instances    10  none        0          0  1  +    
    >$ casual.example.echo                      0  join        0          0  1  + 

`casual.example.echo` is a local service in this domain


## TODO example clients to call the echo server and stuff...


