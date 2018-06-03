#include "profiling.h"
#include <numeric>

using namespace code_policy;
using namespace std::chrono;

bool GLQuery::m_started = false;

static void formattedTimeOutput(double t)
{
  if(t >= 1) CINFO(t<<" s")
      else if(t >= 0.001) CINFO(t * 1000<<" ms")
      else if(t >= 0.000001) CINFO(t * 1000000<<" us")
      else CINFO(t * 1000000000<<" ns");
}


void GLQuery::Begin()
{
  CASSERT(!m_started, "GLquery already started");
  m_started = true;
  GLCHECK(glBeginQuery(GL_TIME_ELAPSED, GLobject<QueryPolicy>::obj()));
}

double GLQuery::End()
{
  CASSERT(m_started, "GLquery wasn't started");
  GLCHECK(glEndQuery(GL_TIME_ELAPSED));
  GLint64 result = 0;
  GLCHECK(glGetQueryObjecti64v(GLobject<QueryPolicy>::obj(), GL_QUERY_RESULT, &result));
  m_started = false;
  return double(result) / 1000000000;
}


ProfilingManager::GLProfilingAutoTimer::GLProfilingAutoTimer(string name)
  : m_name(move(name))
{
  m_timer.Begin();
}

ProfilingManager::GLProfilingAutoTimer::~GLProfilingAutoTimer()
{
  Val end = m_timer.End();
  CINFO("GL timer '"<<m_name<<"': ");
  formattedTimeOutput(end);
}


ProfilingManager::GLProfilingMeasurement::GLProfilingMeasurement(char const*name)
{
  m_timer = &ProfilingManager::Get().m_gl_timers[name];
  m_timer->Start();
}

ProfilingManager::GLProfilingMeasurement::~GLProfilingMeasurement()
{
  m_timer->Stop();
}


ProfilingManager::ProfilingAutoTimer::ProfilingAutoTimer(string name)
  : m_name(move(name))
{
  m_start = steady_clock::now();
}

ProfilingManager::ProfilingAutoTimer::~ProfilingAutoTimer()
{
  Val end = steady_clock::now();
  CINFO("Timer '"<<m_name<<"': ");
  formattedTimeOutput(duration_cast<duration<double>>(end - m_start).count());
}


void ProfilingManager::ProfilingTimer::Start()
{
  m_started = true;
  m_start = steady_clock::now();
}

void ProfilingManager::ProfilingTimer::Stop()
{
  Val end = steady_clock::now();
  CASSERT(m_started, "Timer wasn't started");
  m_measured.emplace_back(duration_cast<duration<double>>(end - m_start));
  m_started = false;
}

double ProfilingManager::ProfilingTimer::GetAverageTime()const
{
  CASSERT(!m_measured.empty() && !m_started, "Timer wasn't stopped");
  return std::accumulate(m_measured.cbegin(), m_measured.cend(), 0., [](double v, Val i){ return v + i.count(); }) / m_measured.size();
}


ProfilingManager::ProfilingMeasurement::ProfilingMeasurement(char const*name)
{
  m_timer = &ProfilingManager::Get().m_timers[name];
  m_timer->Start();
}

ProfilingManager::ProfilingMeasurement::~ProfilingMeasurement()
{
  m_timer->Stop();
}


ProfilingManager& ProfilingManager::Get()
{
  static ProfilingManager s_manager;
  return s_manager;
}

ProfilingManager::~ProfilingManager()
{
  if(!m_timers.empty())
    CINFO("Timers:");
  for(Val i: m_timers)
  {
    CINFO("Timer '"<<i.first<<"': ");
    formattedTimeOutput(i.second.GetAverageTime());
  }
  if(!m_gl_timers.empty())
    CINFO("GL timers:");
  for(Val i: m_gl_timers)
  {
    CINFO("GL timer '"<<i.first<<"': ");
    formattedTimeOutput(i.second.GetAverageTime());
  }
}
