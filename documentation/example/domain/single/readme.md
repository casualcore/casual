# single domain example

## prerequisites

See [domain example]( ../example.md)


## create domain

Create a directory where you want your domain to "live".

**In production you probably want to have a dedicated user for a domain and just use the "domain-user" home directory as the domain root**

Copy the domain setup from the example:

```bash
$ cd <your domain directory>
$ cp -r $CASAUL_HOME/documentation/example/domain/single/* .
```

Edit `domain.env` so it corresponds to your setup.

Source the env file:

```bash     
$ source domain.env 
```

## start domain

```bash
$ casual domain --boot configuration/domain.yaml
```

## inspect the domain

```
$ casual queue --list-queues 
name      group  rc  rd        count  size  avg  EQ  DQ  UC  last
--------  -----  --  --------  -----  ----  ---  --  --  --  ----
a1        A       0  0.000000      0     0    0   0   0   0  -   
a2        A       0  0.000000      0     0    0   0   0   0  -   
a3        A       0  0.000000      0     0    0   0   0   0  -   
b1        B       0  0.000000      0     0    0   0   0   0  -   
b2        B       0  0.000000      0     0    0   0   0   0  -   
b3        B       0  0.000000      0     0    0   0   0   0  -   
a1.error  A       0  0.000000      0     0    0   0   0   0  -   
a2.error  A       0  0.000000      0     0    0   0   0   0  -   
a3.error  A       0  0.000000      0     0    0   0   0   0  -   
b1.error  B       0  0.000000      0     0    0   0   0   0  -   
b2.error  B       0  0.000000      0     0    0   0   0   0  -   
b3.error  B       0  0.000000      0     0    0   0   0   0  -   
```

```
$ casual service --list-services 
name                 category  mode  timeout   contract  I  C  AT        min       max       P  PAT       RI  RC  last
-------------------  --------  ----  --------  --------  -  -  --------  --------  --------  -  --------  --  --  ----
casual/example/echo  example   join  0.000000    linger  2  0  0.000000  0.000000  0.000000  0  0.000000   0   0  -   
```


## try out some of the CLI

### call echo and enqueue the reply to a1

```
$ echo "casual" | casual buffer --compose | casual call --service casual/example/echo | casual queue --enqueue a1 
43b76f1d6bfc4167aa5a52ad3bd9fb13
```

Check the state:
```
$ casual service --list-services 
name                 category  mode  timeout   contract  I  C  AT        min       max       P  PAT       RI  RC  last                            
-------------------  --------  ----  --------  --------  -  -  --------  --------  --------  -  --------  --  --  --------------------------------
casual/example/echo  example   join  0.000000    linger  2  1  0.000066  0.000066  0.000066  0  0.000000   0   0  2021-10-24T18:03:33.902228+02:00
```

```
$ casual queue --list-queues 
name      group  rc  rd        count  size  avg  EQ  DQ  UC  last                            
--------  -----  --  --------  -----  ----  ---  --  --  --  --------------------------------
a1        A       0  0.000000      1     7    7   1   0   0  2021-10-24T18:03:33.902595+02:00
...
``` 

```
$ casual queue --list-messages a1
id                                S  size  trid  rd  type      reply  available  timestamp                       
--------------------------------  -  ----  ----  --  --------  -----  ---------  --------------------------------
43b76f1d6bfc4167aa5a52ad3bd9fb13  C     7         0  X_OCTET/                 -  2021-10-24T18:18:32.184452+02:00
``` 

Peek and inspect the message

```
$ casual queue --peek a1 43b76f1d6bfc4167aa5a52ad3bd9fb13 | casual buffer --extract 
casual
```



### dequeue from a1 -> call echo -> enqueue b1

```
$ casual queue --dequeue a1 | casual call --service casual/example/echo | casual queue --enqueue b1
f7209b4a11dc4643a8614b74ea5e0508
``` 


Check the state
```
$ $ casual queue --list-queues 
name      group  rc  rd        count  size  avg  EQ  DQ  UC  last                            
--------  -----  --  --------  -----  ----  ---  --  --  --  --------------------------------
a1        A       0  0.000000      0     0    0   1   1   0  2021-10-24T18:03:33.902595+02:00
a2        A       0  0.000000      0     0    0   0   0   0  -                               
a3        A       0  0.000000      0     0    0   0   0   0  -                               
b1        B       0  0.000000      1     7    7   1   0   0  2021-10-24T18:11:02.967124+02:00
b2        B       0  0.000000      0     0    0   0   0   0  -                               
b3        B       0  0.000000      0     0    0   0   0   0  -                               
a1.error  A       0  0.000000      0     0    0   0   0   0  -                               
a2.error  A       0  0.000000      0     0    0   0   0   0  -                               
a3.error  A       0  0.000000      0     0    0   0   0   0  -                               
b1.error  B       0  0.000000      0     0    0   0   0   0  -                               
b2.error  B       0  0.000000      0     0    0   0   0   0  -                               
b3.error  B       0  0.000000      0     0    0   0   0   0  -    
```


### dequeue from b1 and extract the payload from the buffer

```
$ casual queue --dequeue b1 | casual buffer --extract 
casual
``` 


