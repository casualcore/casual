//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_TYPE_H_
#define CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_TYPE_H_

#include "sf/namevaluepair.h"

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         class Reader;
         class Writer;
      }

   } // sf

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
      void serialize( sf::archive::Reader& archive, Type< V>& value, const char* name)
      {

      }

      template< typename V>
      void serialize( sf::archive::Writer& archive, Type< V>& value, const char* name)
      {
         if( value.assigned())
         {
            archive << sf::makeNameValuePair( name, value.value());
         }
      }


   } // config




} // casual

#endif // CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_TYPE_H_
