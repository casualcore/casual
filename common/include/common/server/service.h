//!
//! service.h
//!
//! Created on: Oct 20, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_SERVER_SERVICE_H_
#define CASUAL_COMMON_SERVER_SERVICE_H_


#include "xatmi.h"

#include <functional>
#include <string>
#include <ostream>
#include <cstdint>

namespace casual
{
   namespace common
   {
      namespace server
      {
         struct Service
         {
            enum Type : std::uint64_t
            {
               cXATMI = 0,
               cCasualAdmin = 10,
               cCasualSF = 11,
            };



            enum class Transaction : std::uint64_t
            {
               //! if there is a transaction join it, if not, start a new one
               automatic = 0,
               //! if there is a transaction join it, if not, execute outside transaction
               join = 1,
               //! Regardless - start a new transaction
               atomic = 2,
               //! Regardless - execute outside transaction
               none = 3
            };


            using function_type = std::function< void( TPSVCINFO *)>;
            using adress_type = void(*)( TPSVCINFO *);

            Service( std::string name, function_type function, std::uint64_t type, Transaction transaction);
            Service( std::string name, function_type function);

            Service( Service&&);


            void call( TPSVCINFO* serviceInformation);

            std::string name;
            function_type function;

            std::uint64_t type = Type::cXATMI;
            Transaction transaction = Transaction::automatic;
            bool active = true;

            friend std::ostream& operator << ( std::ostream& out, const Service& service);

            friend bool operator == ( const Service& lhs, const Service& rhs);
            friend bool operator != ( const Service& lhs, const Service& rhs);

         private:
            adress_type m_adress;
         };

         namespace service
         {
            namespace transaction
            {
               constexpr std::uint64_t mode( Service::Transaction mode)
               {
                  return static_cast< std::uint64_t>( mode);
               }
               constexpr Service::Transaction mode( std::uint64_t mode)
               {
                  return Service::Transaction( mode);
               }
               Service::Transaction mode( const std::string& mode);

            } // transaction

         } // service

      } // server
   } // common
} // casual

#endif // SERVICE_H_
