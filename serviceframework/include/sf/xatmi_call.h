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

// TODO: Temp
#include "common/trace.h"

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

            void call( const std::string& service, buffer::Base& input, buffer::Base& output, long flags);


            call_descriptor_type send( const std::string& service, buffer::Base& input, long flags);

            bool receive( call_descriptor_type& callDescriptor, buffer::Base& output, long flags);

            void cancel( call_descriptor_type cd);




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

                  void cancel( call_descriptor_type cd)
                  {
                     service::cancel( cd);
                  }

               };

            } // internal


            template< typename P>
            class basic_result
            {
            public:
               typedef P policy_type;

               basic_result( basic_result&&) = default;
               basic_result( policy_type&& policy) : m_policy{ std::move( policy)} {}

               template< typename T>
               basic_result& operator >> ( T&& value)
               {
                  m_policy.archive() >> std::forward< T>( value);
                  return *this;
               }
            private:

               policy_type m_policy;
            };

            namespace sync
            {

               template< typename P, typename C = internal::xatmi_call>
               class basic_call
               {
               public:

                  typedef P policy_type;
                  typedef C caller_type;

                  typedef typename policy_type::send_policy send_policy;
                  typedef typename policy_type::result_policy result_policy;

                  typedef basic_result< result_policy> result_type;


                  basic_call( std::string service, long flags) : m_service{ std::move( service)}, m_flags{ flags} {}
                  basic_call( std::string service) : basic_call( std::move( service),policy_type::flags) {}

                  result_type operator () ()
                  {
                     result_policy resultPolicy;
                     caller_type caller;

                     caller( m_service, m_policy.buffer(), resultPolicy.buffer(), m_flags);

                     m_policy.buffer().clear();

                     return result_type{ std::move( resultPolicy)};
                  }
                  template< typename T>
                  basic_call& operator << ( T&& value)
                  {
                     m_policy.archive() << std::forward< T>( value);
                     return *this;
                  }

               private:
                  const std::string m_service;
                  const long m_flags;
                  send_policy m_policy;
               };

            } // sync

            namespace async
            {
               template< typename P, typename C>
               class basic_receive
               {
               public:

                  typedef P policy_type;
                  typedef basic_result< policy_type> result_type;
                  typedef C caller_type;

                  ~basic_receive()
                  {
                     if( m_callDescriptor != 0)
                     {
                        m_caller.cancel( m_callDescriptor);
                     }
                  }


                  //!
                  //! Blocking get
                  //! @return 1 result
                  //!
                  result_type operator () ()
                  {
                     policy_type policy;

                     if( ! m_caller( m_callDescriptor, policy.buffer(), m_flags & ~TPNOBLOCK))
                     {
                        // TODO: something is broken in the xatmi-part...
                        throw exception::NotReallySureWhatToCallThisExcepion( "caller returns false when TPNOBLOCK is not set");
                     }

                     m_callDescriptor = 0;

                     return result_type{ std::move( policy)};
                  }

                  //!
                  //! Non blocking get.
                  //! @return 0..1 results
                  //!
                  std::vector< result_type> receive()
                  {
                     policy_type policy;

                     std::vector< result_type> result;

                     if( m_caller( m_callDescriptor, policy.buffer(), m_flags | TPNOBLOCK))
                     {
                        result.push_back( result_type{ std::move( policy)});
                     }
                     return result;
                  }

               private:

                  template< typename T1, typename T2>
                  friend class basic_call;

                  basic_receive( call_descriptor_type cd, long flags) : m_callDescriptor( cd), m_flags( flags) {}

                  call_descriptor_type m_callDescriptor;
                  long m_flags;
                  caller_type m_caller;
               };


               template< typename P, typename C = internal::xatmi_send_receive>
               class basic_call
               {
               public:
                  typedef P policy_type;
                  typedef C caller_type;
                  typedef typename policy_type::send_policy send_policy;
                  typedef typename policy_type::result_policy result_policy;
                  typedef basic_receive< result_policy, caller_type> receive_type;
                  typedef typename receive_type::result_type result_type;



                  basic_call( std::string service, long flags) : m_service{ std::move( service)}, m_flags{ flags} {}
                  basic_call( std::string service) : basic_call( std::move( service),policy_type::flags) {}


                  template< typename T>
                  basic_call& operator << ( T&& value)
                  {
                     m_policy.archive() << std::forward< T>( value);
                     return *this;
                  }

                  receive_type operator () ()
                  {
                     caller_type sender;

                     receive_type reply{ sender( m_service, m_policy.buffer(), m_flags), m_flags};

                     m_policy.buffer().clear();

                     return reply;
                  }

               private:
                  const std::string m_service;
                  const long m_flags;
                  send_policy m_policy;

               };
            } // async

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

               namespace binary
               {
                  using Send = helper::archive_holder< archive::binary::Writer, buffer::Binary>;

                  using Result = helper::archive_holder< archive::binary::Reader, buffer::Binary>;

               } // binary

               struct Binary
               {
               public:
                  enum Default
                  {
                     flags = 0L
                  };

                  using send_policy = binary::Send;
                  using result_policy = binary::Result;
               };
            } // policy

            namespace binary
            {
               typedef sync::basic_call< policy::Binary> Sync;
               typedef async::basic_call< policy::Binary> Async;
            }

         } // service

      } // xatmi
   } // sf
} // casual



#endif /* XATMI_CALL_H_ */
