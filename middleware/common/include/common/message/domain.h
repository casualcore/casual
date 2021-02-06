//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/type.h"
#include "common/domain.h"

namespace casual
{
   namespace common::message
   {
      namespace domain
      {
         namespace process
         {
            namespace connect
            {

               template< message::Type type>
               struct basic_request : message::basic_request< type>
               {
                  using base_type = message::basic_request< type>;;
                  using base_type::base_type;

                  Uuid identification;
                  bool whitelist = false;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( identification);
                     CASUAL_SERIALIZE( whitelist);
                  )
               };

               using Request = basic_request< Type::domain_process_connect_request>;

               template< message::Type type>
               struct basic_reply : message::basic_message< type>
               {
                  using base_type =  message::basic_message< type>;
                  using base_type::base_type;

                  enum class Directive : short
                  {
                     start,
                     singleton,
                     shutdown
                  };

                  inline friend std::ostream& operator << ( std::ostream& out, Directive value)
                  {
                     switch( value)
                     {
                        case Directive::start: return out << "start";
                        case Directive::singleton: return out << "singleton";
                        case Directive::shutdown: return out << "shutdown";
                     }
                     return out << "unknown";
                  }

                  Directive directive = Directive::start;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( directive);
                  )
               };

               using Reply = basic_reply< Type::domain_process_connect_reply>;

            } // connect

            namespace lookup
            {
               using base_request = message::basic_request< Type::domain_process_lookup_request>;
               struct Request : base_request
               {
                  enum class Directive : short
                  {
                     wait,
                     direct
                  };

                  inline friend std::ostream& operator << ( std::ostream& out, Directive value)
                  {
                     switch( value)
                     {
                        case Directive::wait: return out << "wait";
                        case Directive::direct: return out << "direct";
                     }
                     return out << "unknown";
                  }

                  Uuid identification;
                  strong::process::id pid;
                  Directive directive = Directive::wait;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( identification);
                     CASUAL_SERIALIZE( pid);
                     CASUAL_SERIALIZE( directive);
                  )
               };

               using base_reply = basic_reply< Type::domain_process_lookup_reply>;

               struct Reply : base_reply
               {
                  Uuid identification;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( identification);
                  })
               };

            } // lookup

            namespace prepare
            {
               namespace shutdown
               {
                  using base_request = basic_request< Type::domain_process_prepare_shutdown_request>;
                  struct Request : base_request
                  {
                     using base_request::base_request;

                     std::vector< common::process::Handle> processes;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_request::serialize( archive);
                        CASUAL_SERIALIZE( processes);
                     })
                  };

                  using base_reply = basic_reply< Type::domain_process_prepare_shutdown_reply>;
                  struct Reply : base_reply
                  {
                     using base_reply::base_reply;

                     std::vector< common::process::Handle> processes;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_reply::serialize( archive);
                        CASUAL_SERIALIZE( processes);
                     })
                  };


               } // shutdown
            } // prepare
         } // process

         namespace instance
         {
            namespace global
            {
               namespace state
               {
                  using Request = basic_request< Type::domain_instance_global_state_request>;
                  
                  using base_reply = basic_message< Type::domain_instance_global_state_reply>;
                  struct Reply : base_reply
                  {
                     using base_reply::base_reply;

                     struct 
                     {
                        std::vector< environment::Variable> variables;
                        
                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           CASUAL_SERIALIZE( variables);
                        })
                        
                     } environment;

                     struct 
                     {
                        std::string alias;
                        platform::size::type index{};

                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           CASUAL_SERIALIZE( alias);
                           CASUAL_SERIALIZE( index);
                        })

                     } instance;

                     struct
                     {
                        common::process::Handle handle;
                        std::string path;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           CASUAL_SERIALIZE( handle);
                           CASUAL_SERIALIZE( path);
                        })

                     } process;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_reply::serialize( archive);
                        CASUAL_SERIALIZE( environment);
                        CASUAL_SERIALIZE( instance);
                        CASUAL_SERIALIZE( process);
                     })
                  };
                  
               } // state
            } // global
         } // instance
      } // domain

      namespace reverse
      {
         template<>
         struct type_traits< domain::process::lookup::Request> : detail::type< domain::process::lookup::Reply> {};

         template<>
         struct type_traits< domain::process::connect::Request> : detail::type< domain::process::connect::Reply> {};


         template<>
         struct type_traits< domain::process::prepare::shutdown::Request> : detail::type< domain::process::prepare::shutdown::Reply> {};

         template<>
         struct type_traits< domain::instance::global::state::Request> : detail::type< domain::instance::global::state::Reply> {};

      } // reverse

   } // common::message
} // casual


