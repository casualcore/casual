//!
//! casual
//!

#ifndef TEST_VO_H_
#define TEST_VO_H_


#include "sf/namevaluepair.h"
#include "sf/archive/archive.h"

#include "sf/platform.h"
#include "sf/pimpl.h"


#include <functional>


namespace casual
{
   namespace test
   {

      struct SimpleVO
      {

         SimpleVO() = default;
         SimpleVO( long value) : m_long{ value} {}

         SimpleVO( std::function<void(SimpleVO&)> foreign) { foreign( *this);}


         bool m_bool = false;
         long m_long = 123456;
         std::string m_string = "foo";
         short m_short = 256;
         long long m_longlong = std::numeric_limits< long long>::max();
         sf::platform::time_point m_time = sf::platform::time_point::max();

         template< typename A>
         void serialize( A& archive)
         {
            archive & CASUAL_MAKE_NVP( m_bool);
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
   m_bool: false
   m_long: 234
   m_string: bla bla bla bla
   m_short: 23
   m_longlong: 1234567890123456789
   m_time: 1234567890
)";
         }

         static std::string json()
         {
            return R"({
"value":
   {
      "m_bool": false,
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
   <m_bool>false</m_bool>
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
