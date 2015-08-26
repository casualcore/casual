/*
 * tcp.h
 *
 *  Created on: Feb 9, 2015
 *      Author: kristone
 */

#ifndef COMMON_INCLUDE_COMMON_NETWORK_TCP_H_
#define COMMON_INCLUDE_COMMON_NETWORK_TCP_H_

#include <string>

#include "common/move.h"
#include "common/platform.h"

namespace casual
{

   namespace common
   {

      namespace network
      {

         namespace tcp
         {
            class Socket
            {
            public:

               explicit Socket( int descriptor);
               ~Socket();

               Socket( Socket&&) noexcept;
               Socket& operator = ( Socket&&) noexcept;

               explicit operator bool () const noexcept;

               int descriptor() const noexcept;

            private:

               int m_descriptor;
               move::Moved m_moved;

            };

            class Session
            {
            public:

               explicit Session( int descriptor);
               ~Session();

               Session( Session&&) noexcept;
               Session& operator = ( Session&&) noexcept;

               void push( const platform::binary_type& data) const;
               void pull( platform::binary_type& data) const;
               platform::binary_type pull() const;

            private:

               Socket m_socket;

            };


            class Client
            {

            public:

               Client( const std::string& host, const std::string& port);
               ~Client();

               Session session() const;

            private:

               Socket m_socket;

            };


            class Server
            {

            public:

               explicit Server( const std::string& port);
               ~Server();

               Session session() const;

            private:

               Socket m_socket;

            };


         } // tcp

      } // network

   } // common

} // casual



#endif /* COMMON_INCLUDE_COMMON_NETWORK_TCP_H_ */
