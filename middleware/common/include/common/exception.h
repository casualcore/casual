//!
//! casual_exception.h
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
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


#include <xatmi.h>
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

         namespace queue
         {

            struct Unavailable : base
            {
               using base::base;
            };

         } // queue


         namespace network
         {

            struct Unavailable : base
            {
               using base::base;
            };

         } // network


         namespace signal
         {
            struct base : common::exception::base
            {
               using common::exception::base::base;
            };

            template< common::platform::signal_type signal>
            struct basic_signal : signal::base
            {

               template< typename... Args>
               basic_signal( std::string description, Args&&... information)
                  : signal::base( description, make_nip( "signal", common::signal::type::string( signal)), std::forward< Args>( information)...) {}

               basic_signal()
                  : signal::base( "signal: " + common::signal::type::string( signal)) {}

            };

            typedef basic_signal< common::signal::alarm> Timeout;

            typedef basic_signal< common::signal::terminate> Terminate;

            typedef basic_signal< common::signal::user> User;


            namespace child
            {
               typedef basic_signal< common::signal::child> Terminate;
            } // child

         } // signal

         namespace code
         {

            struct base : common::exception::base
            {
               using common::exception::base::base;

               virtual std::string tag_name() const noexcept = 0;
               virtual long code() const noexcept = 0;
               virtual log::category::Type category() const noexcept = 0;
            };

            template< typename base_tag>
            struct tag_base : base
            {
               using tag = base_tag;

               using base::base;

               template< typename... Args>
               tag_base( std::string description, Args&&... information)
                : base( std::move( description), std::forward< Args>( information)...) {}

               std::string tag_name() const noexcept override
               {
                  return common::type::name< tag>();
               }
            };

            template< long code_value, log::category::Type categor_value, typename base_tag>
            struct basic_code : public tag_base< base_tag>
            {
               using tag_base< base_tag>::tag_base;

               long code() const noexcept override { return code_value;}
               log::category::Type category() const noexcept override { return categor_value;}
            };


            namespace category
            {
               template< log::category::Type categor_value, typename base_tag>
               struct basic_category : tag_base< base_tag>
               {
                  using base_type = tag_base< base_tag>;
                  using base_type::base_type;

                  template< typename... Args>
                  basic_category( long code, std::string description, Args&&... information)
                   : base_type( std::move( description), std::forward< Args>( information)...), m_code( code) {}

                  long code() const noexcept override { return m_code;}
                  log::category::Type category() const noexcept override { return categor_value;}

               private:
                  long m_code;
               };

               struct category_tag {};

               template< typename base_tag>
               using basic_error = basic_category< log::category::Type::error, base_tag>;

               using error = basic_error< category_tag>;

            } // category

         } // code



         namespace xatmi
         {
            struct base_tag {};

            using base = code::tag_base< base_tag>;


            template< long code, log::category::Type category>
            struct basic_xatmi : public code::basic_code< code, category, base_tag>
            {
               using base_type = code::basic_code< code, category, base_tag>;

               template< typename... Args>
               basic_xatmi( const std::string& description, Args&&... information)
                : base_type( error::xatmi::error( code) + " - " + description, std::forward< Args>( information)...) {}

               basic_xatmi()
                  : basic_xatmi( error::xatmi::error( code)) {}
            };

            namespace no
            {
               using Message = basic_xatmi< TPEBLOCK, log::category::Type::debug>;
            } // no

            using Limit = basic_xatmi< TPELIMIT, log::category::Type::information>;


            namespace os
            {
               using Error = basic_xatmi< TPEOS, log::category::Type::error>;
            } // os


            using Protocoll = basic_xatmi< TPEPROTO, log::category::Type::error>;

            namespace invalid
            {
               using Argument = basic_xatmi< TPEINVAL, log::category::Type::debug>;

               using Descriptor = basic_xatmi< TPEBADDESC, log::category::Type::debug>;
            } // invalid

            namespace service
            {
               using Error = basic_xatmi< TPESVCERR, log::category::Type::error>;

               using Fail = basic_xatmi< TPESVCFAIL, log::category::Type::debug>;

               namespace no
               {
                  using Entry = basic_xatmi< TPENOENT, log::category::Type::debug>;
               } // no

               //typedef basic_xatmi< TPENOENT, log::category::Type::user> NoEntry;
               typedef basic_xatmi< TPENOENT, log::category::Type::error> NoEntry;

               typedef basic_xatmi< TPEMATCH, log::category::Type::debug> Advertised;
            }

            using System = basic_xatmi< TPESYSTEM, log::category::Type::error>;

            using Timeout = basic_xatmi< TPETIME, log::category::Type::debug>;

            namespace transaction
            {
               using Support = basic_xatmi< TPETRAN, log::category::Type::debug>;
            } // transaction

            using Signal = basic_xatmi< TPGOTSIG, log::category::Type::information>;

            namespace buffer
            {
               namespace type
               {
                  using Input = basic_xatmi< TPEITYPE, log::category::Type::debug>;

                  using Output = basic_xatmi< TPEOTYPE, log::category::Type::debug>;
               } // type
            }

            namespace conversational
            {
               // TODO: Currently not supported...

              // #define TPEEVENT 22  <-- not supported right now

            }
         } // xatmi

         namespace tx
         {
            struct base_tag {};

            using base = code::tag_base< base_tag>;

            template< long code, log::category::Type category>
            using basic_tx = code::basic_code< code, category, base_tag>;

            using Fail = basic_tx< TX_FAIL, log::category::Type::error>;

            using Error = basic_tx< TX_ERROR, log::category::Type::error>;

            using Protocol = basic_tx< TX_PROTOCOL_ERROR, log::category::Type::error>;

            using Argument = basic_tx< TX_EINVAL, log::category::Type::error>;

            using Outside = basic_tx< TX_OUTSIDE, log::category::Type::error>;

            namespace no
            {
               using Begin = basic_tx< TX_NO_BEGIN, log::category::Type::error>;

               using Support = basic_tx< TX_NOT_SUPPORTED, log::category::Type::debug>;
            } // no
         }

		} // exception
	} // common
} // casual


#define CASUAL_NIP( information) \
   casual::common::exception::make_nip( #information, information)




#endif /* CASUAL_EXCEPTION_H_ */
