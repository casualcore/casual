/*
 * ini.cpp
 *
 *  Created on: Feb 11, 2015
 *      Author: kristone
 */


#include "sf/archive/ini.h"

#include "sf/exception.h"

#include "common/transcode.h"

#include <sstream>
#include <iterator>
#include <algorithm>
#include <locale>
#include <string>

namespace casual
{
   namespace sf
   {

      namespace archive
      {
         namespace ini
         {
            namespace
            {
               namespace local
               {
                  const std::string magic{ '@' };
               } // local
            } //

            Load::Load() = default;
            Load::~Load() = default;

            namespace
            {
               namespace local
               {
                  std::string trim( std::string string)
                  {
                     const auto trimmer = [] ( const std::string::value_type character)
                     { return ! std::isspace( character, std::locale::classic()); };

                     string.erase( string.begin(), std::find_if( string.begin(), string.end(), trimmer));
                     string.erase( std::find_if( string.rbegin(), string.rend(), trimmer).base(), string.end());

                     return string;
                  }

                  // TODO: Make it streamable
                  std::string decode( const std::string& data)
                  {
                     std::ostringstream stream;

                     for( std::string::size_type idx = 0; idx < data.size(); ++idx)
                     {
                        if( data[idx] == '\\')
                        {
                           // TODO: handle all control-characters (and back-slash)

                           switch( data[++idx])
                           {
                           case '\\': stream.put( '\\'); break;
                           case '0': stream.put( '\0');  break;
                           case 'a': stream.put( '\a');  break;
                           case 'b': stream.put( '\b');  break;
                           case 'f': stream.put( '\f');  break;
                           case 'n': stream.put( '\n');  break;
                           case 'r': stream.put( '\r');  break;
                           case 't': stream.put( '\t');  break;
                           case 'v': stream.put( '\v');  break;
                           default: throw exception::archive::invalid::Document{ "Invalid content"};
                           }
                        }
                        else
                        {
                           stream.put( data[idx]);
                        }

                     }

                     return stream.str();
                  }

                  void parse_flat( tree& document, std::istream& stream)
                  {
                     //
                     // This function would make Sean Parent cry !!!
                     //

                     auto composite = &document;

                     std::vector<std::string> last;


                     std::string line;
                     while( std::getline( stream, line))
                     {
                        const auto candidate = trim( line);

                        if( candidate.empty())
                        {
                           //
                           // Found nothing and we just ignore it
                           //
                           continue;
                        }

                        if( candidate.find_first_of( "#;") == 0)
                        {
                           //
                           // Found a comment and we just ignore it
                           //
                           continue;
                        }

                        const auto separator = line.find_first_of( '=');

                        if( separator != std::string::npos)
                        {
                           //
                           // We found a (potential) value and some data
                           //

                           auto name = trim( line.substr( 0, separator));
                           auto data = line.substr( separator + 1);

                           //
                           // Add it to the tree after some unmarshalling
                           //
                           composite->values.emplace( std::move( name), decode( data));

                           continue;
                        }


                        if( candidate.front() == '[' && candidate.back() == ']')
                        {
                           //
                           // Found a potential section (a.k.a. composite a.k.a. serializable)
                           //


                           //
                           // An internal lambda-helper to split a qualified name
                           //
                           const auto splitter = [] ( std::string qualified)
                           {
                              std::vector<std::string> result;

                              std::istringstream stream( std::move( qualified));
                              std::string name;
                              while( std::getline( stream, name, '.'))
                              {
                                 auto candidate = trim( std::move( name) );

                                 if( candidate.empty())
                                 {
                                    throw exception::archive::invalid::Document{ "Invalid name"};
                                 }
                                 else
                                 {
                                    result.push_back( std::move( candidate));
                                 }
                              }

                              if( result.empty())
                              {
                                 throw exception::archive::invalid::Document{ "Invalid name"};
                              }

                              return result;


                           };

                           //
                           // An internal lambda-helper to help out where to add this section
                           //
                           const auto finder = [] ( const std::vector<std::string>& last, const std::vector<std::string>& next)
                           {
                              if( next.size() > last.size())
                              {
                                 return std::mismatch( last.begin(), last.end(), next.begin()).second;
                              }

                              auto match = std::mismatch( next.begin(), next.end(), last.begin());

                              return match.first != next.end() ? match.first : std::prev( match.first);
                           };


                           //
                           // First we split the string (e.g. 'foo.bar' into 'foo' and 'bar')
                           //
                           auto next = splitter( { candidate.begin() + 1, candidate.end() - 1 } );


                           const auto inserter = finder( last, next);

                           composite = &document;

                           //
                           // Search from root to find out where this should be added
                           //
                           for( auto name = next.begin(); name != inserter; ++name)
                           {
                              composite = &std::prev( composite->children.upper_bound( *name))->second;
                           }

                           //
                           // Add all names where they should be added
                           //
                           for( auto name = inserter; name != next.end(); ++name)
                           {
                              composite = &composite->children.emplace( *name, tree())->second;
                           }

                           //
                           // Keep this qualified name until next time
                           //
                           std::swap( last, next);

                           continue;
                        }


                        if( candidate.back() == '\\')
                        {
                           //
                           // TODO: Possibly append to previous data
                           //
                           // Can be done by using 'inserter' and previous.back()
                           //
                        }


                        //
                        // Unknown content
                        //
                        exception::archive::invalid::Document( "Invalid document");

                     }

                  }


               } // local

            } //

            const tree& Load::serialize( std::istream& stream)
            {
               local::parse_flat( m_document, stream);
               return source();
            }

            const tree& Load::serialize( const std::string& ini)
            {
               std::istringstream stream( ini);
               return serialize( stream);
            }


            const tree& Load::source() const
            {
               return m_document;
            }


            namespace reader
            {

               Implementation::Implementation( const tree& document) : m_node_stack{ &document } {}

               std::tuple< std::size_t, bool> Implementation::container_start( const std::size_t size, const char* const name)
               {

                  if( name)
                  {
                     //
                     // We do not know whether it's a node or data
                     //

                     const auto node = m_node_stack.back()->children.equal_range( name);

                     if( node.first != node.second)
                     {
                        //
                        // Transform backwards
                        //
                        for( auto iterator = node.second; iterator != node.first; --iterator)
                        {
                           m_node_stack.push_back( &std::prev( iterator)->second);
                        }

                        return std::make_tuple( std::distance( node.first, node.second), true);
                     }

                     const auto data = m_node_stack.back()->values.equal_range( name);

                     if( data.first != data.second)
                     {
                        //
                        // Transform backwards
                        //
                        for( auto iterator = data.second; iterator != data.first; --iterator)
                        {
                           m_data_stack.push_back( &std::prev( iterator)->second);
                        }

                        return std::make_tuple( std::distance( data.first, data.second), true);
                     }

                  }
                  else
                  {
                     //
                     // An idea to handle this is by creating fake serializable
                     //
                     // E.g. [@name], [@name.@name], etc or something
                     //
                     throw exception::archive::invalid::Node{ "Nested containers not supported (yet)"};
                  }


                  //
                  // Note that we return 'true' anyway
                  //

                  return std::make_tuple( 0, true);

               }

               void Implementation::container_end( const char* const name)
               {

               }

               bool Implementation::serialtype_start( const char* const name)
               {
                  if( name)
                  {
                     const auto node = m_node_stack.back()->children.find( name);

                     if( node != m_node_stack.back()->children.end())
                     {
                        m_node_stack.push_back( &node->second);
                     }
                     else
                     {
                        return false;
                     }
                  }
                  else
                  {
                     //
                     // It must have been a container-content and thus already found
                     //
                  }

                  return true;

               }

               void Implementation::serialtype_end( const char* const name)
               {
                  m_node_stack.pop_back();
               }

               bool Implementation::value_start( const char* name)
               {
                  if( name)
                  {
                     const auto data = m_node_stack.back()->values.find( name);

                     if( data != m_node_stack.back()->values.end())
                     {
                        m_data_stack.push_back( &data->second);
                     }
                     else
                     {
                        return false;
                     }
                  }
                  else
                  {
                     //
                     // It must have been a container-content and thus already found
                     //
                  }

                  return true;

               }

               void Implementation::value_end( const char* name)
               {
                  m_data_stack.pop_back();
               }

               namespace
               {
                  namespace local
                  {

                     template<typename T>
                     T read( const std::string& data)
                     {
                        std::istringstream stream( data);
                        T result;
                        stream >> result;
                        if( ! stream.fail() && stream.eof())   return result;
                        throw exception::archive::invalid::Node{ "unexpected type"};
                     }


                     //
                     // std::istream::eof() is not set when streaming bool
                     //
                     template<>
                     bool read( const std::string& data)
                     {
                        if( data == "true")  return true;
                        if( data == "false") return false;
                        throw exception::archive::invalid::Node{ "unexpected type"};
                     }
                  }
               } // <unnamed>

               void Implementation::read( bool& value) const
               { value = local::read<bool>( *m_data_stack.back()); }
               void Implementation::read( short& value) const
               { value = local::read<short>( *m_data_stack.back()); }
               void Implementation::read( long& value) const
               { value = local::read<long>( *m_data_stack.back()); }
               void Implementation::read( long long& value) const
               { value = local::read<long long>( *m_data_stack.back()); }
               void Implementation::read( float& value) const
               { value = local::read<float>( *m_data_stack.back()); }
               void Implementation::read( double& value) const
               { value = local::read<double>( *m_data_stack.back()); }

               void Implementation::read( char& value) const
               {
                  value = m_data_stack.back()->empty() ? '\0' : m_data_stack.back()->front();
               }

               void Implementation::read( std::string& value) const
               {
                  value = *m_data_stack.back();
               }

               void Implementation::read( std::vector<char>& value) const
               {
                  //
                  // Binary data might be double-decoded (in the end)
                  //
                  value = common::transcode::base64::decode( *m_data_stack.back());
               }

            } // reader


            namespace
            {
               namespace local
               {

                  // TODO: Make it streamable
                  std::string encode( const std::string& data)
                  {
                     std::ostringstream stream;
                     for( const auto sign : data)
                     {
                        // TODO: handle all control-characters (and back-slash)
                        //if( std::iscntrl( sign, std::locale::classic())){ ... }

                        switch( sign)
                        {
                        case '\\': stream << "\\\\";  break;
                        case '\0': stream << "\\0";   break;
                        case '\a': stream << "\\a";   break;
                        case '\b': stream << "\\b";   break;
                        case '\f': stream << "\\f";   break;
                        case '\n': stream << "\\n";   break;
                        case '\r': stream << "\\r";   break;
                        case '\t': stream << "\\t";   break;
                        case '\v': stream << "\\v";   break;
                        default: stream.put( sign);   break;
                        }
                     }

                     return stream.str();
                  }

                  void write_flat( const tree& node, std::ostream& stream, const std::string& name = "")
                  {
                     for( const auto& value : node.values)
                     {
                        stream << value.first << '=' << encode( value.second) << '\n';
                     }

                     for( const auto& child : node.children)
                     {
                        const auto qualified = name.empty() ? child.first : name + '.' + child.first;

                        //
                        // Let's print useless empty sections anyway ('cause
                        // reading need 'em as of today)
                        //
                        //if( ! child.second.values.empty())
                        {
                           stream << '\n' << '[' << qualified << ']' << '\n';
                        }

                        write_flat( child.second, stream, qualified);
                     }

                  }

/*
                  void write_tree( const tree& node, std::ostream& stream, std::string::size_type indent = 0)
                  {
                     for( const auto& value : node.values)
                     {
                        stream << std::string( indent, ' ') << value.first << '=' << value.second << '\n';
                     }

                     for( const auto& child : node.children)
                     {
                        ++indent;
                        stream << '\n' << std::string( indent, ' ') << '[' << child.first << ']' << '\n';
                        write_tree( child.second, stream, indent);
                        ++indent;
                     }

                  }
*/

               } // local

            } //


            Save::Save() = default;
            Save::~Save() = default;

            void Save::serialize( std::ostream& ini) const
            {
               local::write_flat( m_document, ini);
            }

            void Save::serialize( std::string& ini) const
            {
               std::ostringstream stream;
               serialize( stream);
               ini.assign( stream.str());
            }

            namespace writer
            {

               Implementation::Implementation( tree& document) : m_node_stack{ &document } {}

               std::size_t Implementation::container_start( const std::size_t size, const char* const name)
               {
                  //
                  // We do not know where it's node or data
                  //

                  if( name)
                  {
                     m_name_stack.push_back( name);
                  }
                  else
                  {
                     throw exception::archive::invalid::Node{ "Nested containers not supported (yet)"};
                  }

                  return size;
               }

               void Implementation::container_end( const char* const name)
               {
                  if( name)
                  {
                     m_name_stack.pop_back();
                  }
               }

               void Implementation::serialtype_start( const char* const name)
               {
                  const auto final = name ? name : m_name_stack.back();

                  const auto child = m_node_stack.back()->children.emplace( final, tree());

                  m_node_stack.push_back( &child->second);
               }

               void Implementation::serialtype_end( const char* const name)
               {
                  m_node_stack.pop_back();
               }

               void Implementation::write( std::string data, const char* const name)
               {
                  const auto final = name ? name : m_name_stack.back();

                  m_node_stack.back()->values.emplace( final, std::move( data));
               }

               std::string Implementation::encode( const bool& value) const
               {
                  return value ? "true" : "false";
               }

               std::string Implementation::encode( const char& value) const
               {
                  return std::string{ value};
               }

               std::string Implementation::encode( const std::vector<char>& value) const
               {
                  //
                  // Binary data might be double-encoded
                  //
                  return common::transcode::base64::encode( value);
               }

            } // writer

         } // ini

      } // archive

   } // sf

} // casual


