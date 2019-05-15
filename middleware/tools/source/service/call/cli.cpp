//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#define CASUAL_NO_XATMI_UNDEFINE

#include "tools/service/call/cli.h"

#include "common/argument.h"
#include "common/optional.h"
#include "common/service/call/context.h"
#include "common/buffer/type.h"
#include "common/exception/handle.h"
#include "common/transaction/context.h"
#include "common/execute.h"


#include "service/manager/admin/api.h"


#include <stdexcept>
#include <iostream>
#include <string>
#include <map>
#include <cstring>

#include "xatmi.h"

namespace casual 
{
   using namespace common;

   namespace tools 
   {
      namespace service
      {
         namespace call
         {
            namespace local
            {
               namespace
               {
                  struct Settings 
                  {
                     std::string service;
                     bool transaction = false;
                     bool asynchronous = false;
                     platform::size::type iterations = 1;

                     bool stream() const { return iterations == 1 && ! asynchronous;}
                  };



                  namespace service
                  {
                     auto state()
                     {
                        auto result = casual::service::manager::admin::api::state();

                        return common::algorithm::transform( result.services, []( auto& s)
                        {
                           return std::move( s.name);
                        });
                     }
                  }



                  namespace dispatch
                  {
                     using invoke_type = std::function< void()>;
                     
                     namespace transaction
                     {
                        auto scope = []( auto&& dispatch) -> invoke_type
                        {
                           return [dispatch=std::move( dispatch)]()
                           {
                              common::transaction::context().begin();
                              auto rollback = common::execute::scope( [](){ common::transaction::context().rollback();});

                              dispatch();

                              common::transaction::context().commit();
                              rollback.release();
                           };
                        };
                     } // transaction

                     auto stream( const std::string& service) -> invoke_type
                     {
                        return [&service]()
                        {
                           auto call = [&service]( const common::buffer::Payload& payload)
                           {
                              auto reply = common::service::call::context().sync( service, buffer::payload::Send{ payload}, {});
                              // Print the result 
                              buffer::payload::binary::stream( reply.buffer, std::cout);
                           };

                           buffer::payload::binary::stream( std::cin, call);
                        };  
                     }

                     auto iterate = []( auto iterations, auto&& dispatch) -> invoke_type
                     {
                        return [iterations,dispatch=std::move( dispatch)]() mutable
                        {
                           while( iterations-- > 0)
                              dispatch();
                        };
                       
                     };

                     
                     template< typename P>
                     auto sequential( const std::string& service, const P& payloads) -> invoke_type
                     {
                        return [&]()
                        {
                           common::algorithm::for_each( payloads, [&]( auto& payload)
                           {
                              auto reply = common::service::call::context().sync( service, buffer::payload::Send{ payload}, {});
                              // Print the result 
                              buffer::payload::binary::stream( reply.buffer, std::cout);
                           });
                        };
                     }

                     template< typename P>
                     auto parallel( const std::string& service, const P& payloads) -> invoke_type
                     {
                        return [&]()
                        {
                           auto descriptors = common::algorithm::transform( payloads, [&]( auto& payload)
                           {
                              return common::service::call::context().async( service, buffer::payload::Send{ payload}, {});
                           });

                           common::algorithm::for_each( descriptors, []( auto& descriptor)
                           {
                              auto reply = common::service::call::context().reply( descriptor, {});
                              // Print the result 
                              buffer::payload::binary::stream( reply.buffer, std::cout);
                           });
                        };
                     }

                     auto construct( const Settings& settings, const std::vector< common::buffer::Payload>& payloads)
                     {
                        auto call = settings.asynchronous ? 
                           dispatch::parallel( settings.service, payloads) 
                           : dispatch::sequential( settings.service, payloads);

                        if( settings.transaction)
                           return dispatch::iterate( settings.iterations, dispatch::transaction::scope( std::move( call)));

                        return dispatch::iterate( settings.iterations, std::move( call));
                     }

                  } // dispatch

                  namespace payload
                  {
                     auto consume() 
                     {
                        std::vector< common::buffer::Payload> payloads;

                        buffer::payload::binary::stream( std::cin, [&payloads]( auto&& payload)
                        {
                           payloads.push_back( std::move( payload));
                        });

                        return payloads;
                     }
                  } // payload

                  
                  
               
                  void call( Settings settings)
                  {
                     if( settings.stream())
                     {
                        if( settings.transaction)
                           dispatch::transaction::scope( dispatch::stream( settings.service))();
                        else 
                           dispatch::stream( settings.service)();  
                     }
                     else 
                     {
                        // we need to consume all payloads
                        auto payloads = payload::consume();

                        dispatch::construct( settings, payloads)();
                        
                     }
                  }

                  namespace complete
                  {
                     auto service = []( auto values, bool help) -> std::vector< std::string>
                     {
                        if( help) 
                           return { "<service>"};

                        try 
                        {
                           return service::state();
                        }
                        catch( ...)
                        {
                           return { "<value>"};
                        }
                     };
                  } // complete
               } // <unnamed>
            } // local
         
            struct cli::Implementation
            {
               argument::Group options()
               {
                  auto invoked = [&]()
                  {
                     local::call( m_settings);
                  };

                  return argument::Group{ invoked, [](){}, { "call"}, R"(generic service call

Reads buffer(s) from stdin and call the provided service, prints the reply buffer(s) to stdout.
Assumes that the input buffer to be in a conformant format, ie, created by casual or some other tool.
Error will be printed to stderr
)",
                     argument::Option( std::tie( m_settings.service), local::complete::service, { "-s", "--service"}, "service to call")( argument::cardinality::one{}),
                     argument::Option( std::tie( m_settings.iterations), { "--iterations"}, "number of iterations (default: 1)"),
                     argument::Option( std::tie( m_settings.transaction), { "--transaction"}, "if every iteration should be in a transaction"),
                     argument::Option( std::tie( m_settings.asynchronous), { "--asynchronous"}, "if 'buffer batch' should be called in parallel"),
                  };
               }

               local::Settings m_settings;
            };

            cli::cli() = default; 
            cli::~cli() = default; 

            argument::Group cli::options() &
            {
               return m_implementation->options();
            }
         } // call
      } // manager  
   } // gateway
} // casual



