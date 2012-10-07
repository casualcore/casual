/*
 * casual_archivebase.h
 *
 *  Created on: Sep 16, 2012
 *      Author: lazan
 */

#ifndef CASUAL_ARCHIVEBASE_H_
#define CASUAL_ARCHIVEBASE_H_

namespace casual
{
	namespace sf
	{

	  class ArchiveBase
	  {

		public:

		    ArchiveBase();
			virtual ~ArchiveBase();

		protected:

			void handleStart (const char* name);

			void handleEnd (const char* name);

			void handleContainerStart ();

			void handleContainerEnd ();

			void handleSerialtypeStart ();

			void handleSerialtypeEnd ();

		private:


			virtual void handle_start (const char* name);

			virtual void handle_end (const char* name);

			virtual void handle_container_start ();

			virtual void handle_container_end ();

			virtual void handle_serialtype_start ();

			virtual void handle_serialtype_end ();

	  };

	} // sf
} // casual


#endif /* CASUAL_ARCHIVEBASE_H_ */
