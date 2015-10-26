//!
//! exception.cpp
//!
//! Created on: Mar 8, 2015
//!     Author: Lazan
//!

#include "common/exception.h"


namespace casual
{
   namespace common
   {
      namespace exception
      {
         base::base( std::string description) : m_description( std::move( description))
         {

         }

         base::base( std::string description, std::vector< nip_type> information) : base( std::move( description) + " - [ ")
         {
            auto current = std::begin( information);
            for( ; current != std::end( information); ++ current)
            {
               m_description += "{ " + std::get< 0>( *current) + ": " + std::get< 1>( *current) + "}";
               if( current + 1 != std::end( information))
               {
                  m_description += ", ";
               }
            }
            m_description += "]";
         }

         const char* base::what() const noexcept
         {
            return m_description.c_str();
         }

         const std::string& base::description() const noexcept
         {
            return m_description;
         }


         std::ostream& operator << ( std::ostream& out, const base& exception)
         {
            return out << exception.description();
         }

      } // exception
   } // common



} // casual
