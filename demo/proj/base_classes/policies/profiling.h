#pragma once
#include "base_classes/gl/objects.h"
#include <chrono>

namespace code_policy
{

#define CAUTOTIMER_SINGLE(name) code_policy::ProfilingManager::ProfilingAutoTimer name(#name);
#define CAUTOTIMER_START(name) auto& profiling_timer_var_##name = code_policy::ProfilingManager::Get().m_timers[#name]; profiling_timer_var_##name.Start();
#define CAUTOTIMER_STOP(name) profiling_timer_var_##name.Stop();
#define CAUTOTIMER(name) code_policy::ProfilingManager::ProfilingMeasurement profiling_timer_var_##name(#name);

#define CGL_AUTOTIMER_SINGLE(name) code_policy::ProfilingManager::GLProfilingAutoTimer name(#name);
#define CGL_AUTOTIMER_START(name) auto& gl_profiling_timer_var_##name = code_policy::ProfilingManager::Get().m_gl_timers[#name]; gl_profiling_timer_var_##name.Start();
#define CGL_AUTOTIMER_STOP(name) gl_profiling_timer_var_##name.Stop();
#define CGL_AUTOTIMER(name) code_policy::ProfilingManager::GLProfilingMeasurement gl_profiling_timer_var_##name(#name);


struct GLQuery : GLobject<QueryPolicy>
{
  void Begin();
  double End();

private:
  static bool m_started;
};


struct ProfilingManager : CUNIQUE
{
  struct GLProfilingAutoTimer {
    GLProfilingAutoTimer(string);
    ~GLProfilingAutoTimer();

  private:
    const string m_name;
    GLQuery m_timer;
  };

  struct GLProfilingTimer {
    void Start()                  { m_timer.Begin();                         }
    void Stop()                   { m_measured += m_timer.End(); ++m_called; }
    double GetAverageTime()const  { return m_measured / m_called;            }

  private:
    uint64 m_called = 0;
    double m_measured = 0.;
    GLQuery m_timer;
  };

  struct GLProfilingMeasurement {
    GLProfilingMeasurement(char const*);
    ~GLProfilingMeasurement();

  private:
    GLProfilingTimer *m_timer;
  };

  struct ProfilingAutoTimer {
    ProfilingAutoTimer(string);
    ~ProfilingAutoTimer();

  private:
    std::chrono::time_point<std::chrono::steady_clock> m_start;
    const string m_name;
  };

  struct ProfilingTimer {
    void Start();
    void Stop();
    double GetAverageTime()const;

  private:
    bool m_started = false;
    std::chrono::time_point<std::chrono::steady_clock> m_start;
    vector<std::chrono::duration<double>> m_measured;
  };

  struct ProfilingMeasurement {
    ProfilingMeasurement(char const*);
    ~ProfilingMeasurement();

  private:
    ProfilingTimer *m_timer;
  };

  static ProfilingManager& Get();

  unordered_map<string, ProfilingTimer> m_timers;
  unordered_map<string, GLProfilingTimer> m_gl_timers;

  ~ProfilingManager();
};

}
