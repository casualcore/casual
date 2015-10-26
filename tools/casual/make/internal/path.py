

import os



def makepath( casual_makefile):

    return os.path.dirname( os.path.abspath( casual_makefile)) + '/.casual/make'


def makestem( casual_makefile):
    
    return os.path.splitext( os.path.basename( casual_makefile))[ 0]


def makefile( casual_makefile):
    
    return makepath( casual_makefile) + '/' + makestem( casual_makefile) + '.mk'
    


