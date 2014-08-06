

from casual.make.platform.factory import *


class CommonUNIX( Platform):
        

    def link_directive(self, libs):
        directive = ""
        
        if isinstance( libs, basestring):
            libs = libs.split();
        
        for lib in libs:
            directive += ' -l' + lib;
            
        return directive;