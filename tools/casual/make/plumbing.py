import casual.make.plumbingimpl.directive

_plumbing = None

class Plumbing(object):
    """
    Plumbing dispatcher
    """
    
    def __init__(self, version = None):
        #
        # Handle versions. Currently only one
        #
        self.version = version
        self.implementation = casual.make.plumbingimpl.directive
    
    def __getattr__(self, name):
        return getattr( self.implementation, name) 


def plumbing(version = None):
    
    global _plumbing
    if not _plumbing or _plumbing.version != version:
        _plumbing = Plumbing(version)
        
    return _plumbing
