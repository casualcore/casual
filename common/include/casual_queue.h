//!
//! casual_queue.h
//!
//! Created on: Jun 9, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_H_
#define CASUAL_QUEUE_H_

#include "casual_ipc.h"
#include "casual_message.h"
#include "casual_marshal.h"
#include "casual_utility_signal.h"


#include <list>

namespace casual
{
	namespace queue
	{
		typedef ipc::message::Transport transport_type;
		typedef transport_type::message_type_type message_type_type;

		namespace blocking
		{

		   class Writer
         {
         public:

		      typedef void result_type;

            Writer( ipc::send::Queue& queue);

            //!
            //! Sends/Writes a message to the queue. which can result in several
            //! actual ipc-messages.
            //!
            template< typename M>
            void operator () ( M& message)
            {
               //
               // Serialize the message
               //
               marshal::output::Binary archive;
               archive << message;

               message_type_type type = message::type( message);

               send( archive, type);

            }
         private:

            void send( marshal::output::Binary& archive, message_type_type type);

            ipc::send::Queue& m_queue;
         };


			//!
			//!
			//!
			class Reader
			{
			public:

		      typedef void result_type;

				Reader( ipc::receive::Queue& queue);

				//!
				//! Gets the next message type.
				//!
				message_type_type next();

				//!
				//! Tries to read a specific message from the queue.
				//! @attention use next() to determine which message is ready to read.
				//!
				template< typename M>
				void operator () ( M& message)
				{
					message_type_type type = message::type( message);

					marshal::input::Binary archive;

					correlate( archive, type);

					archive >> message;
				}


			private:

				void correlate( marshal::input::Binary& archive, message_type_type type);

				ipc::receive::Queue& m_queue;

			};
		} // blocking

		namespace non_blocking
		{
		   //!
		   //! Non blocking writer. There are only a few cases where we use this semantic
		   //!
		   class Writer
         {
         public:

		      typedef bool result_type;

            Writer( ipc::send::Queue& queue);

            //!
            //! Sends/Writes a message to the queue. which can result in several
            //! actual ipc-messages.
            //!
            //! @note non-blocking
            //! @return true if the whole message is sent. false otherwise
            //!
            template< typename M>
            bool operator () ( M& message)
            {
               //
               // Serialize the message
               //
               marshal::output::Binary archive;
               archive << message;

               message_type_type type = message::type( message);

               return send( archive, type);

            }
         private:

            bool send( marshal::output::Binary& archive, message_type_type type);

            ipc::send::Queue& m_queue;
         };


			//!
			//! Non-blocking reader
			//!
			class Reader
			{
			public:

		      typedef bool result_type;

				Reader( ipc::receive::Queue& queue);


				//!
				//! Tries to read a specific message from the queue.
				//! non-blocking
				//! @return true if the specific message-type is read. false otherwise.
				//!
				template< typename M>
				bool operator() ( M& message)
				{
					message_type_type type = message::type( message);

					marshal::input::Binary archive;

					if( correlate( archive, type))
					{
						archive >> message;
						return true;
					}
					return false;
				}

				//!
				//! Consumes all transport messages that is present on the ipc-queue, and
				//! stores these to cache.
				//!
				//! @note non blocking
				//!
				bool consume();


			private:

				bool correlate( marshal::input::Binary& archive, message_type_type type);

				ipc::receive::Queue& m_queue;

			};

		} // non_blocking

	} // queue

} // casual


#endif /* CASUAL_QUEUE_H_ */
