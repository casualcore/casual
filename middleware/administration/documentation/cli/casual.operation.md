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

name           value(s)                          description                                                                             
-------------  --------------------------------  ----------------------------------------------------------------------------------------
domain         -                                 local casual domain related administration                                              
service        -                                 service related administration                                                          
queue          -                                 queue related administration                                                            
transaction    -                                 transaction related administration                                                      
gateway        -                                 gateway related administration                                                          
discovery      -                                 responsible for discovery stuff                                                         
call           -                                 generic service call                                                                    
describe       <service> [json, yaml, xml, ini]  service describer                                                                       
buffer         -                                 buffer related 'tools'                                                                  
configuration  -                                 configuration utility - does NOT actively configure anything                            
pipe           -                                 pipe related options                                                                    
--information  <value>                           collect general aggregated information about the domain                                 
--color        true,false,auto                   set/unset color - if auto, colors are used if tty is bound to stdout (default: false)   
--header       true,false,auto                   set/unset header - if auto, headers are used if tty is bound to stdout (default: true)  
--precision    <value>                           set number of decimal points used for output (default: 3)                               
--block        true,false                        set/unset blocking - if false return control to user as soon as possible (default: true)
--verbose      true,false                        verbose output (default: false)                                                         
--porcelain    true,false                        backward compatible, easy to parse output format (default: false)                       
internal       -                                 internal casual stuff for troubleshooting etc...                                        
--version      -                                 display version information                                                             
--help         <value>                           shows this help information                                                             
```
