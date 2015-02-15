//!
//! test_vo.h
//!
//! Created on: Mar 31, 2013
//!     Author: Lazan
//!

#ifndef TEST_VO_H_
#define TEST_VO_H_


#include "sf/namevaluepair.h"
#include "sf/archive/archive.h"

#include "sf/platform.h"

#include "sf/pimpl.h"

namespace casual
{
   namespace test
   {

      struct SimpleVO
      {
         SimpleVO( long p_long = 123456,
               const std::string& p_string = "foo",
               short p_short = 256,
               long long p_longlong = 1234567890123456789
               )
         : m_long( p_long), m_string( p_string), m_short( p_short), m_longlong( p_longlong)
         {

         }

         SimpleVO& operator = ( const SimpleVO&) = default;

         long m_long;
         std::string m_string;
         short m_short;
         long long m_longlong;
         sf::platform::time_type m_time;

         template< typename A>
         void serialize( A& archive)
         {
            archive & CASUAL_MAKE_NVP( m_long);
            archive & CASUAL_MAKE_NVP( m_string);
            archive & CASUAL_MAKE_NVP( m_short);
            archive & CASUAL_MAKE_NVP( m_longlong);
            archive & CASUAL_MAKE_NVP( m_time);
         }

         static std::string yaml()
         {
            return R"(
value:
   m_long: 234
   m_string: bla bla bla bla
   m_short: 23
   m_longlong: 1234567890123456789
   m_time: 1234567890
)";
         }

         static const char* json()
         {
            return R"({
   "value":
   {
      "m_long": 234,
      "m_string": "bla bla bla bla",
      "m_short": 23,
      "m_longlong": 1234567890123456789,
      "m_time": 1234567890
   }
}
)";
         }

         static std::string xml()
         {
            return R"(<?xml version="1.0"?>
<value>
   <m_long>234</m_long>
   <m_string>bla bla bla bla</m_string>
   <m_short>23</m_short>
   <m_longlong>1234567890123456789</m_longlong>
   <m_time>1234567890</m_time>
</value>
)";
         }



      };

      struct Composite
      {
         std::string m_string;
         std::vector< SimpleVO> m_values;
         std::tuple< int, std::string, SimpleVO> m_tuple;


         template< typename A>
         void serialize( A& archive)
         {
            archive & CASUAL_MAKE_NVP( m_string);
            archive & CASUAL_MAKE_NVP( m_values);
            archive & CASUAL_MAKE_NVP( m_tuple);
         }
      };


      struct Binary : public SimpleVO
      {
         sf::platform::binary_type m_binary;

         template< typename A>
         void serialize( A& archive)
         {
            SimpleVO::serialize( archive);
            archive & CASUAL_MAKE_NVP( m_binary);
         }
      };


      namespace pimpl
      {
         struct Simple
         {

            // user defined
            Simple( long value);

            Simple();
            ~Simple();
            Simple( const Simple&);
            Simple& operator = ( const Simple&);
            Simple( Simple&&) noexcept;
            Simple& operator = ( Simple&&) noexcept;


            long getLong() const;
            const std::string& getString() const;
            std::string& getString();


            void setLong( long value);
            void setString( const std::string& value);



            void serialize( sf::archive::Reader& reader);
            void serialize( sf::archive::Writer& writer) const;


         private:
            class Implementation;
            sf::Pimpl< Implementation> m_pimpl;
         };

      } // pimpl

   } // test
} // casual



#endif /* TEST_VO_H_ */
