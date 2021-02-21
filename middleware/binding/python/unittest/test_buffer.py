import unittest
import ctypes

import casual.server.buffer as buffer
import casual.server.exception as exception


class TestBufferMethods(unittest.TestCase):

    def test_create_json_buffer(self):

        buf = buffer.JsonBuffer()
        self.assertEqual( buf.size.value, 1024)
        self.assertTrue( isinstance(buf, buffer.JsonBuffer))

    def test_create_xml_buffer(self):
   
        buf = buffer.XmlBuffer()
        self.assertEqual( buf.size.value, 1024)
        self.assertTrue( isinstance(buf, buffer.XmlBuffer))

    def test_create_json_buffer_with_data_string(self):
   
        data = "{'name':'Charlie'}"
        buf = buffer.JsonBuffer( data)
        self.assertTrue( isinstance(buf, buffer.JsonBuffer))
        self.assertEqual( buf.is_bytes, False)
        self.assertEqual( buf.data(), data.encode())


    def test_create_json_buffer_with_data_bytes(self):
        data = b"{'name':'Charlie'}"
        buf = buffer.JsonBuffer(data)
        self.assertTrue( isinstance(buf, buffer.JsonBuffer))
        self.assertEqual( buf.is_bytes, True)
        self.assertEqual( buf.data(), data)

    def test_create_buffer_from_JsonType(self):

        buf = buffer.create_buffer(buffer.JsonBuffer())
        self.assertTrue( isinstance(buf, buffer.JsonBuffer))

    def test_create_buffer_from_XmlType(self):
   
        buf = buffer.create_buffer(buffer.XmlBuffer())
        self.assertTrue( isinstance(buf, buffer.XmlBuffer))
    
    def test_create_buffer_from_UnknownType(self):
      
        self.assertRaises(exception.BufferError, buffer.create_buffer, "unknown")


if __name__ == '__main__':
    unittest.main()
