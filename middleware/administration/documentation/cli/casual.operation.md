# casual

```bash
>$ casual --help 

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
call           ?                                          generic service call                                                                    
describe       ?  <service> [json, yaml, xml, ini]  1..2  service describer                                                                       
buffer         ?                                          buffer related 'tools'                                                                  
--color        ?  [true, false]                        1  set/unset color (default: false)                                                        
--header       ?  [true, false]                        1  set/unset header (default: true)                                                        
--precision    ?  <value>                              1  set number of decimal points used for output (default: 3)                               
--block        ?  [true, false]                        1  set/unset blocking - if false return control to user as soon as possible (default: true)
--verbose      ?  [true, false]                        1  verbose output (default: false)                                                         
--porcelain    ?  [true, false]                        1  easy to parse output format (default: false)                                            
--help         ?  <value>                              *  use --help <option> to see further details                                              

```
