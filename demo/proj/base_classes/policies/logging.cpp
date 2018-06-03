#include "logging.h"
#include <fstream>
#include <iostream>
#include <mutex>

using namespace code_policy;
using namespace std;

ostream &FilePolicy::stream()
{
  if(!m_log_file)
  {
    static mutex s_m;
    lock_guard<mutex> l(s_m);
    if(!m_log_file)
      m_log_file = new ofstream("log.txt", ofstream::out | ofstream::app);
  }
  return *m_log_file;
}

void FilePolicy::Close()
{
  if(m_log_file)
    delete m_log_file;
}


ostream &CoutPolicy::stream()const
{
  return cout;
}

void CoutPolicy::Close()
{
  cout.flush();
}
