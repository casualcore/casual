# environment

## variables

`casual-domain-manager` sets a few environment variables when spawning executables, 
that user code can use.


name                     | description
-------------------------|-----------------------
`CASUAL_INSTANCE_ALIAS`  | The configured _alias_ of the server/executable (or process basename)
`CASUAL_INSTANCE_INDEX`  | The _index_ of scaled instances the actual instance is `[0..*]`