//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/serialize/ini.h"
#include "common/serialize/create.h"

#include "common/exception/casual.h"

#include "common/transcode.h"
#include "common/buffer/type.h"

#include <sstream>
#include <iterator>
#include <algorithm>
#include <locale>
#include <string>

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace ini
         {
            namespace local
            {
               namespace
               {
                  std::vector< std::string> keys() { return { "ini", common::buffer::type::ini()};};

                  const std::string magic{ '@' };

                  struct tree
                  {
                     std::multimap<std::string,tree> children;
                     std::multimap<std::string,std::string> values;
                  };

                  namespace reader
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
                              default: throw exception::casual::invalid::Document{ "Invalid content"};
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
                        // This function would make Sean Parent cry !!!

                        auto composite = &document;

                        std::vector<std::string> last;

                        std::string line;
                        while( std::getline( stream, line))
                        {
                           const auto candidate = trim( line);

                           if( candidate.empty())
                           {
                              // Found nothing and we just ignore it
                              continue;
                           }

                           if( candidate.find_first_of( "#;") == 0)
                           {
                              // Found a comment and we just ignore it
                              continue;
                           }

                           const auto separator = line.find_first_of( '=');

                           if( separator != std::string::npos)
                           {
                              // We found a (potential) value and some data

                              auto name = trim( line.substr( 0, separator));
                              auto data = line.substr( separator + 1);

                              // Add it to the tree after some unmarshalling
                              composite->values.emplace( std::move( name), decode( data));

                              continue;
                           }


                           if( candidate.front() == '[' && candidate.back() == ']')
                           {
                              // Found a potential section (a.k.a. composite a.k.a. serializable)


                              // An internal lambda-helper to split a qualified name
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
                                       throw exception::casual::invalid::Document{ "Invalid name"};
                                    }
                                    else
                                    {
                                       result.push_back( std::move( candidate));
                                    }
                                 }

                                 if( result.empty())
                                 {
                                    throw exception::casual::invalid::Document{ "Invalid name"};
                                 }

                                 return result;


                              };

                              // An internal lambda-helper to help out where to add this section
                              const auto finder = [] ( const std::vector<std::string>& last, const std::vector<std::string>& next)
                              {
                                 if( next.size() > last.size())
                                 {
                                    return std::mismatch( last.begin(), last.end(), next.begin()).second;
                                 }

                                 auto match = std::mismatch( next.begin(), next.end(), last.begin());

                                 return match.first != next.end() ? match.first : std::prev( match.first);
                              };

                              // First we split the string (e.g. 'foo.bar' into 'foo' and 'bar')
                              auto next = splitter( { candidate.begin() + 1, candidate.end() - 1 } );


                              const auto inserter = finder( last, next);

                              composite = &document;

                              // Search from root to find out where this should be added
                              for( auto name = next.begin(); name != inserter; ++name)
                              {
                                 composite = &std::prev( composite->children.upper_bound( *name))->second;
                              }

                              // Add all names where they should be added
                              for( auto name = inserter; name != next.end(); ++name)
                              {
                                 composite = &composite->children.emplace( *name, tree())->second;
                              }

                              // Keep this qualified name until next time
                              std::swap( last, next);

                              continue;
                           }


                           if( candidate.back() == '\\')
                           {
                              // TODO: Possibly append to previous data
                              //
                              // Can be done by using 'inserter' and previous.back()
                           }

                           // Unknown content
                           exception::casual::invalid::Document( "Invalid document");
                        }
                     }

                     tree parse_flat( std::istream& stream)
                     {
                        tree document;
                        parse_flat( document, stream);
                        return document;
                     }

                     tree parse_flat( const std::string& ini)
                     {
                        std::istringstream stream( ini);
                        return parse_flat( stream);
                     }

                     tree parse_flat( const char* ini, platform::size::type size)
                     {
                        std::istringstream stream( std::string( ini, size));
                        return parse_flat( stream);
                     }


                     tree parse_flat( const platform::binary::type& ini)
                     {
                        return parse_flat( ini.data(), ini.size());
                     }

                     struct Implementation
                     {
                        static auto keys() { return local::keys();}


                        Implementation( tree&& document) : m_document( std::move( document)), m_node_stack{ &m_document} {}

                        template< typename S>
                        Implementation( S&& source) : Implementation( parse_flat( std::forward< S>( source))) {};

                        std::tuple< platform::size::type, bool> container_start( const platform::size::type size, const char* const name)
                        {

                           if( name)
                           {
                              // We do not know whether it's a node or data

                              const auto node = m_node_stack.back()->children.equal_range( name);

                              if( node.first != node.second)
                              {
                                 // Transform backwards
                                 for( auto iterator = node.second; iterator != node.first; --iterator)
                                 {
                                    m_node_stack.push_back( &std::prev( iterator)->second);
                                 }

                                 return std::make_tuple( std::distance( node.first, node.second), true);
                              }

                              const auto data = m_node_stack.back()->values.equal_range( name);

                              if( data.first != data.second)
                              {
                                 // Transform backwards
                                 for( auto iterator = data.second; iterator != data.first; --iterator)
                                 {
                                    m_data_stack.push_back( &std::prev( iterator)->second);
                                 }

                                 return std::make_tuple( std::distance( data.first, data.second), true);
                              }

                           }
                           else
                           {
                              // An idea to handle this is by creating fake serializable
                              //
                              // E.g. [@name], [@name.@name], etc or something
                              throw exception::casual::invalid::Node{ "Nested containers not supported (yet)"};
                           }

                           // Note that we return 'true' anyway - Why?

                           return std::make_tuple( 0, false);

                        }

                        void container_end( const char* const name) {}

                        bool composite_start( const char* const name)
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

                           // Either we found the node or we assume it's an 'unnamed' container
                           // element that is already pushed to the stack

                           return true;

                        }

                        void composite_end(  const char* const name)
                        {
                           m_node_stack.pop_back();
                        }

                        bool value_start( const char* name)
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

                           // Either we found the node or we assume it's an 'unnamed' container
                           // element that is already pushed to the stack

                           return true;

                        }

                        void value_end( const char* name)
                        {
                           m_data_stack.pop_back();
                        }

                        template< typename T>
                        bool read( T& value, const char* const name)
                        {
                           if( ! value_start( name))
                           {
                              return false;
                           }

                           read( value);
                           value_end( name);

                           return true;
                        }

    
                        template<typename T>
                        void read( T& value)
                        {
                           value = common::from_string< T>( *m_data_stack.back());
                        }

                        void read( bool& value)
                        { 
                           if( *m_data_stack.back() == "true")  value = true;
                           else if( *m_data_stack.back() == "false") value = false;
                           else throw exception::casual::invalid::Node{ "unexpected type"};
                        }

                        void read( char& value)
                        {
                           value = m_data_stack.back()->empty() ? '\0' : m_data_stack.back()->front();
                        }

                        void read( std::string& value)
                        {
                           value = *m_data_stack.back();
                        }

                        void read( view::Binary value)
                        {
                           auto binary = common::transcode::base64::decode( *m_data_stack.back());
                           if( range::size( binary) != range::size( value))
                              throw exception::casual::invalid::Node{ "binary size missmatch"};

                           algorithm::copy( binary, std::begin( value));
                        }

                        void read( platform::binary::type& value)
                        {
                           // Binary data might be double-decoded (in the end)
                           value = common::transcode::base64::decode( *m_data_stack.back());
                        }

                        policy::canonical::Representation canonical()
                        {
                           return {};
                        }
                     private:

                        tree m_document;
                        using data = std::string;
                        std::vector<const data*> m_data_stack;
                        std::vector<const tree*> m_node_stack;
                     };

                  } // reader


                  namespace writer
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

                           // Let's print useless empty sections anyway ('cause
                           // reading need 'em as of today)
                           {
                              stream << '\n' << '[' << qualified << ']' << '\n';
                           }

                           write_flat( child.second, stream, qualified);
                        }
                     }
                     
                     class Implementation
                     {
                     public:

                        static auto keys() { return local::keys();}

                        Implementation() : m_node_stack{ &m_document } {}

                        platform::size::type container_start( const platform::size::type size, const char* const name)
                        {
                           // We do not know where it's node or data

                           if( name)
                           {
                              m_name_stack.push_back( name);
                           }
                           else
                           {
                              throw exception::casual::invalid::Node{ "Nested containers not supported (yet)"};
                           }

                           return size;
                        }
                        
                        void container_end( const char* const name)
                        {
                           if( name)
                           {
                              m_name_stack.pop_back();
                           }
                        }

                        void composite_start( const char* const name)
                        {
                           const auto final = name ? name : m_name_stack.back();

                           const auto child = m_node_stack.back()->children.emplace( final, tree());

                           m_node_stack.push_back( &child->second);
                        }

                        void composite_end(  const char* const name)
                        {
                           m_node_stack.pop_back();
                        }

                        template< typename T>
                        void write( const T& data, const char* const name)
                        {
                           write( encode( data), name);
                        }

                        void write( std::string data, const char* const name)
                        {
                           const auto final = name ? name : m_name_stack.back();

                           m_node_stack.back()->values.emplace( final, std::move( data));
                        }

                        const tree& document() const { return m_document;}

                        void flush( std::ostream& destination) 
                        {
                           write_flat( Implementation::document(), destination);
                        }

                     private:

                        template<typename T>
                        std::string encode( const T& value) const
                        {
                           return std::to_string( value);
                        }

                        // A few overloads

                        std::string encode( const bool& value) const
                        {
                           return value ? "true" : "false";
                        }

                        std::string encode( const char& value) const
                        {
                           return std::string{ value};
                        }

                        std::string encode( view::immutable::Binary value) const
                        {
                           // Binary data might be double-encoded
                           return common::transcode::base64::encode( value);
                        }

                        std::string encode( const platform::binary::type& value) const
                        {
                           return encode( view::binary::make( value));
                        }

                        using name = const char;

                        tree m_document;
                        std::vector<name*> m_name_stack; // for containers
                        std::vector<tree*> m_node_stack;

                     }; // Implementation

                  } // writer
               } // <unnamed>
            } // local
            
            namespace strict
            {
               serialize::Reader reader( const std::string& source) { return create::reader::strict::create< local::reader::Implementation>( source);}
               serialize::Reader reader( std::istream& source) { return create::reader::strict::create< local::reader::Implementation>( source);}
               serialize::Reader reader( const common::platform::binary::type& source) { return create::reader::strict::create< local::reader::Implementation>( source);}
            } // strict

            namespace relaxed
            {    
               serialize::Reader reader( const std::string& source) { return create::reader::relaxed::create< local::reader::Implementation>( source);}
               serialize::Reader reader( std::istream& source) { return create::reader::relaxed::create< local::reader::Implementation>( source);}
               serialize::Reader reader( const common::platform::binary::type& source) { return create::reader::relaxed::create< local::reader::Implementation>( source);}
            } // relaxed

            serialize::Writer writer( std::string& destination)
            {
               return serialize::create::writer::holder< local::writer::Implementation>( destination);
            }

            serialize::Writer writer( std::ostream& destination)
            {
               return serialize::create::writer::holder< local::writer::Implementation>( destination);
            }

            serialize::Writer writer( common::platform::binary::type& destination)
            {
               return serialize::create::writer::holder< local::writer::Implementation>( destination);
            }

         } // ini

         namespace create
         {
            namespace reader
            {
               template struct Registration< ini::local::reader::Implementation>;
            } // writer
            namespace writer
            {
               template struct Registration< ini::local::writer::Implementation>;
            } // writer
         } // create

      } // serialize
   } // common
} // casual


