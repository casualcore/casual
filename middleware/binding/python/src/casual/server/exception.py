class BufferError(SystemError):
    """
    Raised when an XATMI buffer operation fails.
    
    This typically occurs during memory allocation (`tpalloc`), deallocation, 
    or when interacting with invalid or corrupted buffer states.
    """


class CallError(SystemError):
    """
    Raised when an XATMI service invocation fails.
    
    This encompasses errors occurring during synchronous (`tpcall`) or 
    asynchronous (`tpacall`, `tpgetrply`) service requests, such as timeouts, 
    invalid descriptors, or service failures (`TPESVCFAIL`).
    """
