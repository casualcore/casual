
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
    
    >$ cd $HOME/casual/example/domain/multiple/minimal/domain1
    >$ source domain.env
    >$ casual-admin domain --boot 
    
    
## start domain2

    >$ cd $HOME/casual/example/domain/multiple/minimal/domain2
    >$ source domain.env
    >$ casual-admin domain --boot 





