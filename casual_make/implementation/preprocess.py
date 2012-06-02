'''
Created on 27 maj 2012

@author: hbergk
'''

import re
import sys

class Preprocess(object):
    """Handle simple preprocessing"""
    
    def __init__(self, casual_makefile):
        """constructor"""
        self.casual_makefile = open(casual_makefile, "r")
        self.printline = [True]
        
        self.systems = dict({'__APPLE__':'darwin', '__LINUX__':'linux'})
    
    def process(self):
        """Process file"""
        data = ""
        while True:
            line = self.casual_makefile.readline()
            if line:
                m=self.matchPreprocessorDirectives(line)
                if m:
                    self.dispatch(m)
                elif self.printline[len(self.printline)-1]:
                    if line.lstrip():
                        line = line.lstrip()
                    data = data + line
            else:
                break
        return data
                
                
    def matchPreprocessorDirectives(self, line): 
        """Regexp-match supported preprocessor directives"""
        
        m=re.match("[ \t]*# *(ifdef) (.+)", line)
        if m:
            return m
    
        m=re.match("[ \t]*# *(ifndef) (.+)", line)
        if m:
            return m

        m=re.match("[ \t]*# *(elif) (.+)", line)
        if m:
            #
            # Fix to handle that elif is a reserved world in python
            #
            line = re.sub(r'elif', r'preprocessor_elif', line)
            return re.match("[ \t]*#(preprocessor_elif) (.+)", line)
         
        m=re.match("[ \t]*# *(else)", line)
        if m:
            #
            # Fix to handle that else is a reserved world in python
            #
            return re.match("#(preprocessor_else)", "#preprocessor_else")
        
        m=re.match("[ \t]*# *(endif)", line)
        if m:
            return m
        else:
            return None
                 
    def dispatch(self, expression):
        """Dispatch to correct method"""
        if expression:
            command="self." + expression.group(1) 
            if expression.lastindex == 2: 
                command = command + "('" + expression.group(2) + "')"
            else:
                command = command + "()"
            
            self.printline[len(self.printline)-1] = eval(command)
        
        
    def define(self, argument):
        pass
        
    def ifdef(self, argument):
        self.printline.append(True)
        return sys.platform.startswith(self.systems[argument])
 
    def ifndef(self, argument):
        self.printline.append(True)
        return not self.ifdef(argument)

    def preprocessor_elif(self, argument):
        return not self.printline[len(self.printline)-1] and sys.platform.startswith(self.systems[argument])
        
    def preprocessor_else(self):
        return not self.printline[len(self.printline)-1]
    
    def endif(self):   
        self.printline.pop()
        return self.printline[len(self.printline)-1]
        