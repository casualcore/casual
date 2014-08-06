
from casual.make.platform.platform_unix import *


class OSX( CommonUNIX):
    
    def prologue(self):
        print
        print '# include static platform specific'
        print "include $(CASUALMAKE_PATH)/casual_make/casual/make/platform/osx/static.mk"
        print
    
        
        
 
      



