#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <locale.h>

#include "dirs.h"

#include "config.hpp"
#include "errors.hpp"
#include "file_util.hpp"
#include "fstream.hpp"
#include "getdata.hpp"
#include "itemize.hpp"
#include "mutable_container.hpp"
#include "posib_err.hpp"
#include "string_map.hpp"

#define DEFAULT_LANG "en_US"

namespace pcommon {

  static const Module a_module = Module();
  
  typedef Notifier * NotifierPtr;
  
  Config::Config(ParmString name,
		 const KeyInfo * mainbegin, 
		 const KeyInfo * mainend)
    : name_(name)
    , data_(new_string_map())
    , attached_(0)
    , md_info_list_index(-1)
  {
    kmi.main_begin = mainbegin;
    kmi.main_end   = mainend;
    kmi.extra_begin = 0;
    kmi.extra_end   = 0;
    kmi.modules_begin = &a_module;
    kmi.modules_end   = &a_module;
    notifier_list = new NotifierPtr[1];
    notifier_list[0] = 0;
  }

  Config::~Config() {
    delete data_;
    delete[] notifier_list;
  }

  Config::Config(const Config & other) 
    : name_(other.name_), attached_(0), kmi(other.kmi) 
  {
    data_ = other.data_->clone();
    notifier_list = new NotifierPtr[1];
    notifier_list[0] = 0;
  }

  Config & Config::operator= (const Config & other)
  {
    delete data_;
    delete[] notifier_list;
    attached_ = 0;
    kmi = other.kmi;
    data_->assign(other.data_);
    notifier_list = new NotifierPtr[1];
    notifier_list[0] = 0;
    return *this;
  }

  Config * Config::clone() const {
    return new Config(*this);
  }

  void Config::assign(const Config * other) {
    *this = *(const Config *)(other);
  }

  void Config::set_modules(const Module * modbegin, 
				 const Module * modend)
  {
    kmi.modules_begin = modbegin;
    kmi.modules_end   = modend;
  }

  void Config::set_extra(const KeyInfo * begin, 
			       const KeyInfo * end) 
  {
    kmi.extra_begin = begin;
    kmi.extra_end   = end;
  }

  //
  // Notifier methods
  //

  NotifierEmulation * Config::notifiers() const 
  {
    return new NotifierEmulation(notifier_list);
  }
  
  bool Config::add_notifier(Notifier * n) 
  {
    Notifier * * i = notifier_list;

    while (*i != 0 && *i != n)
      ++i;

    if (*i != 0) {
    
      return false;
    
    } else {

      Notifier * * temp = notifier_list;
      size_t old_size = i - temp;
      notifier_list = new NotifierPtr[old_size + 2];
      unsigned int j = 0;
      for (; j != old_size; ++j)
	notifier_list[j] = temp[j];
      notifier_list[j] = n;
      notifier_list[j+1] = 0;
      return true;

    }
  }

  bool Config::remove_notifier(const Notifier * n) 
  {
    Notifier * * i = notifier_list;
    while (*i != 0 && *i != n)
      ++i;
    if (*i == 0) {
      return false;
    } else {
      Notifier * * temp = notifier_list;
      size_t old_size = i - temp;
      notifier_list = new NotifierPtr[old_size];
      unsigned j = 0;
      for (; j != old_size - 1; ++j) {
	if (temp[j] != n)
	  notifier_list[j] = temp[j];
      }
      notifier_list[j] = 0;
      return true;
    }
    return true;
  }

  bool Config::replace_notifier(const Notifier * o, 
				      Notifier * n) 
  {
    Notifier * * i = notifier_list;
    while (*i != 0 && *i != n)
      ++i;
    if (*i == 0) {
      return false;
    } else {
      *i = n;
      return true;
    }
  }

  bool Config::have(ParmString key) const 
  {
    const char * value = data_->lookup(key);
    if (value == 0 || value[0] == '\x01') {
      return false;
    } else {
      return true;
    }
  }

  //
  // retrive methods
  //

  PosibErr<bool> Config::retrieve_bool(ParmString key) const
  {
    RET_ON_ERR_SET(retrieve(key), String, str);
    return str[0] == 't';
  }

  PosibErr<int> Config::retrieve_int(ParmString key) const
  {
    RET_ON_ERR_SET(retrieve(key), String, str);
    int i;
    sscanf(str.c_str(), "%i", &i);
    return i;
  }

  PosibErr<String> Config::retrieve(ParmString key) const
  {
    const char * value = data_->lookup(key);
    if (value != 0) {
      if (value[0] == '\x01')
	++value;
      return String(value);
    } else {
      return get_default(key);
    }
  }

  PosibErr<void> Config::retrieve_list(ParmString key, 
				       MutableContainer * m) const
  {
    RET_ON_ERR_SET(get_default(key), String, def);
    const char * value = data_->lookup(key);
    if (value != 0) {
      def += ',';
      def += data_->lookup(key);
    }
    itemize(def, *m);
    return no_err;
  }

  static const KeyInfo * find(ParmString key, 
			      const KeyInfo * i, 
			      const KeyInfo * end) 
  {
    while (i != end) {
      if (strcmp(key, i->name) == 0)
	return i;
      ++i;
    }
    return i;
  }

  static const Module * find(ParmString key, 
			     const Module * i, 
			     const Module * end) 
  {
    while (i != end) {
      if (strcmp(key, i->name) == 0) 
	return i;
      ++i;
    }
    return i;
  }

  const char * Config::base_name(ParmString name)
  {
    const char * c = strchr(name, '-');
    unsigned int p = c ? c - name : -1;
    if ((p == 3 && (strncmp(name, "add",p) == 0 
		    || strncmp(name, "rem",p) == 0))
	|| (p == 4 && strncmp(name, "dont",p) == 0)) 
      return name + p + 1;
    else
      return name;
  }

  PosibErr<const KeyInfo *> Config::keyinfo(ParmString key) const
  {
    PosibErr<const KeyInfo *> ret;
    const KeyInfo * i;
  
    i = pcommon::find(key, kmi.main_begin, kmi.main_end);
    if (i != kmi.main_end) return ret = i;
  
    i = pcommon::find(key, kmi.extra_begin, kmi.extra_end);
    if (i != kmi.extra_end) return ret = i;
  
    const char * h = strchr(key, '-');

    if (h == 0) 
      return ret.prim_err(unknown_key, key);

    String k(key,h-key);
    const Module * j = pcommon::find(k, 
				     kmi.modules_begin, 
				     kmi.modules_end);
    if (j == kmi.modules_end)
      return ret.prim_err(unknown_key, key);
  
    i = pcommon::find(key, j->begin, j->end);
    if (i != j->end) return ret = i;
  
    return ret.prim_err(unknown_key, key);
  }

  PosibErr<String> Config::get_default(ParmString key) const
  {
    RET_ON_ERR_SET(keyinfo(key), const KeyInfo *, ki);

    bool   in_replace = false;
    String final_str;
    String replace;
    const char * i = ki->def;
    if (*i == '!') { // special cases
      ++i;
    
      if (strcmp(i, "lang") == 0) do {
	if (have("master")) {
	  final_str = "<unknown>";
	  break;
	}
	final_str = DEFAULT_LANG;
	const char * lang = getenv("LANGUAGE");
	if (lang == 0)
	  lang = getenv("LANG");
	if (lang == 0) break;
	i = lang;
	if (! (islower(i[0]) && islower(i[1])) ) break;
	final_str.assign(i, 2);
	i += 2;
	if (! (i[0] == '_' || i[0] == '-')) break;
	i += 1;
	if (! (isupper(i[0]) && isupper(i[1])) ) break;
	final_str += '_';
	final_str.append(i, 2);

      } while (false); else if (strcmp(i, "special") == 0) {

	// do nothing

      } else {
      
	abort(); // this should not happen
      
      }
    
    } else for(; *i; ++i) {
    
      if (!in_replace) {

	if (*i == '<') {
	  in_replace = true;
	} else {
	  final_str += *i;
	}

      } else { // in_replace
      
	if (*i == '/' || *i == ':' || *i == '|' || *i == '#' || *i == '^') {
	  char sep = *i;
	  String second;
	  ++i;
	  while (*i != '\0' && *i != '>') second += *i++;
	  if (sep == '/') {
	    String s1 = retrieve(replace);
	    String s2 = retrieve(second);
	    final_str += add_possible_dir(s1, s2);
	  } else if (sep == ':') {
	    String s1 = retrieve(replace);
	    final_str += add_possible_dir(s1, second);
	  } else if (sep == '#') {
	    String s1 = retrieve(replace);
	    assert(second.size() == 1);
	    unsigned int s = 0;
	    while (s != s1.size() && s1[s] != second[0]) ++s;
	    final_str.append(s1, s, String::npos);
	  } else if (sep == '^') {
	    String s1 = retrieve(replace);
	    String s2 = retrieve(second);
	    final_str += figure_out_dir(s1, s2);
	  } else { // sep == '|'
	    assert(replace[0] == '$');
	    const char * env = getenv(replace.c_str()+1);
	    final_str += env ? env : second;
	  }
	  replace = "";
	  in_replace = false;

	} else if (*i == '>') {

	  final_str += retrieve(replace);
	  replace = "";
	  in_replace = false;

	} else {

	  replace += *i;

	}

      }
      
    }
    return final_str;
  }


#define notify_all(ki, value, fun)        \
  do {                                    \
    Notifier * * i = notifier_list; \
    while (*i != 0) {                     \
      (*i)->fun(ki,value);                \
      ++i;                                \
    }                                     \
  } while (false)


  class NotifyListBlockChange : public MutableContainer 
  {
    const KeyInfo * key_info;
    Notifier * * notifier_list;
  public:
    NotifyListBlockChange(const KeyInfo * ki, Notifier * * n);
    bool add(ParmString);
    bool remove(ParmString);
    void clear();
  };

  NotifyListBlockChange::
  NotifyListBlockChange(const KeyInfo * ki, Notifier * * n)
    : key_info(ki), notifier_list(n) {}

  bool NotifyListBlockChange::add(ParmString v) {
    notify_all(key_info, v, item_added);
    return true;
  }

  bool NotifyListBlockChange::remove(ParmString v) {
    notify_all(key_info, v, item_removed);
    return true;
  }

  void NotifyListBlockChange::clear() {
    notify_all(key_info, 0, all_removed);
  }

  PosibErr<void> Config::replace(ParmString k, ParmString value) {
    if (strcmp(value,"<default>") == 0)
      return remove(k);

    const char * key;
    const char * i = strchr(k, '-');
    int p = (i == 0 ? -1 : i - k);
    if ((p == 3 && (strncmp(k, "add",p) == 0 
		    || strncmp(k, "rem",p) == 0))
	|| (p == 4 && strncmp(k, "dont",p) == 0))
      {
	key = k + p + 1;
	if (strncmp(key, "all-", 4) == 0) {
	  key = key + 4;
	  p = 7;
	}
      } else {
	key = k;
	p = 0;
      }
  
    RET_ON_ERR_SET(keyinfo(key), const KeyInfo *, ki);

    if (ki->otherdata[1] && attached_)
      return make_err(cant_change_value, key);
  
    assert(ki->def != 0); // if null this key should never have values
    // directly added to it

    int num;
    switch (ki->type) {
    
    case KeyInfoBool:
    
      if (p == 4 || (p == 0 && strcmp(value,"false") == 0)) {

	data_->replace(key, "false");
	notify_all(ki, false, item_updated);
	return no_err;

      } else if (p != 0) {

	return make_err(unknown_key,  k);

      } else if (value[0] == '\0' || strcmp(value,"true") == 0) {

	data_->replace(key, "true");
	notify_all(ki, true, item_updated);
	return no_err;

      } else {

	return make_err(bad_value, key, value, "either \"true\" or \"false\"");

      }
      break;
      
    case KeyInfoString:
      
      if (p == 0) {

	data_->replace(key,value);
	notify_all(ki, value, item_updated);
	return no_err;
      
      } else {
      
	return make_err(unknown_key,  key);
      
      }
      break;
      
    case KeyInfoInt:

      if (p == 0 && sscanf(value, "%i", &num) == 1 && num >= 0) {

	data_->replace(key,value);
	notify_all(ki, num, item_updated);
	return no_err;

      } else if (p != 0) {

	return make_err(unknown_key, key);

      } else {

	return make_err(bad_value, key, value, "a positive integer");

      }
      break;

    case KeyInfoList:

      char a;
      if (p == 0) {
	abort(); //FIXME
	//return ret.prim_err(list_set, key); 
      } else if (p == 7) {        // prefix must be "rem-all-"
	if (value[0] != '\0') {
	  return make_err(bad_value, k, value, "nothing");
	}
	a = '!';
      } else if (k[0] == 'a') { // prefix must be "add-"
	a = '+';
      } else {                  // prefix must be "rem-"
	a = '-';
      }

      if (a != '!') {
	i = data_->lookup(key);
	if (i == 0) i = "";
	String s = i;
	s += ',';
	s += a;
	s += value;
	data_->replace(key, s);
      } else {
	data_->replace(key, "!");
      }

      switch (a) {
      case '!': 
	notify_all(ki, value, all_removed);  
	break;
      case '+': 
	notify_all(ki, value, item_added);
	break;
      case '-': 
	notify_all(ki, value, item_removed);
	break;
      }

      break;
    }

    return no_err;
  }

  PosibErr<bool> Config::remove (ParmString key) {
  
    RET_ON_ERR_SET(keyinfo(key), const KeyInfo *, ki);

    if (ki->otherdata[1] && attached_)
      return make_err(cant_change_value, key);
  
    assert(ki->def != 0); // if null this key should never have values
    // directly added to it

    bool success = data_->remove(key);

    switch (ki->type) {

    case KeyInfoString:

      notify_all(ki, retrieve(key), item_updated);
      break;
    
    case KeyInfoBool:

      notify_all(ki, retrieve_bool(key), item_updated);
      break;

    case KeyInfoInt:

      notify_all(ki, retrieve_int(key), item_updated);
      break;

    case KeyInfoList:
    
      NotifyListBlockChange n(ki, notifier_list);
      retrieve_list(key, &n);
      break;
    }

    return success;
  }

  StringPairEmulation * Config::elements() 
  {
    return data_->elements();
  }


  /////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////

  class PossibleElementsEmul : public KeyInfoEmulation
  {
  private:
    bool include_extra;
    const Config * cd;
    const KeyInfo * i;
    const Module  * m;
  public:
    PossibleElementsEmul(const Config * d, bool ic)
      : include_extra(ic), cd(d), i(d->kmi.main_begin), m(0) {}

    KeyInfoEmulation * clone() const {
      return new PossibleElementsEmul(*this);
    }

    void assign(const KeyInfoEmulation * other) {
      *this = *(const PossibleElementsEmul *)(other);
    }

    const KeyInfo * next() {
      if (i == cd->kmi.main_end) {
	if (include_extra)
	  i = cd->kmi.extra_begin;
	else
	  i = cd->kmi.extra_end;
      }
      
      if (i == cd->kmi.extra_end) {
	m = cd->kmi.modules_begin;
	if (m == cd->kmi.modules_end) return 0;
	else i = m->begin;
      }

      if (m == 0)
	return i++;

      if (m == cd->kmi.modules_end)
	return 0;

      if (i == m->end) {
	++m;
	if (m == cd->kmi.modules_end) return 0;
	else i = m->begin;
      }

      return i++;
    }

    bool at_end() const {
      return (m == cd->kmi.modules_end);
    }
  };

  KeyInfoEmulation *
  Config::possible_elements(bool include_extra)
  {
    return new PossibleElementsEmul(this, include_extra);
  }

  class ListDump : public MutableContainer 
  {
    FILE * out;
    const char * name;
  public:
    ListDump(FILE * o, ParmString n) 
      : out(o), name(n) {}
    bool add(ParmString d) {
      fputs("add-", out);
      fputs(name,   out);
      putc (' ',    out);
      fputs(d,      out);
      putc ('\n',   out);
      return true;
    }
    bool remove(ParmString d) {
      fputs("rem-", out);
      fputs(name,   out);
      putc (' ',    out);
      fputs(d,      out);
      putc ('\n',   out);
      return true;
    }
    void clear() {
      fputs("rem-all-",out);
      fputs(name,      out);
      putc ('\n',      out);
    }
  };

  void Config::write_to_stream(FILE * out, 
				     bool include_extra) 
  {
    KeyInfoEmulation * els = possible_elements(include_extra);
    const KeyInfo * i;
    while ((i = els->next()) != 0) {
      if (i->desc == 0) continue;
      fputs("# "                                              , out);
      fputs(i->type ==  KeyInfoList ? "add|rem-" : ""   , out);
      fputs(i->name                                           , out);
      fputs(" descrip: "                                      , out);
      fputs(i->def == 0 ? "(action option) " : ""             , out);
      fputs(i->desc                                           , out);
      fputs("\n"                                              , out);
      if (i->def != 0) {
	fputs("# "        , out);
	fputs(i->name     , out);
	fputs(" default: ", out);
	fputs(i->def      , out);
	fputs("\n"        , out);
	String val = retrieve(i->name);
	if (i->type != KeyInfoList) {
	  fputs("# "        , out);
	  fputs(i->name     , out);
	  fputs(" current: ", out);
	  fputs(val.c_str() , out);
	  fputs("\n"        , out);
	  if (have(i->name)) {
	    fputs(i->name    , out);
	    fputs(" "        , out);
	    fputs(val.c_str(), out);
	    fputs("\n"       , out);
	  }
	} else {
	  const char * value = data_->lookup(i->name);
	  if (value != 0) {
	    ListDump ld(out, i->name);
	    itemize(value, ld);
	  }
	}
      }
      fputs("\n\n", out);
    }
    delete els;
  }

  PosibErr<void> Config::read_in(IStream & in) 
  {
    String key,value;
    while (getdata_pair(in, key, value)) {
      RET_ON_ERR(replace(key, value));
    }
    return no_err;
  }

  PosibErr<void> Config::read_in_file(ParmString file) {
    FStream in;
    RET_ON_ERR(in.open(file, "r"));
    return read_in(in);
  }

  PosibErr<void> Config::read_in_string(ParmString str) {
    StringIStream in(str);
    return read_in(in);
  }

  void Config::merge(const Config & other) {
    KeyInfoEmulation * els = possible_elements();
    bool diff_name = strcmp(name(), other.name()) != 0;
    const KeyInfo * k;
    const KeyInfo * other_k;
    const char * other_name;
    String this_value;
    String other_value;
    while ( (k = els->next()) != 0) {
      if (diff_name && k->otherdata[0] == 'p'
	  && strncmp(k->name, other.name_.c_str(), other.name_.size())
	  && k->name[other.name_.size()] == '_')
	other_name = k->name + other.name_.size();
      else
	other_name = k->name;

      other_k = other.keyinfo(other_name);
      if (diff_name && other_k && other_k->otherdata[0] == 'r') continue;
      // the other key is a prefix key so skip it
      // when this is a prefix key than this key
      // would be prefix_
    
      if (other_k != 0 && 
	  strcmp(k->def, other_k->def) == 0 
	  && !other.have(other_name))
	continue;
      {
	PosibErr<String> pe = other.retrieve(other_name);
	if (pe.get_err() != 0) continue;
	// if an err then this key does not exist in the other
	// table.
	other_value = pe;
      }
      if (other_value == "(default)") continue;
      this_value = retrieve(k->name);
      if (this_value == other_value && 
	  !other.have(k->name)) continue;
      // if the two values match there is no need to insert it into the
      // table unless the other value is specificly set
      if (k->type != KeyInfoList) {
	data_->replace(k->name, other_value);
      } else {
	String new_value;
	if (other_value[0] != '!') {
	  new_value  = this_value;
	  new_value += ',';
	}
	new_value += other_value;
	data_->replace(k->name, new_value);
      }
    }
    delete els;
  }

  PosibErr<void> Config::read_in_settings(const Config * override)
  {
    // FIXME: make this more robust.  
    // Catch errors and atatched there source of origin

    {
      PosibErrBase pe = read_in_file(retrieve("conf-path"));
      if (pe.has_err() && !pe.has_err(cant_read_file))
	return pe;
    }

    {
      PosibErrBase pe = read_in_file(retrieve("per-conf-path"));
      if (pe.has_err() && !pe.has_err(cant_read_file))
	return pe;
    }

    const char * env = getenv("PSPELL_CONF");
    if (env != 0)
      RET_ON_ERR(read_in_string(env));

    if (override != 0)
      merge(*override);

    return no_err;
  }


#define CANT_CHANGE 1

#ifdef ENABLE_WIN32_RELOCATABLE
#  define HOME_DIR "<prefix>"
#  define PERSONAL "<actual-lang>.pws"
#  define REPL     "<actual-lang>.prepl"
#else
#  define HOME_DIR "<$HOME|./>"
#  define PERSONAL ".aspell.<actual-lang>.pws"
#  define REPL     ".aspell.<actual-lang>.prepl"
#endif

  static const KeyInfo config_keys[] = {
    {"actual-dict-dir", KeyInfoString, "<dict-dir^master>", 0}
    , {"actual-lang",     KeyInfoString, "<lang>", 0}
    , {"conf",     KeyInfoString, "aspell.conf",  "main configuration file"             , {0, CANT_CHANGE}}
    , {"conf-dir", KeyInfoString, CONF_DIR,      "location of main configuration file" ,{0, CANT_CHANGE}}
    , {"conf-path",     KeyInfoString, "<conf-dir/conf>",     0}
    , {"data-dir", KeyInfoString, DATA_DIR,        "location of language data files", "r"}
    , {"dict-dir", KeyInfoString, DICT_DIR,        "location of the main word list"      }
    , {"encoding",   KeyInfoString, "iso8859-1", "encoding to expect data to be in"}
    , {"extra-dicts", KeyInfoList, "", "extra dictionaries to use"}
    , {"home-dir", KeyInfoString, HOME_DIR,   "location for personal files" }
    , {"ignore",   KeyInfoInt   , "1",            "ignore words <= n chars"             }
    , {"ignore-accents" , KeyInfoBool, "false", "ignore accents when checking words"}
    , {"ignore-case", KeyInfoBool  , "false",     "ignore case when checking words"}
    , {"ignore-repl", KeyInfoBool  , "false",     "ignore commands to store replacement pairs"}
    , {"jargon",     KeyInfoString, "", "extra information for the word list"}
    , {"keyboard", KeyInfoString, "standard", "keyboard definition to use for typo analysis"}
    , {"lang", KeyInfoString, "<language-tag>", "language code"}
    , {"language-tag", KeyInfoString, "<$LANG|en_US>", "deprecated, use lang instead"}
    , {"local-data-dir", KeyInfoString, "<actual-dict-dir>",        "location of local language data files"     }
    , {"master",        KeyInfoString, "", "base name of the main dictionary to use"}
    , {"master-flags",  KeyInfoString, "", 0}
    , {"master-path",   KeyInfoString, "<dict-dir/master>",   0}
    , {"module",        KeyInfoString, "", ""}
    , {"module-search-order", KeyInfoList, "aspell,ispell", ""}
    , {"per-conf", KeyInfoString, ".aspell.conf", "personal configuration file",{0, CANT_CHANGE}}
    , {"per-conf-path", KeyInfoString, "<home-dir/per-conf>", 0}
    , {"personal", KeyInfoString, PERSONAL,   "personal word list file name"}
    , {"personal-path", KeyInfoString, "<home-dir/personal>", 0}
    , {"prefix",   KeyInfoString, PREFIX, "prefix directory", {0, CANT_CHANGE}}
    , {"repl",     KeyInfoString, REPL, "replacements list file name" }
    , {"repl-path",     KeyInfoString, "<home-dir/repl>",     0}
    , {"run-together",        KeyInfoBool,  "false", "consider run-together words legal"}
    , {"run-together-limit",  KeyInfoInt,   "8", "maxium numbers that can be strung together"}
    , {"run-together-min",    KeyInfoInt,   "3", "minimal length of interior words"}
    , {"run-together-specified", KeyInfoBool, "false", 0}
    , {"save-repl", KeyInfoBool  , "true", "save replacement pairs on save all"}
    , {"set-prefix", KeyInfoBool, "true", "set the prefix based on executable location", {0, CANT_CHANGE}} 
    , {"size",          KeyInfoString, "+60", "size of the word list"}
    , {"spelling",   KeyInfoString, "", "no longer used"}
    , {"strip-accents" , KeyInfoBool, "false", "strip accents from word lists"}
    , {"sug-mode",   KeyInfoString, "normal", "suggestion mode"}
    , {"word-list-path", KeyInfoList, DATA_DIR, "Search path for word list information files"}

  };

  const KeyInfo * pspell_config_impl_keys_begin = config_keys;
  const KeyInfo * pspell_config_impl_keys_end   
  = config_keys + sizeof(config_keys)/sizeof(KeyInfo);

  Config * new_config() {
    return new pcommon::Config("pspell",
			       pspell_config_impl_keys_begin,
			       pspell_config_impl_keys_end);
  }

}