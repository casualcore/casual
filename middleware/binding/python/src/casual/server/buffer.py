import ctypes
from enum import Enum
from typing import Any, Self

from casual.server.exception import BufferError
from casual.xatmi.xatmi import (
    CASUAL_BUFFER_BINARY_SUBTYPE,
    CASUAL_BUFFER_BINARY_TYPE,
    CASUAL_BUFFER_JSON_SUBTYPE,
    CASUAL_BUFFER_JSON_TYPE,
    CASUAL_BUFFER_XML_SUBTYPE,
    CASUAL_BUFFER_XML_TYPE,
    CASUAL_BUFFER_YAML_SUBTYPE,
    CASUAL_BUFFER_YAML_TYPE,
    X_OCTET,
    tpalloc,
    tperrno,
    tperrnostring,
    tpfree,
)


class BufferType(Enum):
    """Enumeration defining standard XATMI buffer types and their corresponding subtypes."""
    X_OCTET = (X_OCTET, 0)
    BINARY = (CASUAL_BUFFER_BINARY_TYPE, CASUAL_BUFFER_BINARY_SUBTYPE)
    JSON = (CASUAL_BUFFER_JSON_TYPE, CASUAL_BUFFER_JSON_SUBTYPE)
    YAML = (CASUAL_BUFFER_YAML_TYPE, CASUAL_BUFFER_YAML_SUBTYPE)
    XML = (CASUAL_BUFFER_XML_TYPE, CASUAL_BUFFER_XML_SUBTYPE)


def _convert(data: str | bytes) -> tuple[bool, bytes]:
    """Converts string data to bytes and returns a flag indicating whether the origin was bytes."""
    if isinstance(data, str):
        return False, data.encode()
    return True, data


class Buffer:
    """Represents an XATMI buffer with support for context manager management."""

    def __init__(
        self,
        buffertype: bytes,
        subtype: bytes | None = None,
        data: str | bytes | None = None,
    ):
        self._buffertype = buffertype
        self._subtype = subtype
        self.is_bytes = True

        if data is not None:
            self.is_bytes, encoded_data = _convert(data)
            self.size = ctypes.c_long(len(encoded_data))
            self.holder = tpalloc(buffertype, subtype, self.size)
            if not self.holder:
                raise BufferError(tperrnostring(tperrno()))
            self.set(encoded_data)
        else:
            self.size = ctypes.c_long(1024)
            self.holder = tpalloc(buffertype, subtype, self.size)
            if not self.holder:
                raise BufferError(tperrnostring(tperrno()))

    def set(self, data: bytes) -> None:
        """Writes binary data to the underlying buffer memory."""
        ctypes.memmove(self.holder, data, len(data))
        self.size = ctypes.c_long(len(data))

    @property
    def raw(self) -> bytes:
        """Returns the raw ctypes pointer to the allocated buffer memory."""
        return self.holder

    def data(self) -> bytes | str | None:
        """Returns the buffer's contents (decoded back to string if it was originally provided as one)."""
        if self.holder is not None:
            raw_data = self.holder[0 : self.size.value]
        else:
            return None
        return raw_data if self.is_bytes else raw_data.decode()

    def free(self) -> None:
        """Explicitly releases the allocated buffer memory."""
        if getattr(self, "holder", None) and tpfree:
            tpfree(self.holder)
            self.holder = None

    def __enter__(self) -> Self:
        """Enables the buffer to be used safely within a 'with' statement block."""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        """Automatically releases buffer memory when exiting the 'with' block."""
        self.free()

    def __del__(self) -> None:
        """Safety fallback to free memory if the context manager is not explicitly used."""
        try:
            self.free()
        except Exception:  # noqa: BLE001, S110
            pass


class JsonBuffer(Buffer):
    """Specialized XATMI buffer pre-configured for JSON payload handling."""
    def __init__(self, data: str | bytes | None = None):
        super().__init__(*BufferType.JSON.value, data)


class XmlBuffer(Buffer):
    """Specialized XATMI buffer pre-configured for XML payload handling."""
    def __init__(self, data: str | bytes | None = None):
        super().__init__(*BufferType.XML.value, data)


def create_buffer(buffer_obj: Any) -> Buffer:
    """Factory function to create matching empty buffers based on an existing buffer instance."""

    if isinstance(buffer_obj, XmlBuffer):
        return XmlBuffer()
    elif isinstance(buffer_obj, JsonBuffer):
        return JsonBuffer()
    else:
        raise BufferError("Unknown buffer type")