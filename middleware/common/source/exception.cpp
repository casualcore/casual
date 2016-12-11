//!
//! casual
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

         namespace
         {
            namespace local
            {
               std::string make_description( std::string description, std::vector< nip_type> information)
               {
                  description += " - [ ";

                  auto current = std::begin( information);
                  for( ; current != std::end( information); ++ current)
                  {
                     description += "{ " + std::get< 0>( *current) + ": " + std::get< 1>( *current) + "}";

                     if( current + 1 != std::end( information))
                     {
                        description += ", ";
                     }
                  }

                  description += "]";

                  return description;
               }

            }
         }


         base::base( std::string description, std::vector< nip_type> information)
         : base( local::make_description( std::move( description), std::move( information)))
         {

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

         namespace xatmi
         {
            void propagate( int error)
            {
               switch( error)
               {
                  case 0: break;
                  case TPEOS: throw os::Error{};
                  case TPEPROTO: throw Protocoll{};
                  case TPESVCERR: throw service::Error{};
                  case TPESYSTEM: throw System{};
                  default:
                  {
                     throw Protocoll{ "unexpected error", CASUAL_NIP( error)};
                  }
               }

            }

         } // xatmi

      } // exception
   } // common



} // casual
