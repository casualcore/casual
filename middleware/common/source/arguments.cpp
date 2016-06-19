//!
//! arguments.cpp
//!
//! Created on: Nov 21, 2015
//!     Author: Lazan
//!

#include "common/arguments.h"


namespace casual
{
   namespace common
   {
      namespace argument
      {


         namespace visitor
         {
            Base::~Base() = default;

            void Base::visit( const internal::base_directive& option)
            {
               do_visit( option);
            }

         } // visitor


         namespace option
         {
            bool Holder::option( const std::string& option) const
            {
               return m_option->option( option);
            }
            void Holder::assign( const std::string& option, std::vector< std::string>&& values) const
            {
               m_option->assign( option, std::move( values));
            }
            bool Holder::consumed() const
            {
               return m_option->consumed();
            }
            void Holder::dispatch() const
            {
               m_option->dispatch();
            }

            bool Holder::valid() const
            {
               return m_option->valid();
            }

            const std::vector< std::string>& Holder::options() const
            {
               return m_option->options();
            }

            const std::string& Holder::description() const
            {
               return m_option->description();
            }
            internal::value_cardinality Holder::cardinality() const
            {
               return m_option->cardinality();
            }

            void Holder::visit( visitor::Base& visitor) const
            {
               m_option->visit( visitor);
            }


         } // option

         namespace local
         {
            namespace
            {
               struct Option
               {
                  Option( const std::string& option) : option( option) {}

                  template< typename T>
                  bool operator () ( T&& value) const
                  {
                     return value.option( option);
                  }
               private:
                  const std::string& option;
               };
            } // <unnamed>
         } // local


         namespace internal
         {
            base_directive::base_directive( std::vector< std::string> options, std::string description)
            : m_options{ std::move( options)}, m_description{ std::move( description)}
            {

            }

            const std::vector< std::string>& base_directive::options() const { return m_options;}
            const std::string& base_directive::description() const { return m_description;}

            bool base_directive::option( const std::string& option) const
            {
               return ! range::find( m_options, option).empty();
            }

            void base_directive::visit( visitor::Base& visitor) const
            {
               visitor.visit( *this);
            }
         } // internal


         Group::Group( options_type options) : m_options{ std::move( options)}
         {

         }

         void Group::visit( visitor::Base& visitor) const
         {
            range::for_each( m_options, std::bind( &option::Holder::visit, std::placeholders::_1, std::ref( visitor)));
         }


         bool Group::option( const std::string& option) const
         {
            return range::any_of( m_options, local::Option{ option});
         }

         void Group::assign( const std::string& option, std::vector< std::string>&& values)
         {
            auto found = range::find_if( m_options, local::Option{ option});

            if( found)
            {
               found->assign( option, std::move( values));
            }
         }

         bool Group::consumed() const
         {
            return range::all_of( m_options, std::mem_fn( &option::Holder::consumed));
         }

         void Group::dispatch() const
         {
            range::for_each( m_options, std::mem_fn( &option::Holder::dispatch));
         }


         bool Group::valid() const
         {
            return range::all_of( m_options, std::mem_fn( &option::Holder::valid));
         }


         namespace local
         {
            namespace
            {

               namespace format
               {
                  std::ostream& cardinality( std::ostream& out, internal::value_cardinality cardinality)
                  {
                     if( cardinality.min == cardinality.max)
                     {
                        if( cardinality.min == 1)
                        {
                           out << " <value>";
                        }
                        else if( cardinality.min > 1 && cardinality.min <= 3)
                        {
                           for( auto count = cardinality.min; count > 0; --count)
                           {
                              out << " <value>";
                           }
                        }
                        else if( cardinality.min > 3)
                        {
                           out << " <value>..{ " << cardinality.min << "}";
                        }
                     }
                     else
                     {
                        out << " <value> " << cardinality.min << "..";
                        if( cardinality.max == std::numeric_limits< std::size_t>::max())
                        {
                           out << '*';
                        }
                        else
                        {
                           out << cardinality.max;
                        }
                     }
                     return out;
                  }


                  std::ostream& description( std::ostream& out, const std::string& description, int indent = 6)
                  {
                     for( const auto& row : string::split( description, '\n'))
                     {
                        out << std::string( indent, ' ') << row << "\n";
                     }
                     return out;
                  }

                  template< typename T>
                  void help( std::ostream& out, const T& directive)
                  {

                     auto option_string = string::join( directive.options(), ", ");

                     if( option_string.empty())
                     {
                        option_string = "<empty>";
                     }

                     out << "   " << terminal::color::white << option_string;
                     format::cardinality( out, directive.cardinality()) << std::endl;

                     format::description( out, directive.description()) << std::endl;
                  }

               } // format

               class Help : public visitor::Base
               {
               public:

                  Help( const Group* group, std::string description, std::ostream& out = std::clog)
                     : m_group( group), m_description( std::move( description)), m_out( out)
                  {
                  }

                  //Help( const Group* group) : Help( group, "") {}

                  void do_visit( const internal::base_directive& option) override
                  {
                     format::help( m_out, option);
                  }

                  void do_visit( const Group& group) override
                  {
                     //format::help( m_out, group);
                     std::clog << "---group ---" << std::endl;

                     //internal::format::help( m_out, *this);

                  }

                  void operator () ()
                  {
                     m_out << "NAME\n   ";
                     m_out << terminal::color::white << file::name::base( process::path());

                     m_out << "\n\nDESCRIPTION\n";

                     format::description( m_out, m_description, 3) << "\nOPTIONS\n";

                     m_group->visit( *this);
                  }

               private:
                  const Group* m_group = nullptr;
                  const std::string m_description;
                  std::ostream& m_out;
               };

            } // <unnamed>
         } // local

      } // argument



      Arguments::Arguments( options_type&& options) : Arguments( "", std::move( options))
      {
      }

      Arguments::Arguments( std::string description, options_type options) : Arguments( std::move( description), { "--help"}, std::move( options))
      {
      }

      Arguments::Arguments( std::string description, std::vector< std::string> help_option, options_type options)
       : Group{ std::move( options)}
      {
         m_options.push_back(
               argument::directive( std::move( help_option), "Shows this help", argument::local::Help{ this, std::move( description)}));
      }

      Arguments::Arguments( argument::no::Help, options_type options)
         : Group{ std::move( options)}
      {
      }

      void Arguments::parse( int argc, char** argv)
      {
         if( argc > 0)
         {
            std::vector< std::string> arguments{ argv + 1, argv + argc};
            parse( argv[ 0], arguments);
         }
      }

      void Arguments::parse( const std::vector< std::string>& arguments)
      {
         parse( process::path(), arguments);
      }


      void Arguments::parse( const std::string& process, const std::vector< std::string>& arguments)
      {
         process::path( process);


         //
         // start divide and conquer the arguments and try to find handlers for'em
         //

         auto argumentRange = range::make( arguments);

         while( argumentRange)
         {

            //
            // Try to find a handler for this argument
            //
            auto found = range::find_if( m_options, argument::local::Option{ *argumentRange});

            if( ! found)
            {
               throw exception::invalid::Argument{ "invalid argument: " + *argumentRange};
            }

            //
            // pin the current found argument, and consume it so we can continue to
            // search
            //
            auto argument = *argumentRange;
            ++argumentRange;

            //
            // Find the end of values associated with this option
            //
            auto slice = range::divide_first( argumentRange, m_options, []( const std::string& option, const argument::option::Holder& holder)
               {
                  return holder.option( option);
               });

            found->assign( argument, range::to_vector( std::get< 0>( slice)));

            if( ! found->valid())
            {
               throw exception::invalid::Argument{ "invalid values for: " + argument};
            }

            argumentRange = std::get< 1>( slice);
         }

         dispatch();
      }

      const std::string& Arguments::process() const
      {
         return process::path();
      }



   } // common


} // casual
