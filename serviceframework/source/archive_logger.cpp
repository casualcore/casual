#include "sf/archive_logger.h"
#include "common/logger.h"

#include <iostream>
#include <iomanip>
#include <locale>
//#include <codecvt>

namespace casual
{
namespace sf
{

namespace archive
{

namespace logger
{
   namespace
   {
      namespace local
      {
         const std::size_t indent = 3;
      }
   }

   Implementation::Implementation()
   {
      m_buffer << std::boolalpha << std::fixed << std::setprecision( 6);
   }

   Implementation::~Implementation()
   {
      std::clog << m_buffer.str() << std::endl;
   }

   void Implementation::handle_start( const char* const name)
   {
      m_name = name;
      m_buffer << std::setfill( '-') << std::setw( m_indent * local::indent) << "";
   }

   void Implementation::handle_end( const char* const name)
   {}

   std::size_t Implementation::handle_container_start( const std::size_t size)
   {
      m_buffer << m_name << ' ' << '(' << size << ')' << std::endl;
      ++m_indent;

      return size;
   }

   void Implementation::handle_container_end()
   {
      --m_indent;
   }

   void Implementation::handle_serialtype_start()
   {
      m_buffer << m_name << std::endl;
      ++m_indent;
   }

   void Implementation::handle_serialtype_end()
   {
      --m_indent;
   }

   void Implementation::value_start()
   {
      m_buffer << std::setfill( '.') << std::setw( 25 - m_indent * local::indent) << std::left << m_name << '[';
   }

   void Implementation::value_end()
   {
      m_buffer << ']' << std::endl;
   }

   void Implementation::write_value( const std::string& value)
   {
      m_buffer << value;
   }

   void Implementation::write_value( const std::wstring& value)
   {
      //std::wstring_convert<std::codecvt_utf8<std::wstring::value_type>> converter;
      //m_buffer << converter.to_bytes( value);
      m_buffer << "wide string (" << value.size() << ")";
   }

   void Implementation::write_value( const common::binary_type& value)
   {
      m_buffer << "binary data (" << value.size() << ')';
   }


} // logger

} // archive

} // sf

} // casual
