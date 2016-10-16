
# minimal multiple domain example

## pre requirements

see [domain example]( ../../domain.md)


## create domains

Create a directory where you want your domains to "live" 

*in production one probably wants to have a dedicated user for each domain and just use the "domain-user" home directory as the domain root*

The following will be used in this example.

    >$ mkdir -p $HOME/casual/example/domain/multiple/medium
    
    
Copy the domains setup from the example:

    >$ cp -r $CASUAL_HOME/example/domain/multiple/medium/* $HOME/casual/example/domain/multiple/medium/

If you chose another base directore for this example, please update the following files so they corresponds with your choice 
 
 * [domain1/domain.env](domain1/domain.env)    
 * [domain2/domain.env](domain2/domain.env) 
 

## start domain1

In terminal 1    
    
    >$ cd $HOME/casual/example/domain/multiple/medium/domain1
    >$ source domain.env
    >$ casual-admin domain --boot 
    
    
## start domain2

In terminal 2

    >$ cd $HOME/casual/example/domain/multiple/medium/domain2
    >$ source domain.env
    >$ casual-admin domain --boot 


## interact with the setup

### In terminal 2


### In terminal 1

## TODO example clients to call the echo server and stuff...


