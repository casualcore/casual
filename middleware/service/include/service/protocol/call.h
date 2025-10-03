//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "service/call.h"

#include "common/serialize/binary.h"
#include "common/string.h"

namespace casual
{
   namespace service::protocol
   {
      inline namespace v1
      {
         namespace detail
         {
            template< typename Result, typename Policy>
            struct basic_result : Result
            {
               using result_type = Result;
               using policy_type = Policy;

               basic_result( basic_result&&) = default;

               basic_result( result_type&& result)
                  : result_type{ std::move( result)}, m_policy{ *this}
               {
               }

               template< typename T>
               basic_result& operator >> ( T&& value)
               {
                  m_policy.archive() >> std::forward< T>( value);
                  return *this;
               }
               
               template< typename T>
               auto extract()
               {
                  T result;
                  m_policy.archive() >> result;
                  return result;
               }

               template< typename T>
               auto extract( std::string_view name)
               {
                  T result;
                  m_policy.archive() >> CASUAL_NAMED_VALUE_NAME( result, name.data());
                  return result;
               }

            private:
               policy_type m_policy;
            };

            template< typename I, typename R>
            struct basic_call
            {
               using input_policy = I;
               using result_policy = R;
               using result_type = basic_result< service::call::Result, result_policy>;
               using Complement = service::call::Complement;

               basic_call() : m_payload( input_policy::type())
               {}

               template< typename T>
               basic_call& operator << ( T&& value)
               {
                  m_input.archive << std::forward< T>( value);
                  return *this;
               }

               //! calls the `service`
               //! @returns service reply in a form of `basic_result`
               result_type operator () ( common::string::Argument service)
               {
                  m_input.archive.consume( m_payload.data);
                  return service::call::invoke( std::move( service), m_payload);
               }

               //! calls the `service` with 0..* argument   s. If the first argument is `service::call::Complement`
               //! it will be used as complement to the call, and not part of the payload.
               //! @returns service reply in a form of `basic_result`
               template< typename Arg, typename... Args>
               result_type operator () ( common::string::Argument service, const Arg& arg, const Args&... args)
               {
                  if constexpr( std::is_same_v< std::decay_t< Arg>, Complement>)
                  {
                     // `arg` is a `Complement`, so we don't add it to the payload
                     ( ( m_input.archive << args), ...);

                     m_input.archive.consume( m_payload.data);
                     return service::call::invoke( std::move( service), m_payload, arg);
                  }
                  else
                  {
                     // `arg` is not a `Complement`, so we add it to the payload
                     m_input.archive << arg;
                     ( ( m_input.archive <<  args), ...);

                     m_input.archive.consume( m_payload.data);
                     return service::call::invoke( std::move( service), m_payload);
                  }
               }

            private:

               common::buffer::Payload m_payload;
               input_policy m_input;

            };

            template< typename R>
            struct basic_receive
            {
               using result_policy = R;
               using result_type = basic_result< service::receive::Result, result_policy>;
               using Flag = service::receive::Flag;

               basic_receive( common::strong::correlation::id correlation) : m_correlation( correlation) {}

               result_type operator () () const
               {
                  return { service::receive::invoke( m_correlation)};
               }

               result_type operator () ( Flag flags) const
               {
                  return { service::receive::invoke( m_correlation, flags)};
               }


            private:
               common::strong::correlation::id m_correlation;
            };

            template< typename I, typename R>
            struct basic_send
            {
               using input_policy = I;
               using result_policy = R;
               using receive_type = basic_receive< result_policy>;
               using Complement = service::send::Complement;

               basic_send() : m_payload( input_policy::type()) {} //, m_input( m_payload) {}

               template< typename T>
               basic_send& operator << ( T&& value)
               {
                  m_input.archive() << std::forward< T>( value);
                  return *this;
               }

               //! calls the `service`
               //! @returns service reply in a form of `basic_result`
               receive_type operator () ( common::string::Argument service)
               {
                  m_input.archive.consume( m_payload.data);
                  return service::send::invoke( std::move( service), m_payload);
               }

               //! calls the `service` with 0..* argument   s. If the first argument is `service::call::Complement`
               //! it will be used as complement to the call, and not part of the payload.
               //! @returns service reply in a form of `basic_result`
               template< typename Arg, typename... Args>
               receive_type operator () ( common::string::Argument service, const Arg& arg, const Args&... args)
               {
                  if constexpr( std::is_same_v< std::decay_t< Arg>, Complement>)
                  {
                     // `arg` is a `Complement`, so we don't add it to the payload
                     ( ( m_input.archive << args), ...);

                     m_input.archive.consume( m_payload.data);
                     return service::send::invoke( std::move( service), m_payload, arg);
                  }
                  else
                  {
                     // `arg` is not a `Complement`, so we add it to the payload
                     m_input.archive << arg;
                     ( ( m_input.archive <<  args), ...);

                     m_input.archive.consume( m_payload.data);
                     return service::send::invoke( std::move( service), m_payload);
                  }
               }

            private:
               common::buffer::Payload m_payload;
               input_policy m_input;
            };

         } // detail

         namespace binary
         {
            namespace policy
            {
               struct Input
               {
                  static constexpr auto type() { return common::buffer::type::binary;};
                  common::serialize::Writer archive = common::serialize::binary::writer();
               };

               struct Result
               {
                  template< typename R>
                  Result( R& result) : m_archive( common::serialize::binary::reader( result.buffer.data)) {}

                  common::serialize::Reader& archive() { return m_archive;}

               private:
                  common::serialize::Reader m_archive;

               };
            } // policy

            using Call = detail::basic_call< policy::Input, policy::Result>;
            using Send = detail::basic_send< policy::Input, policy::Result>;

         } // binary
      } // v1

   } // service::protocol
} // casual


