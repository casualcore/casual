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


    def __init__(self, preprocessed_data):
        '''
        Constructor
        '''
        self.preprocessed_data = preprocessed_data
        self.content=""
        self.defines=""
        self.includes=""
        
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
        content=self.preprocessed_data
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
        # Extract includes from file
        #
        self.includes = self.extractIncludes( content)
        content = self.removeIncludes(content)
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
        return re.sub(r'//+.*\n', '', indata)
        #return re.sub("#.*\n","", indata)
    
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

    def extractIncludes(self, indata):
        """Extracting the include statements"""
        return re.findall(r'[ \t]*Include[ \t]*\((.*)\)',indata)
               
    def removeIncludes(self, indata):
        """Remove the include statements"""
        return re.sub(r'[ \t]*Include[ \t]*\((.*)\).*\n','',indata)

    def adjustWhitespace(self,indata):
        """Remove starting and trailing whitespaces"""
        utdata=re.sub(r' *" *','"',indata)
        return utdata
    
    def adjustQuotes(self,indata):
        """Add quotes to method arguments"""

        utdata=""
        quote = False
        for c in indata:
            if c == '(':
                utdata = utdata + c + r'"'
                quote = not quote
            elif c ==')':
                utdata = utdata + r'"' + c
                quote = not quote
            elif c == ',':
                if quote:
                    utdata = utdata + r'"' + c + r'"'
                else:
                    utdata = utdata + c
            elif c == '"':
                #utdata = utdata + c
                quote = not quote
            else:  
                utdata = utdata + c
        #
        # remove quotes if no argument. i.e. method("")
        #
        utdata = re.sub(r'\(\"[ \t]*\"\)',r'()',utdata)
        return self.adjustWhitespace( utdata)    
    
