//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/serialize/macro.h"

namespace casual
{
   namespace serviceframework
   {
      namespace archive
      {
         class Reader;
         class Writer;
      }

   } // serviceframework

   namespace configuration
   {
      template< typename V>
      struct Type
      {
         using value_type = V;

         Type() = default;

         template< typename... Args>
         Type( Args&&... args) : m_value{ std::forward< Args>( args)...}, m_assigned{ true} {}

         Type( const Type&) = default;
         Type& operator = ( const Type&) = default;
         Type( Type&&) = default;
         Type& operator = ( Type&&) = default;

         template< typename T>
         Type& operator = ( T&& value)
         {
            m_value = std::forward< T>( value);
            m_assigned = true;
            return *this;
         }


         operator value_type() { return m_value;}
         operator const value_type&() const { return m_value;}

         bool assigned() const { return m_assigned;}

         const value_type& value() const { return m_value;}

      private:
         value_type m_value;
         bool m_assigned = false;
      };

      template< typename V>
      void serialize( serviceframework::archive::Reader& archive, Type< V>& value, const char* name)
      {

      }

      template< typename V>
      void serialize( serviceframework::archive::Writer& archive, Type< V>& value, const char* name)
      {
         if( value.assigned())
         {
            archive << serviceframework::makeNameValuePair( name, value.value());
         }
      }


   } // config




} // casual


