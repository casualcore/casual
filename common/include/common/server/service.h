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

namespace casual
{
   namespace common
   {
      namespace server
      {
         struct Service
         {
            enum Type
            {
               cXATMI = 0,
               cCasualAdmin = 4200,
               cCasualSF = 4201,
            };


            enum TransactionType
            {
               //! if there is a transaction join it, if not, start a new one
               cAuto = 0,
               //! if there is a transaction join it, if not, execute outside transaction
               cJoin = 1,
               //! Regardless - start a new transaction
               cAtomic = 2,
               //! Regardless - execute outside transaction
               cNone = 3
            };

            using function_type = std::function< void( TPSVCINFO *)>;
            using adress_type = void(*)( TPSVCINFO *);


            Service( std::string name, function_type function, long type, TransactionType transaction)
               : name( std::move( name)), function( function), type( type), transaction( transaction), m_adress( *function.target< adress_type>()) {}

            Service( std::string name, function_type function)
               : Service( std::move( name), std::move( function), 0, TransactionType::cAuto) {}


            Service( Service&&) = default;


            void call( TPSVCINFO* serviceInformation)
            {
               function( serviceInformation);
            }

            std::string name;
            function_type function;

            long type = 0;
            TransactionType transaction = TransactionType::cAuto;
            bool active = true;

            friend std::ostream& operator << ( std::ostream& out, const Service& service)
            {
               return out << "{name: " << service.name << " type: " << service.type << " transaction: " << service.transaction
                     << " active: " << service.active << "};";
            }

            friend bool operator == ( const Service& lhs, const Service& rhs) { return lhs.m_adress == rhs.m_adress;}
            friend bool operator != ( const Service& lhs, const Service& rhs) { return lhs.m_adress != rhs.m_adress;}

         private:
            adress_type m_adress;
         };
      } // server
   } // common
} // casual

#endif // SERVICE_H_
