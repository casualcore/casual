//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "serviceframework/service/call.h"

#include "common/serialize/binary.h"
#include "common/string.h"

namespace casual
{
   namespace serviceframework::service::protocol
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
               using Flag = service::call::Flag;
               using Flags = service::call::Flags;

               basic_call() : m_payload( input_policy::type())
               {
               }

               template< typename T>
               basic_call& operator << ( T&& value)
               {
                  m_input.archive << std::forward< T>( value);
                  return *this;
               }

               //! calls the `service` with 0..* arguments. If the first argument is `service::call::Flags` it will 
               //! be used as flags to the call, and not part of the payload.
               //! @returns the result...
               template< typename... Args>
               auto operator () ( common::string::Argument service, Args&&... args)
               {
                  return call( std::move( service), common::traits::priority::tag< 1>{}, std::forward< Args>( args)...);
               }


            private:
               template< typename Arg, typename... Args>
               auto call( common::string::Argument service, common::traits::priority::tag< 1>, Arg flags, Args&&... args)
                  -> decltype( result_type{ service::call::invoke( std::move( service), std::declval< service::payload_type&>(), flags)})
               {
                  ( ( m_input.archive << std::forward< Args>( args)), ...);
                  m_input.archive.consume( m_payload.data);
                  return result_type{ service::call::invoke( std::move( service), m_payload, flags)};
               }

               template< typename... Args>
               auto call( common::string::Argument service, common::traits::priority::tag< 0>, Args&&... args)
                  -> decltype( result_type{ service::call::invoke( std::move( service), std::declval< service::payload_type&>())})
               {
                  ( ( m_input.archive << std::forward< Args>( args)), ...);
                  m_input.archive.consume( m_payload.data);
                  return { service::call::invoke( std::move( service), m_payload)};
               }

               service::payload_type m_payload;
               input_policy m_input;

            };

            template< typename R>
            struct basic_receive
            {
               using descriptor_type = platform::descriptor::type;
               using result_policy = R;
               using result_type = basic_result< service::receive::Result, result_policy>;
               using Flag = service::receive::Flag;
               using Flags = service::receive::Flags;

               basic_receive( descriptor_type descriptor) : m_descriptor( descriptor) {}

               result_type operator () () const
               {
                  return { service::receive::invoke( m_descriptor)};
               }

               result_type operator () ( Flags flags) const
               {
                  return { service::receive::invoke( m_descriptor, flags)};
               }


            private:
               platform::descriptor::type m_descriptor;
            };

            template< typename I, typename R>
            struct basic_send
            {
               using input_policy = I;
               using result_policy = R;
               using receive_type = basic_receive< result_policy>;
               using Flag = service::send::Flag;
               using Flags = service::send::Flags;


               basic_send() : m_payload( input_policy::type()), m_input( m_payload) {}

               template< typename T>
               basic_send& operator << ( T&& value)
               {
                  m_input.archive() << std::forward< T>( value);
                  return *this;
               }

               receive_type operator () ( const std::string& service)
               {
                  return { service::send::invoke( service, m_payload)};
               }

               receive_type operator () ( const std::string& service, Flags flags)
               {
                  return { service::send::invoke( service, m_payload, flags)};
               }

            private:
               service::payload_type m_payload;
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

   } // serviceframework::service::protocol
} // casual


