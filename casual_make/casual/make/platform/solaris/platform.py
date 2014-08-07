
from casual.make.platform.platform_unix import *


class Solaris( CommonUNIX):
    
    def pre_make(self):
        print
        print '# include static platform specific'
        print "include $(CASUALMAKE_PATH)/casual_make/casual/make/platform/solaris/static.mk"
        print
    
        
        
 
      



