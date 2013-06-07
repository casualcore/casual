//!
//! xatmi_call.h
//!
//! Created on: May 25, 2013
//!     Author: Lazan
//!

#ifndef XATMI_CALL_H_
#define XATMI_CALL_H_

#include "sf/archive.h"
#include "sf/archive_binary.h"
#include "sf/buffer.h"


//
// xatmi
//
#include <xatmi.h>

//
// std
//
#include <string>
#include <vector>
#include <type_traits>

namespace casual
{
   namespace sf
   {
      namespace xatmi
      {




         namespace service
         {
            typedef int call_descriptor_type;
            constexpr long valid_sync_flags();
            /*

            */

            void call( const std::string& service, buffer::Base& input, buffer::Base& output, long flags);

            call_descriptor_type send( const std::string& service, buffer::Base& input, long flags);

            bool receive( call_descriptor_type& callDescriptor, buffer::Base& output, long flags);




            namespace internal
            {
               struct xatmi_call
               {
                  void operator() ( const std::string& service, buffer::Base& input, buffer::Base& output, long flags) const
                  {
                     call( service, input, output, flags);
                  }
               };

               struct xatmi_send_receive
               {
                  call_descriptor_type operator() ( const std::string& service, buffer::Base& input, long flags) const
                  {
                     return send( service, input, flags);
                  }

                  bool operator() ( call_descriptor_type& cd, buffer::Base& output, long flags) const
                  {
                     return receive( cd, output, flags);
                  }
               };

               //typedef std::function< void( const std::string&, buffer::Base&, buffer::Base&, long)> xatmi_c;

            } // internal

            template< typename P>
            class basic_reply
            {
            public:
               typedef P policy_type;

               basic_reply( policy_type& policy) : m_policy( policy) {}

               template< typename T>
               basic_reply& operator >> ( T&& value)
               {
                  m_policy.reader().archive() >> std::forward< T>( value);
                  return *this;
               }
            private:
               policy_type& m_policy;
            };


            template< typename P, long flags = 0, typename C = internal::xatmi_call>
            class basic_sync
            {
            public:
               //static_assert( flags & ( ~valid_sync_flags()) == 0, "Invalid flags");

               typedef P policy_type;
               typedef C caller_type;
               typedef basic_reply< policy_type> reply_type;


               basic_sync( const std::string& name) : m_service( name) {}

               reply_type call()
               {
                  caller_type caller;

                  caller( m_service, m_policy.writer().buffer(), m_policy.reader().buffer(), flags);

                  return reply_type( m_policy);
               }
               template< typename T>
               basic_sync& operator << ( T&& value)
               {
                  m_policy.writer().archive() << std::forward< T>( value);
                  return *this;
               }

            private:
               const std::string m_service;
               policy_type m_policy;
            };

            template< typename P, long flags = 0, typename C = internal::xatmi_send_receive>
            class basic_async
            {
            public:
               //static_assert( flags & ( ~valid_sync_flags()) == 0, "Invalid flags");

               typedef P policy_type;
               typedef C caller_type;
               typedef basic_reply< policy_type> reply_type;


               basic_async( const std::string& name) : m_service( name) {}

            private:

               //!
               //! Internal stuff to get different return types depdendent on flags
               //! We have to declare these before the instantiation...
               //! @{
               template< long currentFlags>
               typename std::enable_if< ( currentFlags & TPNOBLOCK) == TPNOBLOCK, std::vector< reply_type>>::type
               flag_dependent_receive()
               {
                  caller_type caller;
                  std::vector< reply_type> result;
                  if( caller( m_callDescriptor, m_policy.reader().buffer(), flags))
                  {
                     result.emplace_back( m_policy);
                  }
                  return result;
               }

               template< long currentFlags>
               typename std::enable_if< ( currentFlags & TPNOBLOCK) != TPNOBLOCK, reply_type>::type
               flag_dependent_receive()
               {
                  caller_type caller;
                  if( ! caller( m_callDescriptor, m_policy.reader().buffer(), flags))
                  {
                     // TODO: something is broken in the xatmi-part...
                     throw exception::NotReallySureWhatToCallThisExcepion();
                  }
                  return reply_type( m_policy);
               }
               //! @}

            public:

               void send()
               {
                  caller_type caller;

                  m_callDescriptor = caller( m_service, m_policy.writer().buffer(), flags);
               }

               auto receive() -> decltype( this->flag_dependent_receive< flags>())
               {
                  return flag_dependent_receive< flags>();
               }

               template< typename T>
               basic_async& operator << ( T&& value)
               {
                  m_policy.writer().archive() << std::forward< T>( value);
                  return *this;
               }

            private:

               const std::string m_service;
               policy_type m_policy;
               call_descriptor_type m_callDescriptor = 0;
            };


            namespace policy
            {
               namespace helper
               {
                  template< typename A, typename B>
                  struct archive_holder
                  {
                     archive_holder() : m_archive( m_buffer) {}

                     A& archive() { return m_archive;}
                     B& buffer() { return m_buffer;}

                  private:
                     B m_buffer;
                     A m_archive;
                  };
               }

               struct Binary
               {
               public:

                  Binary() = default;
                  Binary( const Binary&) = delete;
                  Binary( Binary&&) = default;

                  typedef helper::archive_holder< archive::binary::Writer, buffer::Binary> writer_type;
                  typedef helper::archive_holder< archive::binary::Reader, buffer::Binary> reader_type;

                  writer_type& writer() { return m_writer;}
                  reader_type& reader() { return m_reader;}


               private:
                  writer_type m_writer;
                  reader_type m_reader;

               };

               /*
               struct YAML
               {
                  YAML() : m_writer( m_writerBuffer) {}


                  archive::Writer& writer();


               private:

                  archive::yaml::writer::Strict::implementation_type::buffer_type m_writerBuffer;
                  archive::yaml::writer::Strict m_writer;

                  archive::yaml::reader::Strict::implementation_type::buffer_type m_readerBuffer;
                  archive::yaml::reader::Strict m_reader;
               };
               */
            }

         } // service

      } // xatmi
   } // sf
} // casual



#endif /* XATMI_CALL_H_ */
