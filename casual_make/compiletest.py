
import sys
import re

from string import Template

myTemplate = Template("""
try: 
    $command
except:
    print $method.__doc__
    raise
""")

def removeComments( indata) :
    """Removing C/C++/#-comments from input"""
    indata=re.sub(r'/\*[\s\S]*\*/', '', indata)
    indata=re.sub(r'//+.*\n', '', indata)
    return re.sub("#.*\n","", indata)

def removeSpacesAndNewlineEscapes( indata):
    """Removing extra whitespaces and NewlineEscapes."""
    indata=re.sub(r"\\\n","", indata)
    indata=re.sub(r'\n+[ \t]*\n',r'\n', indata)
    return re.sub("[ \t]+"," ", indata)
     
     

def extractDefines( indata):
    """Extracting the definestatements"""
    return re.findall(r'(\S+[ \t]*=[ \t]*.*)\n',indata)
     
    
def removeDefines( indata):
    """Remove the definestatements"""
    return re.sub(r'(\S+[ \t]*=[ \t]*.*)\n','',indata)

def produceStatement(indata):
    """Produces statements in soon to be the runnable python script"""
    lines=re.split(r'\n', indata)

    output = "from implementation.functiondefinitions import *\n"
    for statement in lines:
        if not re.match(r'^[ \t]*$',statement):
            searchMethod = re.search(r'(\w+)[ \t]*\(', statement)
            method = searchMethod.group(1)
            command = adjustQuotes(statement)
            output = output + myTemplate.substitute(command=command, method=method)
    return output

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
    
makefile_cmk_name=sys.argv[1]    
    
makefile_cmk=open(makefile_cmk_name,"r")
content=makefile_cmk.read()

content = removeComments( content)
content = removeSpacesAndNewlineEscapes( content)
defines = extractDefines( content)
content = removeDefines(content)

makefile_mk=open(makefile_cmk_name + 'mk', "w")
for define in defines:
    makefile_mk.write(define + '\n')


commands = produceStatement( content)

#print commands

try:
    exec commands
except:
    print "Error in " + makefile_cmk_name
    raise


