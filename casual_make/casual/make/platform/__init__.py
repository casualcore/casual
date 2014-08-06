
from casual.make.platform.factory import *
from casual.make.platform.osx.platform import *
from casual.make.platform.linux.platform import *
from casual.make.platform.solaris.platform import *


register( 'osx', OSX);
register( 'linux', Linux);
register( 'solaris', Solaris);

