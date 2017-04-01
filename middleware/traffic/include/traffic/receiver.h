//!
//! casual
//!

#ifndef TRAFFIC_RECEIVER_H_
#define TRAFFIC_RECEIVER_H_


#include "common/platform.h"
#include "common/transaction/id.h"


namespace casual
{
   namespace traffic
   {

      //!
      //! Traffic-event
      //!
      struct Event
      {
         const std::string& service() const;

         //!
         //! Parent (caller) service, if any.
         //!
         const std::string& parent() const;

         common::platform::pid::type pid() const;
         const common::Uuid& execution() const;
         const common::transaction::ID& transaction() const;
         const common::platform::time::point::type& start() const;
         const common::platform::time::point::type& end() const;

      protected:
         Event();

      private:
         virtual const std::string& get_service() const = 0;
         virtual const std::string& get_parent() const = 0;
         virtual common::platform::pid::type get_pid() const = 0;
         virtual const common::Uuid& get_execution() const = 0;
         virtual const common::transaction::ID& get_transaction() const = 0;
         virtual const common::platform::time::point::type& get_start() const = 0;
         virtual const common::platform::time::point::type& get_end() const = 0;
      };


      namespace handler
      {
         struct Base
         {
            using event_type = traffic::Event;

            Base() = default;
            virtual ~Base() = default;

            virtual void persist_begin() = 0;
            virtual void log( const event_type&) = 0;
            virtual void persist_commit() = 0;
         };

      } // handler


      struct Receiver
      {
      public:

         Receiver();
         Receiver( const common::Uuid& application);
         ~Receiver();

         Receiver( const Receiver&) = delete;
         Receiver& operator = ( const Receiver&) = delete;


         int start( handler::Base& log);
      };

   } // traffic
} // casual


#endif // RECEIVER_H_
