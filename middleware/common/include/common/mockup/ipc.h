//!
//! casual
//!

#ifndef COMMON_MOCKUP_IPC_H_
#define COMMON_MOCKUP_IPC_H_

//#include "common/ipc.h"
#include "common/communication/message.h"
#include "common/communication/ipc.h"
#include "common/pimpl.h"
#include "common/platform.h"
#include "common/message/type.h"
#include "common/message/dispatch.h"

#include "common/marshal/binary.h"


namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace mockup
         {
            using Disconnect =  common::message::basic_message< common::message::Type::mockup_disconnect>;
            using Clear =  common::message::basic_message< common::message::Type::mockup_clear>;
         }
      }

      namespace mockup
      {
         namespace pid
         {
            platform::pid::type next();

         } // pid

         namespace ipc
         {

            using id_type = platform::ipc::id::type;

            using transform_type = std::function< std::vector< communication::message::Complete>( communication::message::Complete&)>;

            namespace eventually
            {


               void send( id_type destination, communication::message::Complete&& complete);

               template< typename M>
               void send( id_type destination, M&& message)
               {
                  send( destination, marshal::complete( std::move( message)));
               }


            } // eventually




            //!
            //! Links one queue to another.
            //!
            //! Reads transport-messages from input and writes them to
            //! output. Caches transport if we can't write.
            //!
            //! neither of the input and output is owned by an instance of Link
            //!
            struct Link
            {
               Link( id_type input, id_type output);
               ~Link();

               Link( Link&&) noexcept;
               Link& operator = ( Link&&) noexcept;

               id_type input() const;
               id_type output() const;

               void clear() const;

            private:
               class Implementation;
               move::basic_pimpl< Implementation> m_implementation;
            };

            //!
            //! Collects messages from input and put them in output
            //! caches messages if the output is full
            //!
            //!
            struct Collector
            {
               Collector();
               ~Collector();


               //!
               //! input-queue is owned by the Collector
               //!
               id_type input() const;
               inline id_type id() const { return input();}

               //!
               //! output-queue is owned by the Collector
               //!
               communication::ipc::inbound::Device& output() const;

               process::Handle process() const;

               void clear();

            private:
               struct Implementation;
               move::basic_pimpl< Implementation> m_implementation;
            };



            //!
            //! Replies to a request
            //!
            //!
            struct Replier
            {
               //!
               //! @param replier invoked on receive, and could send a reply
               //!
               Replier( message::dispatch::Handler&& replier);

               ~Replier();


               Replier( Replier&&) noexcept;
               Replier& operator = ( Replier&&) noexcept;

               //!
               //! input-queue is owned by the Replier
               //!
               id_type input() const;
               process::Handle process() const;

            private:
               struct Implementation;
               move::basic_pimpl< Implementation> m_implementation;
            };


         } // ipc

      } // mockup
   } // common


} // casual

#endif // IPC_H_
