import ctypes
import os

X_OCTET =  b"X_OCTET"
X_C_TYPE = b"X_C_TYPE"
X_COMMON = b"X_COMMON"

TPFAIL = 1
TPSUCCESS = 2

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

CASUAL_BUFFER_BINARY_TYPE = b".binary"
CASUAL_BUFFER_BINARY_SUBTYPE = None
CASUAL_BUFFER_INI_TYPE = b".ini"
CASUAL_BUFFER_INI_SUBTYPE = None
CASUAL_BUFFER_JSON_TYPE = b".json"
CASUAL_BUFFER_JSON_SUBTYPE = None
CASUAL_BUFFER_XML_TYPE = b".xml"
CASUAL_BUFFER_XML_SUBTYPE = None
CASUAL_BUFFER_YAML_TYPE = b".yaml"
CASUAL_BUFFER_YAML_SUBTYPE = None

CASUAL_HOME = os.getenv("CASUAL_HOME")

if not CASUAL_HOME:
   raise SystemError( "CASUAL_HOME not set")

#
# ctypes definitions
#
_path = os.getenv("CASUAL_HOME") + '/lib/libcasual-xatmi.so'
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

tpreturn = _mod.tpreturn
tpreturn.argtypes = (ctypes.c_int,ctypes.c_long, ctypes.c_char_p, ctypes.c_long, ctypes.c_long)
tpreturn.restype = None

casual_user_log = _mod.casual_user_log
casual_user_log.argtypes = (ctypes.c_char_p, ctypes.c_char_p)
casual_user_log.restype = ctypes.c_int

class TPSVCINFO(ctypes.Structure):
    _fields_ = [("name", (ctypes.c_char * 128)),
               ("data", ctypes.c_char_p),
               ("len", ctypes.c_long),
               ("flags", ctypes.c_long),
               ("cd", ctypes.c_int)]

tpservice = ctypes.CFUNCTYPE(None, ctypes.POINTER(TPSVCINFO))

class casual_service_name_mapping(ctypes.Structure):
    _fields_ = [("function_pointer", tpservice),
               ("name", ctypes.c_char_p),
               ("category", ctypes.c_char_p),
               ("transaction", ctypes.c_uint64)]

class casual_xa_switch_mapping( ctypes.Structure):
    _fields_ = [("key", ctypes.c_char_p),
               ("xa_switch", ctypes.c_void_p)]

tpsvrinit_type = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int, ctypes.POINTER( ctypes.c_char_p))

class casual_server_argument( ctypes.Structure):
    _fields_ = [("services", ctypes.POINTER(casual_service_name_mapping)),
               ("server_init", tpsvrinit_type),
               ("server_done", ctypes.c_void_p),
               ("argc", ctypes.c_int),
               ("argv", ctypes.POINTER(ctypes.c_char_p)),
               ("xa_switches", ctypes.POINTER(casual_xa_switch_mapping))]

casual_start_server = _mod.casual_start_server
casual_start_server.argtypes = [ ctypes.POINTER(casual_server_argument),]
casual_start_server.restype = ctypes.c_int
