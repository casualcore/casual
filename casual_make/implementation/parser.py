'''
Created on 28 apr 2012

@author: hbergk
'''

#
# Imports
#
import re


class Parser(object):
    '''
    Purpose: Format and parse the casual_make-file
    '''


    def __init__(self, casual_makefile):
        '''
        Constructor
        '''
        self.casual_makefile = casual_makefile
        self.content=""
        self.defines=""
        
    def normalize(self):
        '''
        ######################################################################
        ## 
        ## Denna fil normaliserar en imake-fil
        ## 
        ## Malsattningen ar att fa alla "anrop" till funktioner i en imake-fil
        ## ska komma pa en rad och ha ':' som avskiljare mellan parametrar, detta 
        ## da tjanster deklareras med "tjanst1,tjanst2". Vi vill alltsa behalla
        ## ',' mellan dessa "sub-parametrar"
        ##
        ## Exempelvis: 
        ##   LinkAtmiServer(someServer:obj1. obj2.:lib1.so lib2.so:"tjanst1,tjanst2")    
        ##
        ######################################################################
        '''
 
        #
        # Open file and read content
        #
        casual_makefile=open(self.casual_makefile,"r")
        content=casual_makefile.read()
        #
        # Do some cleaning
        #
        content = self.removeComments( content)
        content = self.removeSpacesAndNewlineEscapes( content)
        #
        # Extract defines from file
        #
        self.defines = self.extractDefines( content)
        content = self.removeDefines(content)
        #
        # Adjust quotes in statements. 
        #
        content = self.adjustQuotes(content)
        self.content=content + '\n'
        #
        # Add post rules
        #
        self.content = self.content + "internal_post_make_rules()\n"      

    #
    # Helpers
    #
    def removeComments(self, indata) :
        """Removing C/C++/#-comments from input"""
        indata=re.sub(r'/\*[\s\S]*\*/', '', indata)
        indata=re.sub(r'//+.*\n', '', indata)
        return re.sub("#.*\n","", indata)
    
    def removeSpacesAndNewlineEscapes(self, indata):
        """Removing extra whitespaces and NewlineEscapes."""
        indata=re.sub(r"\\[\t ]*\n","", indata)
        indata=re.sub(r'\n+[ \t]*\n',r'\n', indata)
        return re.sub("[ \t]+"," ", indata)
    
    def extractDefines(self, indata):
        """Extracting the definestatements"""
        return re.findall(r'(\S+[ \t]*=[ \t]*.*)\n',indata)
               
    def removeDefines(self, indata):
        """Remove the definestatements"""
        return re.sub(r'(\S+[ \t]*=[ \t]*.*)\n','',indata)

    def adjustWhitespace(self,indata):
        """Remove starting and trailing whitespaces"""
        utdata=re.sub(r' *" *','"',indata)
        return utdata
    
    def adjustQuotes(self,indata):
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
        return self.adjustWhitespace( utdata)    
    
