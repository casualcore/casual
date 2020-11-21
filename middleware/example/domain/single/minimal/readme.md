
# minimal single domain example

## prerequisites

See [domain example]( ../../readme.md)


## create domain

Create a directory where you want your domain to "live".

**In production you probably want to have a dedicated user for a domain and just use the "domain-user" home directory as the domain root**

Copy the domain setup from the example:

```bash
>$ cd <your domain directory>
>$ cp -r $CASAUL_HOME/example/domain/minimal/* .
```

Edit `domain.env` so it corresponds to your setup.

Source the env file:

```bash     
>$ source domain.env 
```

## start domain

```bash
>$ casual domain --boot configuration/domain.yaml
```




