class Version(object):
    
    def __init__(self, major = 0, minor = 1, age = 0):
        self.major = major
        self.minor = minor
        self.age = age
        
    def full_version(self):
        return str(self.major) + '.' + str(self.minor) + '.' + str(self.age)
    
    def soname_version(self):
        return str(self.major) + '.' + str(self.minor)
 
    

class Output(object):
    
    def __init__(self, name, major = None, minor = None, age = None):
        
        self.name = name
        
        if major:
            self.version = Version( major, minor, age)
        else:
            self.version = None
            
    #def __str__(self):
    #    return self.name

