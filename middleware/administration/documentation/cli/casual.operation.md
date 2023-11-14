# casual

```shell
host# casual --help 

DESCRIPTION
  
  casual administration CLI
  
  To get more detailed help, use any of:
  casual --help <option>
  casual <option> --help
  casual --help <option> <option> 
  
  Where <option> is one of the listed below

OPTIONS        c  value                             vc    description                                                                             
-------------  -  --------------------------------  ----  ----------------------------------------------------------------------------------------
--information  ?  <value>                              *  collect general aggregated information about the domain                                 
domain         ?                                          local casual domain related administration                                              
service        ?                                          service related administration                                                          
queue          ?                                          queue related administration                                                            
transaction    ?                                          transaction related administration                                                      
gateway        ?                                          gateway related administration                                                          
discovery      ?                                          responsible for discovery stuff                                                         
call           ?                                          generic service call                                                                    
describe       ?  <service> [json, yaml, xml, ini]  1..2  service describer                                                                       
buffer         ?                                          buffer related 'tools'                                                                  
configuration  ?                                          configuration utility - does NOT actively configure anything                            
pipe           ?                                          pipe related options                                                                    
--color        ?  [true, false, auto]                  1  set/unset color - if auto, colors are used if tty is bound to stdout (default: false)   
--header       ?  [true, false, auto]                  1  set/unset header - if auto, headers are used if tty is bound to stdout (default: true)  
--precision    ?  <value>                              1  set number of decimal points used for output (default: 3)                               
--block        ?  [true, false]                        1  set/unset blocking - if false return control to user as soon as possible (default: true)
--verbose      ?  [true, false]                        1  verbose output (default: false)                                                         
--porcelain    ?  [true, false]                        1  backward compatible, easy to parse output format (default: false)                       
internal       ?                                          internal casual stuff for troubleshooting etc...                                        
--version      ?                                          display version information                                                             
--help         ?  <value>                              *  use --help <option> to see further details                                              

```
