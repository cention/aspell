// This file is part of The New Aspell
// Copyright (C) 2002 by Kevin Atkinson under the GNU LGPL
// license version 2.0 or 2.1.  You should have received a copy of the
// LGPL license along with this library if you did not you can find it
// at http://www.gnu.org/.

#include "filter.hpp"
#include "parm_string.hpp"
#include "config.hpp"
#include "string_list.hpp"
#include "stack_ptr.hpp"
#include "string_enumeration.hpp"
#include "enumeration.hpp"

#include "iostream.hpp"

namespace acommon {

  //
  // filter modes
  // 

  const char * filter_modes = "none,url,email,sgml,tex";
  
  static const KeyInfo modes_module[] = {
    {"fm-email", KeyInfoList  , "url,email",0}
    , {"fm-none",  KeyInfoList  , "",         0}
    , {"fm-sgml",  KeyInfoList  , "url,sgml", 0}
    , {"fm-tex",   KeyInfoList  , "url,tex",  0}
    , {"fm-url",   KeyInfoList  , "url",      0}
  };

  class IndividualFilter;

  //
  // filter constructors
  //

  struct FilterEntry {
    const char * name;
    IndividualFilter * (* constructor) ();
  };
  
  IndividualFilter * new_url_filter ();
  IndividualFilter * new_email_filter ();
  IndividualFilter * new_tex_filter ();
  IndividualFilter * new_sgml_filter ();

  static FilterEntry standard_filters[] = {
    {"url",   new_url_filter},
    {"email", new_email_filter},
    {"tex", new_tex_filter},
    {"sgml", new_sgml_filter}
  };
  static unsigned int standard_filters_size 
  = sizeof(standard_filters)/sizeof(FilterEntry);

  //
  // config options for the filters
  //
  
  extern const KeyInfo * email_options_begin;
  extern const KeyInfo * email_options_end;

  extern const KeyInfo * tex_options_begin;
  extern const KeyInfo * tex_options_end;

  extern const KeyInfo * sgml_options_begin;
  extern const KeyInfo * sgml_options_end;

  static ConfigModule filter_modules[] =  {
    {"fm", modes_module, modes_module + sizeof(modes_module)/sizeof(KeyInfo)},
    {"email", email_options_begin, email_options_end},
    {"tex",   tex_options_begin,   tex_options_end},
    {"sgml",  sgml_options_begin, sgml_options_end}
  };

  // these variables are used in the new_config function and
  // thus need external linkage
  const ConfigModule * filter_modules_begin = filter_modules;
  const ConfigModule * filter_modules_end   
  = filter_modules + sizeof(filter_modules)/sizeof(ConfigModule);

  //
  // actual code
  //

  IndividualFilter * new_individual_filter(ParmString);

  PosibErr<Filter *> new_filter(Speller * speller, 
				Config * config)
  {
    StackPtr<Filter> filter(new Filter());
    filter->setup(speller, config);
    StackPtr<StringList> sl(new_string_list());
    filter->config()->retrieve_list("filter", sl);
    Enumeration<StringEnumeration> els = sl->elements();
    const char * filter_name;
    while ( (filter_name = els.next()) != 0) 
    {
      IndividualFilter * ifilter = new_individual_filter(filter_name);
      filter->add_filter(ifilter);
    }
    return filter.release();
  }
  
  IndividualFilter * new_individual_filter(ParmString filter_name)
  {
    unsigned int i = 0;
    while (i != standard_filters_size) {
      if (standard_filters[i].name == filter_name)
	return (*standard_filters[i].constructor)();
      ++i;
    }
    return 0;
  }

}