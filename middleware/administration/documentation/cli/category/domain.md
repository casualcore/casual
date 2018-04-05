# domain `casual CLI`

``` 
  domain  0..1
        local casual domain related administration

    SUB OPTIONS
      -ls, --list-servers  0..1
            list all servers

      -le, --list-executables  0..1
            list all executables

      -li, --list-instances  0..1
            list all instances

      -si, --scale-instances  0..1  (<alias> <#>) [2..* {2}]
            <alias> <#> scale executable instances

      -s, --shutdown  0..1
            shutdown the domain

      -b, --boot  0..1  (<value>...) [0..*]
            boot domain

      -p, --persist-state  0..1
            persist current state

      --state  0..1  ([json,yaml,xml,ini]) [0..1]
            domain state

```
