//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>



#include "../include/test_vo.h"


namespace casual
{
   namespace test
   {
      namespace pimpl
      {
         class Simple::Implementation
         {
         public:
            Implementation() = default;

            Implementation( long value) : m_long( value) {}

            long m_long = 0;
            std::string m_string;
            short m_short = 0;
            long long m_longlong = 0;
            serviceframework::platform::time::point::type m_time;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( m_long);
               archive & CASUAL_MAKE_NVP( m_string);
               archive & CASUAL_MAKE_NVP( m_short);
               archive & CASUAL_MAKE_NVP( m_longlong);
               archive & CASUAL_MAKE_NVP( m_time);
            }
         };

         Simple::Simple( long value) : m_pimpl( value) {}

         Simple::Simple() = default;
         Simple::~Simple() = default;
         Simple::Simple( const Simple&) = default;
         Simple& Simple::operator = ( const Simple&) = default;
         Simple::Simple( Simple&&) noexcept = default;
         Simple& Simple::operator = ( Simple&&) noexcept = default;


         long Simple::getLong() const
         {
            return m_pimpl->m_long;
         }

         const std::string& Simple::getString() const
         {
            return m_pimpl->m_string;
         }


         void Simple::serialize( serviceframework::archive::Reader& reader)
         {
            m_pimpl->serialize( reader);
         }

         std::string& Simple::getString()
         {
            return m_pimpl->m_string;
         }

         void Simple::setLong( long value)
         {
            m_pimpl->m_long = value;
         }

         void Simple::setString( const std::string& value)
         {
            m_pimpl->m_string = value;
         }



         void Simple::serialize( serviceframework::archive::Writer& writer) const
         {
            m_pimpl->serialize( writer);
         }


      } // pimpl


   } // test

   TEST( casual_test_vo, pimpl_Simple_instantiation)
   {
      test::pimpl::Simple simple;

      simple.setLong( 42);

      EXPECT_TRUE( simple.getLong() == 42);
   }


   TEST( casual_test_vo, pimpl_Simple_copy_ctor)
   {
      test::pimpl::Simple first( 42);

      test::pimpl::Simple second{ first};

      EXPECT_TRUE( second.getLong() == 42);
   }

   TEST( casual_test_vo, pimpl_Simple_copy_assignment)
   {
      test::pimpl::Simple first( 42);

      test::pimpl::Simple second = first;

      EXPECT_TRUE( second.getLong() == 42);
   }

   TEST( casual_test_vo, pimpl_Simple_move_ctor)
   {
      test::pimpl::Simple first( 42);

      test::pimpl::Simple second{ std::move( first)};

      EXPECT_TRUE( second.getLong() == 42);
   }


   TEST( casual_test_vo, pimpl_Simple_move_assignment)
   {
      test::pimpl::Simple first( 42);


      test::pimpl::Simple second = std::move( first);

      EXPECT_TRUE( second.getLong() == 42);
   }


} // casual
