
import sys
import re
makefile_cmk_name=sys.argv[1]    
    
makefile_cmk=open(makefile_cmk_name,"r")
content=makefile_cmk.read()

def adjustQuotes(indata):
    """Add quotes to method arguments"""
    utdata=""
    for c in indata:
        if c == '(':
            utdata = utdata + c + r'"'
        elif c ==')':
            utdata = utdata + r'"' + c
        elif c == ',':
            utdata = utdata + r'"' + c + r'"'
        else:  
            utdata = utdata + c
    return utdata


print fix(content)
