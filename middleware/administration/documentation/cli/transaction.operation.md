# casual transaction

```shell
host# casual --help transaction

  transaction [0..1]
        transaction related administration

    SUB OPTIONS
      -lt, --list-transactions [0..1]
            list current transactions

      -lr, --list-resources [0..1]
            list all resources

      -li, --list-instances [0..1]
            list resource instances

      --begin [0..1]
            creates a 'single' transaction directive
            
            * associates all upstream transaction aware messages with the 'single' transaction, if they don't are associated already.
            * all directives from upstream will be 'terminated', that is, notify the upstream 'owner' and not forward the directive
            * sends the directive downstream, so other casual-pipe components can associate 'new stuff' with the 'single' transaction
            
            hence, only one 'directive' can be in flight within a link in the casual-pipe.

      --compound [0..1]
            creates a compound transaction directive 
            
            * associates all upstream transaction aware messages with a new transaction, if they don't are associated already.
            * all directives from upstream will be 'terminated', that is, notify the upstream 'owner' and not forward the directive
            * sends the directive downstream, so other casual-pipe components can associate 'new stuff' with a new transaction
            
            hence, only one 'directive' can be in flight within a link in the casual-pipe.

      --commit [0..1]
            sends transaction finalize request to the 'owners' of upstream transactions
            
            * all commitable associated transaction will be sent for commit.
            * all NOT commitable associated transactions will be sent for rollback
            * all transaction directives from upstream will be 'terminated', that is, notify the upstream 'owner' and not forward the directive
            * all 'forward' 'payloads' downstream will not have any transaction associated
            
            hence, directly downstream there will be no transaction, but users can start new transaction directives downstream.

      --rollback [0..1]
            sends transaction (rollback) finalize request to the 'owners' of upstream transactions
            
            * all associated transaction will be sent for rollback
            * all transaction directives from upstream will be 'terminated', that is, notify the upstream 'owner' and not forward the directive
            * all 'forward' 'payloads' downstream will not have any transaction associated
            
            hence, directly downstream there will be no transaction, but users can start new transaction directives downstream.

      -si, --scale-instances [0..1]  (rm-id, # instances) [0..* {2}]
            scale resource proxy instances

      -lp, --list-pending [0..1]
            list pending tasks

      --information [0..1]
            collect aggregated information about transactions in this domain

      --state [0..1]  (json, yaml, xml, ini) [0..1]
            view current state in optional format

```