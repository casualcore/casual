import os
from casual.make.platform.platform_unix import CommonUNIX
from casual.make.platform.registry import RegisterPlatform

@RegisterPlatform("osx")
class OSX( CommonUNIX):
    
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
    
        
        
 
      



