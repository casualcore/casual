/*
 * casual_archivebase.cpp
 *
 *  Created on: Sep 16, 2012
 *      Author: lazan
 */


#include "casual_archivebase.h"


namespace casual
{
	namespace sf
	{
		ArchiveBase::ArchiveBase()
		{

		}
		virtual ArchiveBase::~ArchiveBase()
		{

		}

		void ArchiveBase::handleStart (const char* name)
		{
			handle_start( name);
		}

		void ArchiveBase::handleEnd (const char* name)
		{
			handle_end( name);
		}


		void ArchiveBase::handleContainerStart()
		{
			handle_container_start();
		}

		void ArchiveBase::handleContainerEnd()
		{
			handle_container_end();
		}

		void ArchiveBase::handleSerialtypeStart()
		{
			handle_serialtype_start();
		}

		void ArchiveBase::handleSerialtypeEnd()
		{
			handle_serialtype_end();
		}




		void ArchiveBase::handle_start(const char* name)
		{
			// no op
		}

		void ArchiveBase::handle_end(const char* name)
		{
			// no op
		}


		void ArchiveBase::handle_container_start()
		{
			// no op
		}

		void ArchiveBase::handle_container_end()
		{
			// no op
		}

		void ArchiveBase::handle_serialtype_start()
		{
			// no op
		}

		void ArchiveBase::handle_serialtype_end()
		{
			// no op
		}

	}
}

