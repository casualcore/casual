//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "common/communication/message.h"

namespace casual
{

   namespace common
   {

      namespace communication
      {

         namespace message
         {
            namespace complete
            {

               namespace host
               {
                  namespace
                  {
                     namespace header
                     {
                        auto type( const network::Header& value)
                        {
                           return static_cast< common::message::Type>( common::network::byteorder::decode<network::Header::host_type_type>( value.type));
                        }

                        auto size( const network::Header& value)
                        {
                           return common::network::byteorder::size::decode<network::Header::host_size_type>( value.size);
                        }

                     } // header
                  } // <unnamed>
               } // host

               namespace network
               {
                  std::ostream& operator << ( std::ostream& out, const Header& value)
                  {
                     return out  <<
                        "{ type: " << host::header::type( value) <<
                        ", correlation: " << uuid::string( value.correlation) <<
                        ", size: " << host::header::size( value) <<
                        '}';
                  }

               } // network

            } // complete

            Complete::Complete() = default;

            Complete::Complete( message_type_type type, const Uuid& correlation) : type{ type}, correlation{ correlation} {}

            Complete::Complete( const complete::network::Header& header)
              : type{ complete::host::header::type( header)}, correlation{ header.correlation}
            {
               payload.resize( complete::host::header::size( header));
            }

            complete::network::Header Complete::header() const
            {
               complete::network::Header header;

               correlation.copy( header.correlation);
               header.type = network::byteorder::encode( cast::underlying( type));
               header.size = network::byteorder::size::encode( payload.size());

               return header;
            }


            Complete::Complete( Complete&& rhs) noexcept
            {
               swap( *this, rhs);
            }

            Complete& Complete::operator = ( Complete&& rhs) noexcept
            {
               swap( *this, rhs);
               return *this;
            }


            Complete::operator bool() const
            {
               return type != message_type_type::absent_message;
            }

            bool Complete::complete() const { return m_unhandled.empty();}

            void swap( Complete& lhs, Complete& rhs)
            {
               using std::swap;
               swap( lhs.correlation, rhs.correlation);
               swap( lhs.payload, rhs.payload);
               swap( lhs.type, rhs.type);
               swap( lhs.m_unhandled, rhs.m_unhandled);
            }

            std::ostream& operator << ( std::ostream& out, const Complete& value)
            {
               return out << "{ type: " << value.type << ", correlation: " << value.correlation << ", size: "
                     << value.payload.size() << std::boolalpha << ", complete: " << value.complete() << '}';
            }

            bool operator == ( const Complete& complete, const Uuid& correlation)
            {
               return complete.correlation == correlation;
            }

         } // message

      } // communication

   } // common


} // casual
