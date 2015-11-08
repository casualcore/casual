//!
//! test_vo.cpp
//!
//! Created on: Dec 21, 2013
//!     Author: Lazan
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
            Implementation() {}

            Implementation( long value) : m_long( value) {}

            long m_long;
            std::string m_string;
            short m_short;
            long long m_longlong;
            sf::platform::time_point m_time;

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
            return m_pimpl.implementation().m_long;
         }

         const std::string& Simple::getString() const
         {
            return m_pimpl.implementation().m_string;
         }


         void Simple::serialize( sf::archive::Reader& reader)
         {
            m_pimpl.implementation().serialize( reader);
         }

         std::string& Simple::getString()
         {
            return m_pimpl.implementation().m_string;
         }

         void Simple::setLong( long value)
         {
            m_pimpl.implementation().m_long = value;
         }

         void Simple::setString( const std::string& value)
         {
            m_pimpl.implementation().m_string = value;
         }



         void Simple::serialize( sf::archive::Writer& writer) const
         {
            m_pimpl.implementation().serialize( writer);
         }


      } // pimpl


   } // test

	TEST( casual_test_vo, pimpl_Simple_instanciation)
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
