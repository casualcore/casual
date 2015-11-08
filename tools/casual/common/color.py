

from functools import partial


class Color:
    
    __grey = 30
    __red = 31
    __green = 32
    __yellow = 33
    __blue = 34
    __magenta = 35
    __cyan = 36
    __white = 37
    
    
    __ENDC = '\033[0m'
    

    def __init__(self):
        self.__active = True
        
    
    def __construct(self, color, bold, bright = False):
        if bright:
            color = color + 60
        
        if bold:
            return '\033[1;' + str( color) + 'm'
        else:
            return '\033[0;' + str( color) + 'm'
        
    
    def grey( self, string, bold = False, bright = False):
        if self.__active:
            return self.__construct( self.__grey, bold) + string + self.__ENDC
        else: return string
    
    def magenta( self, string, bold = False, bright = False):
        if self.__active:
            return self.__construct( self.__magenta, bold, bright) + string + self.__ENDC
        else: return string
    
    def red( self, string, bold = False, bright = False):
        if self.__active:
            return self.__construct( self.__red, bold, bright) + string + self.__ENDC
        else: return string
    
    def blue( self, string, bold = False, bright = False):
        if self.__active:
            return self.__construct( self.__blue, bold, bright) + string + self.__ENDC
        else: return string

    def green( self, string, bold = False, bright = False):
        if self.__active:
            return self.__construct( self.__green, bold, bright) + string + self.__ENDC
        else: return string

    def cyan( self, string, bold = False, bright = False):
        if self.__active:
            return self.__construct( self.__cyan, bold, bright) + string + self.__ENDC
        else: return string
    
    def yellow( self, string, bold = False, bright = False):
        if self.__active:
            return self.__construct( self.__yellow, bold, bright) + string + self.__ENDC
        else: return string
    
    def white( self, string, bold = False, bright = False):
        if self.__active:
            return self.__construct( self.__white, bold, bright) + string + self.__ENDC
        else: return string
    
    
    success = green
    
    def error(self, string, bold = True, bright = True):
        return self.red( string, bold, bright)
    
    
    def warning(self, string, bold = True, bright = True):
        return self.magenta( string, bold, bright)
    
    
    header = magenta
    
    
    
    def default(self, string, bold = False):
        return string


    def active( self, value = True):
        self.__active = value

color=Color()

