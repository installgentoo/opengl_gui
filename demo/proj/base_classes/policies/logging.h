#pragma once
#include "utility.h"
#include <limits>
#include <ostream>

namespace code_policy
{

template<class m_policy, int m_init_level>
struct LoggingControl : m_policy, CUNIQUE
{
  enum Level { ERROR = 0, WARNING = 1, ALL = 2 };

  static LoggingControl& Get() {
    static LoggingControl s_control;
    return s_control;
  }

  constexpr static bool IsLevel(Level level) {
    return m_init_level >= level;
  }

  template<class arg_type> LoggingControl& operator<<(arg_type arg) {
    this->stream()<<arg;
    return *this;
  }
};


struct FilePolicy {
  std::ostream& stream();
  void Close();

private:
  std::ofstream *m_log_file = nullptr;
};

struct CoutPolicy {
  std::ostream& stream()const;
  void Close();
};


#ifdef NDEBUG
  typedef LoggingControl<CoutPolicy, 2> Log;
#else
  typedef LoggingControl<CoutPolicy, 2> Log;
#endif

#ifdef NDEBUG
  #define CTERMINATE Log::Get().Close(); exit(-1);
  #define CASSERT(var, text)
  #define CDEBUGBLOCK(f)
  #define GLCHECK_RET(f) f;
#else
  #define CASSERT(var, text) { if(!(var)) { Log::Get()<<"A "<<text<<" |in expr ("<<#var<<"), in "<<__FILE__<<":"<<__LINE__<<"\n"; CTERMINATE } }
  #define CDEBUGBLOCK(f) f
  #define GLCHECK_RET(f) f; { Val errCode = glGetError(); if(errCode != GL_NO_ERROR) { Log::Get()<<"W OpenGL error "<<glCodeToError(errCode)<<" |in func ("<<#f<<"), in "<<__FILE__<<":"<<__LINE__<<"\n"; CTERMINATE } }
  #define CTERMINATE Log::Get().Close(); assert(0);
#endif

#define CCOMMA ,
#define GLCHECK(f)     { GLCHECK_RET(f) }
#define CINFO(text)    { Log::Get()<<text<<"\n"; }
#define CERROR(text)   { Log::Get()<<"E "<<text<<" |in "<<__FILE__<<":"<<__LINE__<<"\n"; CTERMINATE }
#define CWARNING(text) { if(Log::IsLevel(Log::WARNING)) { Log::Get()<<"W "<<text<<" |in "<<__FILE__<<":"<<__LINE__<<"\n"; } }

template<class To, class T> To cast(T n)
{
  CASSERT((static_cast<long double>(n) >= static_cast<long double>(std::numeric_limits<To>::lowest())) && (static_cast<long double>(n) <= static_cast<long double>(std::numeric_limits<To>::max())), "Cast overflow");
  return static_cast<To>(n);
}

}
