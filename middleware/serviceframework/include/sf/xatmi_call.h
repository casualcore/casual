//!
//! casual
//!

#ifndef XATMI_CALL_H_
#define XATMI_CALL_H_

#include "sf/archive/archive.h"
#include "sf/archive/binary.h"
#include "sf/buffer.h"
#include "sf/log.h"


#include "common/move.h"

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


            void call( const std::string& service, buffer::Buffer& input, buffer::Buffer& output, long flags);


            call_descriptor_type send( const std::string& service, buffer::Buffer& input, long flags);

            bool receive( call_descriptor_type& callDescriptor, buffer::Buffer& output, long flags);


            void cancel( call_descriptor_type cd);




            namespace internal
            {
               struct xatmi_call
               {

                  void operator() ( const std::string& service, buffer::Buffer& input, buffer::Buffer& output, long flags) const
                  {
                     call( service, input, output, flags);
                  }
               };

               struct xatmi_send_receive
               {

                  call_descriptor_type operator() ( const std::string& service, buffer::Buffer& input, long flags) const
                  {
                     return send( service, input, flags);
                  }

                  bool operator() ( call_descriptor_type& cd, buffer::Buffer& output, long flags) const
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
               basic_result( policy_type&& policy) : m_policy{ std::move( policy)}
               {
                  sf::log::sf << "basic_result buffer: " << m_policy.buffer() << '\n';
               }

               template< typename T>
               basic_result& operator >> ( T&& value)
               {
                  m_policy.archive() >> std::forward< T>( value);
                  return *this;
               }

               policy_type& policy()
               {
                  return m_policy;
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
                     sf::Trace trace{ "service::sync::basic_call::operator()"};

                     log::sf << "buffer: " << m_policy.buffer() << '\n';

                     result_policy resultPolicy;
                     caller_type caller;

                     caller( m_service, m_policy.buffer(), resultPolicy.buffer(), m_flags);

                     m_policy.buffer().clear();

                     log::sf  << "output: " << resultPolicy.buffer() << '\n';

                     return result_type{ std::move( resultPolicy)};
                  }
                  template< typename T>
                  basic_call& operator << ( T&& value)
                  {
                     m_policy.archive() << std::forward< T>( value);
                     return *this;
                  }

                  send_policy& policy()
                  {
                     return m_policy;
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
                     if( ! m_moved && m_callDescriptor != 0)
                     {
                        m_caller.cancel( m_callDescriptor);
                     }
                  }

                  basic_receive( basic_receive&&) = default;
                  basic_receive& operator = ( basic_receive&&) = default;


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

                  common::move::Moved m_moved;

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


                  send_policy& policy()
                  {
                     return m_policy;
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
                     archive_holder( archive_holder&& other)
                        : m_buffer( std::move( other.m_buffer)), m_archive( m_buffer) {}

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
                  enum Default
                  {
                     flags = 0L
                  };

                  using send_policy = helper::archive_holder< archive::binary::Writer, buffer::binary::Stream>;
                  using result_policy = helper::archive_holder< archive::binary::Reader, buffer::binary::Stream>;
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
