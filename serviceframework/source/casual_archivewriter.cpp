/*
 * casual_archivewriter.cpp
 *
 *  Created on: Sep 16, 2012
 *      Author: lazan
 */

#include "casual_archivewriter.h"

namespace casual
{
	namespace sf
	{

	 	 ArchiveWriter::ArchiveWriter()
	 	 {

	 	 }

	 	 virtual ArchiveWriter::~OutputArchive()
	 	 {

	 	 }


	 	void ArchiveWriter::write( const bool& value)
	 	{
	 		writePOD( value);
	 	}


		 void ArchiveWriter::write (const char& value)
		 {
				writePOD( value);
			}

		 void ArchiveWriter::write (const short& value)
		 {
			writePOD( value);
		}

		 void ArchiveWriter::write (const int& value);

		 //void write (const unsigned int& value);

		 void ArchiveWriter::write( const long& value)
		 {
			 writePOD( value);
		 }

		 //void write (const unsigned long& value);

		 void ArchiveWriter::write (const float& value)
		 {
		 	writePOD( value);
		 }

		 void ArchiveWriter::write (const double& value)
		 {
		 	writePOD( value);
		 }

		 void ArchiveWriter::write (const std::string& value)
		 {
		 	writePOD( value);
		 }

		 void ArchiveWriter::write (const std::wstring& value)
		 {
		 	writePOD( value);
		 }

		 void ArchiveWriter::write (const std::vector< char>& value)
		 {
			 writePOD( value);
		 }

	}
}



