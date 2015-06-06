import ctypes
import os
from test.test_multiprocessing import c_int

X_OCTET =  "X_OCTET"
X_C_TYPE = "X_C_TYPE"
X_COMMON = "X_COMMON"

TPEBADDESC = 2
TPEBLOCK = 3
TPEINVAL = 4
TPELIMIT = 5
TPENOENT = 6
TPEOS = 7
TPEPROTO = 9
TPESVCERR = 10
TPESVCFAIL = 11
TPESYSTEM = 12
TPETIME = 13
TPETRAN = 14
TPGOTSIG = 15
TPEITYPE = 17
TPEOTYPE = 18
TPEEVENT = 22
TPEMATCH = 23

TPEV_DISCONIMM  = 0x0001
TPEV_SVCERR     = 0x0002
TPEV_SVCFAIL    = 0x0004
TPEV_SVCSUCC    = 0x0008
TPEV_SENDONLY   = 0x0020

CASUAL_BUFFER_BINARY_TYPE = ".binary"
CASUAL_BUFFER_BINARY_SUBTYPE = ""
CASUAL_BUFFER_JSON_TYPE = ".json"
CASUAL_BUFFER_JSON_SUBTYPE = ""
CASUAL_BUFFER_YAML_TYPE = ".yaml"
CASUAL_BUFFER_YAML_SUBTYPE = ""
CASUAL_BUFFER_XML_TYPE = ".xml"
CASUAL_BUFFER_XML_SUBTYPE = ""


#
# ctypes definitions
#
_file = '../../xatmi/bin/libcasual-xatmi.so'
_path = os.path.join(*(os.path.split(__file__)[:-1] + (_file,)))
_mod = ctypes.cdll.LoadLibrary(_path)

#
# Memory handling - Internal
#
tpalloc = _mod.tpalloc
tpalloc.argtypes = (ctypes.c_char_p, ctypes.c_char_p, ctypes.c_long)
tpalloc.restype = ctypes.POINTER(ctypes.c_char)

tpfree = _mod.tpfree
tpfree.argtypes = (ctypes.c_char_p,)


#
# Error handling - Internal
#

#tperrno = ctypes.c_int.in_dll(_mod, "tperrno")

tperrno = _mod.casual_get_tperrno
tperrno.restype = ctypes.c_int

tperrnostring = _mod.tperrnostring
tperrnostring.argtypes = (ctypes.c_int,)
tperrnostring.restype = ctypes.c_char_p


#
# Calling functions - Internal
#
tpcall = _mod.tpcall
tpcall.argtypes = (ctypes.c_char_p, ctypes.c_char_p, ctypes.c_long, ctypes.POINTER( ctypes.POINTER(ctypes.c_char)), ctypes.POINTER( ctypes.c_long), ctypes.c_long)
tpcall.restype = ctypes.c_int

tpacall = _mod.tpacall
tpacall.argtypes = (ctypes.c_char_p, ctypes.c_char_p, ctypes.c_long, ctypes.c_long)
tpacall.restype = ctypes.c_int

tpgetrply = _mod.tpgetrply
tpgetrply.argtypes = (ctypes.POINTER(ctypes.c_int), ctypes.POINTER( ctypes.POINTER(ctypes.c_char)), ctypes.POINTER( ctypes.c_long), ctypes.c_long)
tpgetrply.restype = ctypes.c_int

tpcancel = _mod.tpcancel
tpcancel.argtypes = (ctypes.c_int,)
tpcancel.restype = ctypes.c_int

