
from casual.make.platform.platform_unix import *


class Solaris( CommonUNIX):
    
    def pre_make(self):

        path = os.path.dirname( os.path.realpath(__file__));
        
        print
        print '#'
        print '# Common stuff'
        print 'include ' + path + '/../common.mk'
        print
        print '# include static platform specific'
        print 'include ' + path + '/static.mk'
        print
    
        
        
 
      



