import casual.server.buffer as buffer
import casual.xatmi.xatmi as xatmi

def userlog( message, category = 'debug'):
    """
    userlog function hooking in on casual log c-api
    """
    xatmi.casual_user_log( category, message)