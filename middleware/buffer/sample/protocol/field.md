
# casual buffer fielded protocol

Defines the binary representation of the fielded buffer

Every field in the buffer has the following parts: `<field-id><size><data>`


| part          | network type | network size  | comments
|---------------|--------------|---------------|---------
| field-id      | uint64       | 8             |
| size          | uint64       | 8             | the size of the data part
| data          | <depends>    | <depends>     | Depends on the _type_ of the field id.


## example

### host values

 type   | field-id     | size value   | value       
--------|--------------|--------------|------------ 
char    | 100664297  |      1 | a
short   | 33555433  |      2 | 42
long    | 67109865  |      8 | 1048576
float   | 134219729  |      4 | 0.0238095
double  | 167774161  |      8 | 0.0238095238095238
string  | 201328593  |      6 | casual
binary  | 234883025  |      12 | 123123123123



### network representation

 type   | field-id     | size value   | value      
--------|--------------|--------------|-------------
char    | 3909287942  |      72057594037927936 | a
short   | 3909287938  |      144115188075855872 | 10752
long    | 3909287940  |      576460752303423488 | 17592186044416
float   | 3506896904  |      288230376151711744 | 822919996
double  | 3506896906  |      576460752303423488 | 1767206661751150655
string  | 3506896908  |      432345564227567616 | casual
binary  | 3506896910  |      864691128455135232 | 123123123123

