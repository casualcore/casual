# queue upgrade

Sometimes we need to change the internal representation of
the persistent queues (_queue-base_), hence we provide a tool for
this upgrade, `casual-queue-upgrade`.


## backup

It's highly recommended to take backups of all persistent _queue-groups_ before an upgrade.

`casual` will not do any backups under the hood. We do the `sqlite` schema upgrade within
a _sqlite transaction_, so it should either work or be back in the old state, but we provide no
absolute guarantees. 

## upgrade

`casual-queue-upgrade --files <queue-base-file>...`

**example:**

Assuming all _queue-base-files_ is located under `$CASUAL_DOMAIN_HOME/queue`. 
```bash
$ casual-queue-upgrade --files $CASUAL_DOMAIN_HOME/queue/*.qb
```

