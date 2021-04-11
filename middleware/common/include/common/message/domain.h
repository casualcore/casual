//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/type.h"
#include "common/domain.h"

#include <iosfwd>

namespace casual
{
   namespace common::message
   {
      namespace domain
      {
         namespace manager::shutdown
         {
            using base_request = message::basic_request< Type::domain_manager_shutdown_request>;
            struct Request : base_request
            {
               using base_request::base_request;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_request::serialize( archive);
               )               

            };

            using base_reply = message::basic_message< Type::domain_manager_shutdown_reply>;
            struct Reply : base_reply
            {
               using base_reply::base_reply;

               //! correlations for the task
               std::vector< common::strong::correlation::id> tasks;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( tasks);
               )
            };
            
         } // manager::shutdown

         namespace process
         {
            namespace singleton::connect
            {
               using base_request = message::basic_request< Type::domain_process_singleton_connect_request>;
               struct Request : base_request
               {
                  using base_request::base_request;

                  Uuid identification;
                  std::string environment;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( identification);
                     CASUAL_SERIALIZE( environment);
                  )
               };

               namespace reply
               {
                  enum class Directive : short
                  {
                     start,
                     singleton,
                     shutdown
                  };

                  inline std::ostream& operator << ( std::ostream& out, Directive value)
                  {
                     switch( value)
                     {
                        case Directive::start: return out << "start";
                        case Directive::singleton: return out << "singleton";
                        case Directive::shutdown: return out << "shutdown";
                     }
                     return out << "unknown";
                  }
               } // reply

               using base_reply =  message::basic_message< Type::domain_process_singleton_connect_reply>;
               struct Reply : base_reply
               {
                  using base_reply::base_reply;

                  reply::Directive directive{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( directive);
                  )
               };
            } // singleton::connect

            namespace connect
            {
               using base_request = message::basic_request< Type::domain_process_connect_request>;
               struct Request : base_request
               {
                  using base_request::base_request;

                  bool whitelist = false;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( whitelist);
                  )
               };

               namespace reply
               {
                  enum class Directive : short
                  {
                     start,
                     shutdown
                  };

                  inline std::ostream& operator << ( std::ostream& out, Directive value)
                  {
                     switch( value)
                     {
                        case Directive::start: return out << "start";
                        case Directive::shutdown: return out << "shutdown";
                     }
                     return out << "unknown";
                  }
               } // reply


               using base_reply =  message::basic_message< Type::domain_process_connect_reply>;
               struct Reply : base_reply
               {
                  using base_reply::base_reply;

                  reply::Directive directive{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( directive);
                  )
               };

            } // connect

            namespace lookup
            {
               namespace request
               {
                  enum class Directive : short
                  {
                     wait,
                     direct
                  };

                  inline std::ostream& operator << ( std::ostream& out, Directive value)
                  {
                     switch( value)
                     {
                        case Directive::wait: return out << "wait";
                        case Directive::direct: return out << "direct";
                     }
                     return out << "unknown";
                  }
  
               } // request

               using base_request = message::basic_request< Type::domain_process_lookup_request>;
               struct Request : base_request
               {
                  Uuid identification;
                  strong::process::id pid;
                  request::Directive directive = request::Directive::wait;

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
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( identification);
                  )
               };

            } // lookup

            namespace prepare::shutdown
            {
               using base_request = basic_request< Type::domain_process_prepare_shutdown_request>;
               struct Request : base_request
               {
                  using base_request::base_request;

                  std::vector< common::process::Handle> processes;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( processes);
                  )
               };

               using base_reply = basic_reply< Type::domain_process_prepare_shutdown_reply>;
               struct Reply : base_reply
               {
                  using base_reply::base_reply;

                  std::vector< common::process::Handle> processes;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( processes);
                  )
               };


            } // prepare::shutdown
         } // process

         namespace instance::global::state
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
                     CASUAL_SERIALIZE( variables);
                  )
                  
               } environment;

               struct 
               {
                  std::string alias;
                  platform::size::type index{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( alias);
                     CASUAL_SERIALIZE( index);
                  )

               } instance;

               struct
               {
                  common::process::Handle handle;
                  std::string path;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( handle);
                     CASUAL_SERIALIZE( path);
                  )

               } process;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( environment);
                  CASUAL_SERIALIZE( instance);
                  CASUAL_SERIALIZE( process);
               )
            };
            
         } // instance::global::state
      } // domain

      namespace reverse
      {
         template<>
         struct type_traits< domain::process::lookup::Request> : detail::type< domain::process::lookup::Reply> {};

         template<>
         struct type_traits< domain::process::singleton::connect::Request> : detail::type< domain::process::singleton::connect::Reply> {};

         template<>
         struct type_traits< domain::process::connect::Request> : detail::type< domain::process::connect::Reply> {};

         template<>
         struct type_traits< domain::manager::shutdown::Request> : detail::type< domain::manager::shutdown::Reply> {};


         template<>
         struct type_traits< domain::process::prepare::shutdown::Request> : detail::type< domain::process::prepare::shutdown::Reply> {};

         template<>
         struct type_traits< domain::instance::global::state::Request> : detail::type< domain::instance::global::state::Reply> {};

      } // reverse

   } // common::message
} // casual


