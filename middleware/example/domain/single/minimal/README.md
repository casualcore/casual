# Minimal single domain example

## Create domain

Create a directory where you want your domain to "live".

In a production environment one probably wants to have a dedicated user for a domain and just use the "domain-user" home directory as the domain root.

Copy the domain setup from the example:

```bash
cd <your domain directory>
cp -r $CASAUL_HOME/example/domain/minimal/* .
```

Edit `domain.env` so it corresponds to your setup. It's just a few lines.

Source the env file:
     
```bash
. domain.env 
```

## Start domain

```bash
casual-admin domain --boot configuration/domain.yaml
```
