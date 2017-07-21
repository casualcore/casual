//!
//! casual
//!

#ifndef CASUAL_EXCEPTION_H_
#define CASUAL_EXCEPTION_H_



#include "common/platform.h"
#include "common/error.h"

#include "common/string.h"
#include "common/signal.h"
#include "common/log.h"

#include <stdexcept>
#include <string>
#include <ostream>


#include <xatmi/defines.h>
#include <tx.h>

namespace casual
{
   namespace common
   {
      namespace exception
      {
         using nip_type = std::tuple< std::string, std::string>;

         template< typename T>
         nip_type make_nip( std::string name, const T& information)
         {
            return std::make_tuple( std::move( name), to_string( information));
         }

         struct base : std::exception
         {
            base( std::string description);

            template< typename... Args>
            base( std::string description, Args&&... information)
               : base( std::move( description), construct( {}, std::forward<Args>( information)...)) {}


            const char* what() const noexcept override;
            const std::string& description() const noexcept;


            friend std::ostream& operator << ( std::ostream& out, const base& exception);

         private:
            using nip_type = std::tuple< std::string, std::string>;

            struct Convert
            {
               nip_type operator() ( nip_type nip) { return nip;}

               template< typename T>
               nip_type operator() ( T value) { return make_nip( "value", value);}
            };

            base( std::string description, std::vector< nip_type> information);

            static std::vector< nip_type> construct( std::vector< nip_type> nips)
            {
               return nips;
            }

            template< typename I, typename... Args>
            static std::vector< nip_type> construct( std::vector< nip_type> nips, I&& infomation, Args&&... args)
            {
               nips.push_back( Convert{}( std::forward< I>( infomation)));
               return construct( std::move( nips), std::forward< Args>( args)...);
            }

            const std::string m_description;
         };

         //!
         //! something has gone wrong internally in casual.
         //!
         //! @todo another name?
         //!
         struct Casual : base
         {
            using base::base;
         };


         //
         // Serves as a placeholder for later correct exception, with hopefully a good name...
         //
         struct NotReallySureWhatToNameThisException : public base
         {
            NotReallySureWhatToNameThisException( )
               : base( "NotReallySureWhatToNameThisException") {}

            NotReallySureWhatToNameThisException( const std::string& information)
               : base( information) {}
         };


         struct Shutdown : base
         {
            using base::base;
         };

         namespace invalid
         {
            struct base : common::exception::base
            {
               using common::exception::base::base;

            };

            struct Argument : base
            {
               using base::base;
            };

            struct Flags : base
            {
               using base::base;
            };

            //!
            //! Something is used in a way it's not design to support.
            //!
            //!
            struct Semantic : base
            {
               using base::base;
            };

            namespace environment
            {
               struct Variable : Argument
               {
                  using Argument::Argument;
               };

            } // environment

            struct Configuration : base
            {
               using base::base;
            };

            struct Process : base
            {
               using base::base;
            };

            struct File : base
            {
               using base::base;
            };

         } // invalid

         namespace limit
         {
            struct Memory : base
            {
               using base::base;
            };

         } // limit


         namespace communication
         {
            namespace no
            {
               struct Message : base
               {
                  using base::base;
               };
            } // no

            struct Unavailable : base
            {
               using base::base;
            };

            struct Refused : base
            {
               using base::base;
            };

         } // communication

         namespace queue
         {
            using Unavailable = communication::Unavailable;

         } // queue


         namespace signal
         {
            struct base : common::exception::base
            {
               using common::exception::base::base;
            };

            template< common::signal::Type signal>
            struct basic_signal : signal::base
            {

               template< typename... Args>
               basic_signal( std::string description, Args&&... information)
                  : signal::base( description, make_nip( "signal", common::signal::type::string( signal)), std::forward< Args>( information)...) {}

               basic_signal()
                  : signal::base( "signal: " + common::signal::type::string( signal), make_nip( "signal", signal)) {}

            };

            using Timeout = basic_signal< common::signal::Type::alarm>;
            using Terminate = basic_signal< common::signal::Type::terminate>;
            using User = basic_signal< common::signal::Type::user>;
            using Pipe = basic_signal< common::signal::Type::pipe>;

            namespace child
            {
               using Terminate = basic_signal< common::signal::Type::child>;
            } // child

         } // signal

		} // exception
	} // common
} // casual


#define CASUAL_NIP( information) \
   casual::common::exception::make_nip( #information, information)




#endif /* CASUAL_EXCEPTION_H_ */
