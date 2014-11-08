
from casual.make.platform.platform_unix import *

import os

class OSX( CommonUNIX):
    
    def pre_make(self):
        print
        print '# include static platform specific'
        print 'include ' + os.path.dirname( os.path.realpath(__file__)) + '/static.mk'
        #print "include $(CASUALMAKE_PATH)/casual_make/casual/make/platform/osx/static.mk"
        print
    
        
        
 
      



