//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/serialize/macro.h"
#include "common/serialize/archive.h"

#include "common/pimpl.h"
#include "common/uuid.h"

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
         platform::time::point::type m_time = platform::time::point::type::max();

         std::optional< long> m_optional = 42;

         CASUAL_CONST_CORRECT_SERIALIZE
         (
            CASUAL_SERIALIZE( m_bool);
            CASUAL_SERIALIZE( m_long);
            CASUAL_SERIALIZE( m_string);
            CASUAL_SERIALIZE( m_short);
            CASUAL_SERIALIZE( m_longlong);
            CASUAL_SERIALIZE( m_time);
            CASUAL_SERIALIZE( m_optional);
         )

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
   m_optional: 666
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
         common::Uuid m_uuid = 0x626b039950df4021892e8e7b94806374_uuid;

         CASUAL_CONST_CORRECT_SERIALIZE
         (
            CASUAL_SERIALIZE( m_string);
            CASUAL_SERIALIZE( m_values);
            CASUAL_SERIALIZE( m_tuple);
            CASUAL_SERIALIZE( m_uuid);
         )
      };


      struct Binary : public SimpleVO
      {
         platform::binary::type m_binary;

         CASUAL_CONST_CORRECT_SERIALIZE
         (
            SimpleVO::serialize( archive);
            CASUAL_SERIALIZE( m_binary);
         )
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



            void serialize( common::serialize::Reader& reader);
            void serialize( common::serialize::Writer& writer) const;


         private:
            class Implementation;
            common::basic_pimpl< Implementation> m_pimpl;
         };

      } // pimpl

   } // test
} // casual




