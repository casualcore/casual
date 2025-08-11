# casual

```shell
host# casual --help 

NAME
   casual-administration-cli-documentation

DESCRIPTION

   
   casual administration CLI
   
   To get more detailed help, use any of:
   casual --help <option>
   casual <option> --help
   casual --help <option> <option> 
   
   Where <option> is one of the listed below

OPTIONS

   domain [0..1]
        local casual domain related administration

   service [0..1]
        service related administration

   queue [0..1]
        queue related administration

   file [0..1]
        file related administration

   transaction [0..1]
        transaction related administration

   gateway [0..1]
        gateway related administration

   discovery [0..1]
        responsible for discovery stuff

   configuration [0..1]
        configuration utility
        
        This utility is used to get, post, put and edit the configuration of
        the casual domain.
                       
        Also functionality to normalize, validate and format the configuration.

   call [0..1]
        generic service call
        
        Reads buffer(s) from stdin and call the provided service, prints the reply buffer(s) to stdout.
        Assumes that the input buffer to be in a conformant format, ie, created by casual.
        Errors will be printed to stderr
        
        @note: part of casual-pipe

   describe [0..1]  (<service> [json, yaml, xml, ini]) [1..2]
        service describer
        
           * service   name of the service to to describe
           * [format]  optional format of the output, if absent a _CLI format_ is used.
        
           attention: the service need to be a casual aware service

   buffer [0..1]
        buffer related 'tools'

   pipe [0..1]
        pipe related options

   --information [0..1]  (<value>) [0..*]
        collect general aggregated information about the domain
        If no directives are provided all the _running_ managers are asked to provide information.
        Otherwise, only the provided directives are used.
        
        use auto-complete to aid valid directives.
        
        valid directives:
        * information-domain
        * information-service
        * information-queue
        * information-transaction

   --color [0..1]  (true, false, auto) [1]
        set/unset color - if auto, colors are used if tty is bound to stdout (default: false)

   --header [0..1]  (true, false, auto) [1]
        set/unset header - if auto, headers are used if tty is bound to stdout (default: true)

   --precision [0..1]  (<value>) [1]
        set number of decimal points used for output (default: 3)

   --block [0..1]  (true, false) [1]
        set/unset blocking - if false return control to user as soon as possible (default: true)

   --verbose [0..1]  (true, false) [1]
        verbose output (default: false)

   --porcelain [0..1]  (true, false) [1]
        backward compatible, easy to parse output format (default: false)
        
        Format: `<column1>|<column2>|...|<columnN>`, with no `ws`. 
        casual guarantees that any new columns will be appended to existing, to preserve compatibility within major versions.
        Hence, column order can differ between `porcelain` and "regular".
        
        @note To view `header` in porcelain-mode an explicit `--header true` must be used.

   internal [0..1]
        internal casual stuff for troubleshooting etc...

   --version [0..1]
        display version information

   --help [0..1]  (<value>) [0..*]
        shows this help information
                       
        Use --help <option> to see selected details on <option>
        You can also use more precise help for deeply nested options
        `--help -a -b -c -d -e`

```
