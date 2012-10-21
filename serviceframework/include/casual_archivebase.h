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

	   namespace archive
	   {

         class Base
         {

         public:

            Base();
            virtual ~Base();

         //protected:

            void handleStart( const char* name);

            void handleEnd( const char* name);

            void handleContainerStart();

            void handleContainerEnd();

            void handleSerialtypeStart();

            void handleSerialtypeEnd();

         private:

            virtual void handle_start( const char* name) = 0;

            virtual void handle_end( const char* name) = 0;

            virtual void handle_container_start() = 0;

            virtual void handle_container_end() = 0;

            virtual void handle_serialtype_start() = 0;

            virtual void handle_serialtype_end() = 0;

         };
	   }

	} // sf
} // casual


#endif /* CASUAL_ARCHIVEBASE_H_ */
