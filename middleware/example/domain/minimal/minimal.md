
# minimal domain example

## pre requirements

Make sure you have casual installed and `CASUAL_HOME` set to the installed directory

Make susre you have a resource configuration. If not, copy the example resorce file:
    
    >$ cp $CASUAL_HOME/configuration/example/resources.yaml $CASUAL_HOME/configuration/


## create domain

Create a directory where you want your domain to "live" *in production one probably wants to have a dedicated user for a domain and just use the "domain-user" home directory as the domain root*

Copy the domain setup from the example:

    >$ cd <your domain directory>
    >$ cp -r $CASAUL_HOME/example/domain/minimal/* .

Edit domain.env so it corresponds to your setup. It's just a few lines.

source the env file:
     
    >$ source domain.env 

## start domain

    >$ casual-admin domain --boot 





