//!
//! casual_utility_file.cpp
//!
//! Created on: May 5, 2012
//!     Author: Lazan, Jon-Erik
//!

#include "common/file.h"

#include <cstdio>

//TODO: temp
#include <iostream>

#include <dirent.h>

namespace casual
{

   namespace common
   {
      namespace file
      {
         void remove( const std::string& path)
         {
            if( !path.empty())
            {
               std::remove( path.c_str());
            }
         }

         RemoveGuard::RemoveGuard( const std::string& path)
               : m_path( path)
         {
         }

         RemoveGuard::~RemoveGuard()
         {
            remove( m_path);
         }

         const std::string& RemoveGuard::path() const
         {
            return m_path;
         }

         ScopedPath::ScopedPath( const std::string& path)
               : RemoveGuard( path)
         {
         }

         ScopedPath::operator const std::string&()
         {
            return path();
         }

         std::string find( const std::string& path, const std::regex& search)
         {
            std::string result;

            DIR* directory = opendir( path.c_str());

            if( directory)
            {
               struct dirent* element = nullptr;
               while( ( element = readdir( directory)) != nullptr)
               {

                  if( std::regex_match( element->d_name, search))
                  {
                     if( path.back() != '/')
                     {
                        result = path + "/";
                     }

                     result += element->d_name;
                     break;
                  }
               }

               closedir( directory);
            }

            return result;
         }

         std::string basename( const std::string& path)
         {
            auto basenameStart = std::find( path.crbegin(), path.crend(), '/');
            return std::string( basenameStart.base(), path.end());
         }

         std::string basedir( const std::string& path)
         {
            auto basedirEnd = std::find( path.crbegin(), path.crend(), '/');
            return std::string( path.cbegin(), basedirEnd.base());
         }

         std::string removeExtension( const std::string& path)
         {
            auto extensionBegin = std::find( path.crbegin(), path.crend(), '.');
            return std::string( path.begin(), extensionBegin.base());
         }

         std::string extension( const std::string& file)
         {
            std::string filename = basename( file);
            auto extensionEnd = std::find( filename.crbegin(), filename.crend(), '.');

            if( extensionEnd == filename.crend())
            {
               return std::string();
            }

            return std::string( extensionEnd.base(), filename.cend());
         }

      }

   }

}

