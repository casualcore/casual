# casual domain

```bash
>$ casual --help domain

  domain [0..1]
        local casual domain related administration

    SUB OPTIONS
      -ls, --list-servers [0..1]
            list all servers

      -le, --list-executables [0..1]
            list all executables

      -sa, --scale-aliases [0..1]  (<alias>, <#>) [2..* {2}]
            scale instances for the provided aliases

            deprecated: [-si, --scale-instances]

      -ra, --restart-aliases [0..1]  (<alias>) [1..*]
            restart instances for the given aliases
            
            note: some aliases are unrestartable

            deprecated: [-ri, --restart-instances]

      -rg, --restart-groups [0..1]  (<group>) [0..*]
            restart all instances for aliases that are members of the provided groups
            
            if no groups are provided, all groups are restated.
            
            note: some aliases are unrestartable

      -lis, --list-instances-server [0..1]
            list all running server instances

      -lie, --list-instances-executable [0..1]
            list all running executable instances

      -s, --shutdown [0..1]
            shutdown domain

      -b, --boot [0..1]  (<files>) [0..*]
            boot domain

      --set-environment [0..*]  (<variable>, <value>, [<alias>*]) [2..*]
            set an environment variable for explicit aliases
                                    
            if 0 aliases are provided, the environment virable will be set 
            for all servers and executables 
                                    

      --configuration-get [0..1]  (json, yaml, xml, ini) [0..1]
            get configuration (as provided format)

      --configuration-put [0..1]  (json, yaml, xml, ini) [1]
            reads configuration from stdin and update the domain
            
            The semantics are similar to http PUT:
             * every key that is found is treated as an update of that _entity_
             * every key that is NOT found is treated as a new _entity_ and added to the current state 

      --ping [0..1]  (<alias>) [1..*]
            ping all instances of the provided server alias

      --instance-global-state [0..1]  (<pid>, [<format>]) [1..2]
            get the 'global state' for the provided pid

      --legend [0..1]  (list-executables, list-servers, ping) [1]
            the legend for the supplied option
            
            Documentation and description for abbreviations and acronyms used as columns in output
            
            note: not all options has legend, use 'auto complete' to find out which legends are supported.

      --information [0..1]
            collect aggregated general information about this domain

      --state [0..1]  (json, yaml, xml, ini) [0..1]
            domain state (as provided format)

```
