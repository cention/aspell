#include <assert.h>
#include <string.h>
#include <ctype.h>

// POSIX includes
#include <dirent.h>

#include "can_have_error.hpp"
#include "config.hpp"
#include "emulation.hpp"
#include "errors.hpp"
#include "fstream.hpp"
#include "getdata.hpp"
#include "info.hpp"
#include "manager.hpp"
#include "string_list.hpp"
#include "string_emulation.hpp"

#if 0
#include "preload.h"
#define LT_NON_POSIX_NAMESPACE 1
#ifdef USE_LTDL
#include <ltdl.h>
#endif
#endif

using namespace pcommon;

namespace pcommon { // FIXME: Should this be a diffrent namespace?

  static void free_lt_handle(ManagerLtHandle h) 
  {
#ifdef USE_LTDL
    int s;
    s = lt_dlclose((lt_dlhandle)h);
    assert (s == 0);
    s = lt_dlexit();
    assert (s == 0);
#endif
  }

  extern "C" 
  Manager * libpspell_aspell_LTX_new_manager_class(ManagerLtHandle);

  PosibErr<Manager *> get_manager_class(Config * config)
  {
    String name = config->retrieve("module");
    assert(name == "aspell");
    return libpspell_aspell_LTX_new_manager_class(0);
#if 0
    unsigned int i; 
    for (i = 0; i != pspell_manager_funs_size; ++i) {
      if (strcmp(name.c_str(), pspell_manager_funs[i].name) == 0) {
	return (*pspell_manager_funs[i].fun)(config, 0);
      }
    }
  
#ifdef USE_LTDL
    int s = lt_dlinit();
    assert(s == 0);
    String libname;
    libname  = LIBDIR "/libpspell_";
    libname += name;
    libname += ".la";
    lt_dlhandle h = lt_dlopen (libname.c_str());
    if (h == 0)
      return (new CanHaveErrorImpl())
	->set_error(cant_load_module, name.c_str());
    lt_ptr_t fun = lt_dlsym (h, "new_pspell_manager_class");
    assert (fun != 0);
    CanHaveError * m = (*(NewManagerClass)(fun))(config, h);
    assert (m != 0);
    if (m->error_number() != 0)
      free_lt_handle(h);
    return m;
#else
    return (new CanHaveErrorImpl())
      ->set_error(cant_load_module, name.c_str());
#endif
#endif
  }

  // Note this writes all over str
  static void split_string_list(StringList * list, ParmString str)
  {
    const char * s0 = str;
    const char * s1;
    while (true) {
      while (*s0 != '\0' && isspace(*s0)) ++s0;
      if (*s0 == '\0') break;
      s1 = s0;
      while (!isspace(*s1)) ++s1;
      String temp(s0,s1-s0);
      list->add(temp);
      if (*s1 != '\0')
	s0 = s1 + 1;
    }
  }

  enum IsBetter {BetterMatch, WorseMatch, SameMatch};

  struct Better
  {
    unsigned int cur_rank;
    unsigned int best_rank;
    unsigned int worst_rank;
    virtual void init() = 0;
    virtual void set_best_from_cur() = 0;
    virtual void set_cur_rank() = 0;
    IsBetter better_match(IsBetter prev);  
    virtual ~Better();
  };

  Better::~Better() {}

  IsBetter Better::better_match(IsBetter prev)
  {
    if (prev == WorseMatch)
      return prev;
    set_cur_rank();
    if (cur_rank >= worst_rank)
      return WorseMatch;
    else if (cur_rank < best_rank)
      return BetterMatch;
    else if (cur_rank == best_rank)
      return prev;
    else // cur_rank > best_rank
      if (prev == SameMatch)
	return WorseMatch;
      else
	return BetterMatch;
  }

  struct BetterList : public Better
  {
    const char *         cur;
    ClonePtr<StringList> list;
    const char *         best;
    BetterList();
    void init();
    void set_best_from_cur();
    void set_cur_rank();
  };

  BetterList::BetterList() 
    : list(new_string_list()) 
  {
  }

  void BetterList::init() {
    Emulation<StringEmulation> es = list->elements();
    worst_rank = 0;
    while ( (es.next()) != 0)
      ++worst_rank;
    best_rank = worst_rank;
  }

  void BetterList::set_best_from_cur() 
  {
    best_rank = cur_rank;
    best = cur;
  }

  void BetterList::set_cur_rank() 
  {
    Emulation<StringEmulation> es = list->elements();
    const char * m;
    cur_rank = 0;
    while ( (m = es.next()) != 0 && strcmp(m, cur) != 0)
      ++cur_rank;
  }

  struct BetterSize : public Better
  {
    unsigned int         cur;
    const char *         cur_str;
    char                 req_type;
    unsigned int         requested;
    unsigned int         size;
    unsigned int         best;
    const char *         best_str;
    void init();
    void set_best_from_cur();
    void set_cur_rank();
  };


  void BetterSize::init() {
    worst_rank = 0xFFF;
    best_rank = worst_rank;
  }

  void BetterSize::set_best_from_cur() 
  {
    best_rank = cur_rank;
    best = cur;
    best_str = cur_str;
  }

  void BetterSize::set_cur_rank() 
  {
    int diff = cur - requested;
    int sign;
    if (diff < 0) {
      cur_rank = -diff;
      sign = -1;
    } else {
      cur_rank = diff;
      sign = 1;
    }
    cur_rank <<= 1;
    if ((sign == -1 && req_type == '+') || (sign == 1 && req_type == '-'))
      cur_rank |= 0x1;
    else if ((sign == -1 && req_type == '>') || (sign == 1 && req_type == '<'))
      cur_rank |= 0x100;
  }

  PosibErr<Config *> find_word_list(Config * c) 
  {
    Config * config = c->clone();
    ////////////////////////////////////////////////////////////////////
    //
    // Give first prefrence to an exect match for the language-country
    // code, then give prefrence to those in the alternate code list
    // in the order they are presented, then if there is no match get fine
    // look for one for just language.  If that fails give up.
    // Once the best matching code is found, try to find a matching
    // jargon if one exists, other wise look for one with no jargon.
    //

    BetterList b_code;
    BetterList b_jargon;
    BetterList b_module;
    BetterSize b_size;
    Better * better[4] = {&b_code,&b_jargon,&b_module,&b_size};
    const char * best_file = 0;

    //
    // retrieve and normalize code
    //
    const char * p;
    String code;
    PosibErr<String> str = config->retrieve("lang");
    p = str.data.c_str();
    code += tolower(*p++); code += tolower(*p++);
    String lang = code;
    bool have_country = false;
    if (*p == '-' || *p == '_') {
      ++p;
      have_country = true;
      code += '_'; code += toupper(*p++); code += toupper(*p++);
    }
  
    //
    // Retrive acceptable code search orders
    //
    String lang_country_list;
    if (have_country) {
      lang_country_list = code;
      lang_country_list += ' ';
    }
    String lang_only_list = lang;
    lang_only_list += ' ';

    // read retrive lang_country_list and lang_only_list from file(s)
    // FIXME: Write Me

    //
    split_string_list(b_code.list, lang_country_list);
    split_string_list(b_code.list, lang_only_list);
    b_code.init();

    //
    // Retrieve Jargon
    // 
    str = config->retrieve("jargon");
    if (str.get_err() != 0)
      b_jargon.list->add(str);
    b_jargon.list->add("");
    b_jargon.init();
    str.data.clear();

    //
    // Retrive module list
    //
    if (config->have("module"))
      b_module.list->add(config->retrieve("module"));
    else
      config->retrieve_list("module-search-order", b_module.list);
    b_module.init();

    //
    // Retrive size
    //
    str = config->retrieve("size");
    p = str.data.c_str();
    if (p[0] == '+' || p[0] == '-' || p[0] == '<' || p[0] == '>') {
      b_size.req_type = p[0];
      ++p;
    } else {
      b_size.req_type = '+';
    }
    if (!isdigit(p[0]) || !isdigit(p[1]) || p[2] != '\0')
      abort(); //FIXME: create an error condition here
    b_size.requested = atoi(p);
    b_size.init();

    //
    // 
    //

    DictInfoList * dlist = get_dict_info_list(config);
    DictInfoEmulation * dels = dlist->elements();
    const DictInfo * entry;

    while ( (entry = dels->next()) != 0) {

      b_code  .cur = entry->code;
      b_module.cur = entry->module->name;

      b_jargon.cur = entry->jargon;
    
      b_size.cur_str = entry->size_str;
      b_size.cur     = entry->size;

      //
      // check to see if we got a better match than the current
      // best_match if any
      //

      IsBetter is_better = SameMatch;
      for (int i = 0; i != 4; ++i)
	is_better = better[i]->better_match(is_better);
    
      if (is_better == BetterMatch) {
	for (int i = 0; i != 4; ++i)
	  better[i]->set_best_from_cur();
	best_file  = entry->file;
      }
    }

    delete dels;

    //
    // set config to best match
    //
    if (best_file != 0) {
      FStream f;
      RET_ON_ERR(f.open(best_file, "r"));
      String main_wl,flags;
      bool res = getdata_pair(f, main_wl, flags);
      f.close();
      if (!res) {
	delete config;
	return make_err(bad_file_format,  best_file, "");
      }
      config->replace("lang", b_code.best);
      config->replace("language-tag", b_code.best);
      config->replace("master", main_wl.c_str());
      config->replace("master-flags", flags.c_str());
      config->replace("module", b_module.best);
      config->replace("jargon", b_jargon.best);
      config->replace("size", b_size.best_str);
    } else {
      delete config;
      return make_err(no_wordlist_for_lang, code);
    }
    return config;
  }

  PosibErr<Manager *> new_manager(Config * c0) 
  {
    RET_ON_ERR_SET(find_word_list(c0), Config *, c);
    Manager * m = get_manager_class(c);
    PosibErrBase err = m->setup(c);
    if (err.has_err()) {
      delete m;
      return err;
    } else {
      return m;
    }
  }

  void delete_manager(Manager * m) 
  {
    ManagerLtHandle h = ((Manager *)(m))->lt_handle();
    delete m;
    if (h != 0) free_lt_handle(h);
  }
}