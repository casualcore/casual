class Version(object):
    
    def __init__(self, major = 0, minor = 1, revision = 0):
        self.major = major
        self.minor = minor
        self.revision = revision
        
    def full(self):
        return str(self.major) + '.' + str(self.minor) + '.' + str(self.revision)
    
    def soname(self):
        return str(self.major) + '.' + str(self.minor)
 
    