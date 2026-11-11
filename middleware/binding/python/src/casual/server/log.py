from typing import Any

from casual.xatmi import xatmi


def userlog(message: str | bytes | Any, category: str | bytes = "debug") -> int:
    """
    Logs a message to the Casual logging system using the underlying C-API.

    This function bridges Python calls to the native `casual_user_log` function, 
    automatically handling encoding for both category and message arguments.

    Args:
        message (str | bytes | Any): The message to log. Non-string/non-bytes 
            objects will be converted to strings automatically.
        category (str | bytes): The log category or level tag (e.g., "debug", "info"). 
            Defaults to "debug".

    Returns:
        int: The result status code returned by the underlying C function.

    Raises:
        AttributeError: If the underlying C library reference is missing or invalid.
    """
    # Ensure category is encoded to bytes
    cat_ptr = category.encode("utf-8") if isinstance(category, str) else category

    # Ensure message is converted to string/bytes and encoded properly
    if isinstance(message, str):
        msg_ptr = message.encode("utf-8")
    elif isinstance(message, bytes):
        msg_ptr = message
    else:
        msg_ptr = str(message).encode("utf-8")

    # Call the underlying C-API function
    return xatmi.casual_user_log(cat_ptr, msg_ptr)