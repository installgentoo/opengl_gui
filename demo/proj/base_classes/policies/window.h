#pragma once
#include "logging.h"
#include "events.h"

struct GLFWwindow;
struct SDL_Window;

namespace code_policy
{

template<class m_policy>
struct WindowControl : m_policy, CUNIQUE
{
  static WindowControl& Get() {
    static WindowControl s_manager;
    return s_manager;
  }

  WindowControl() {
    m_policy::Initialize(m_window_size);
  }
  ~WindowControl() {
    m_policy::Deinitialize();
  }

  Val aspect()const { return m_aspect;      }
  Val size()const   { return m_window_size; }
  vec2 left_bottom()const { return vec2(1) / -m_aspect; }
  vec2 right_up()const    { return vec2(1) / m_aspect;  }

  bool equalPos(float l, float r)const;
  bool equalPos(vec2 const&l, vec2 const&r)const;
  bool equalPos(vec4 const&l, vec4 const&r)const;

  vector<Event> PollEvents() {
    return m_policy::PollEvents();
  }

  string8 clipboard()const {
    return m_policy::clipboard();
  }

  void setClipboard(string8 const&s) {
    m_policy::setClipboard(s);
  }

  void DrawToScreen(bool clear);

  void Resize(uint width, uint height) {
    m_window_size = { width, height };
    m_policy::Resize({ width, height });
  }

  void Swap() {
    m_policy::Swap();
  }

private:
  vec2 m_window_size = vec2(400)
      , m_aspect = vec2(1)
      , m_pixel = vec2(1) / vec2(m_window_size);
};


#ifdef USE_GLFW
struct GLFWWindowPolicy
{
protected:
  vec2 size()const;
  bool wasResized();

  void Initialize(uvec2);
  void Deinitialize();

  vector<Event> PollEvents();
  string8 clipboard()const;
  void setClipboard(string8 const&s)const;
  void Swap()const;
  void Resize(uvec2);

private:
  GLFWwindow *m_glfw_window = nullptr;
  static vector<Event> m_events;
  static uvec2 m_window_size;
  static bool m_resized;

  static void WindowSizeChangeCb(GLFWwindow*, int, int);
  static void MouseMoveCb(GLFWwindow*, double, double);
  static void MouseButtonCb(GLFWwindow*, int, int, int);
  static void ScrollCb(GLFWwindow*, double, double);
  static void KeyCb(GLFWwindow*, int, int, int, int);
  static void UnicodeCharCb(GLFWwindow*, uint);
};
typedef WindowControl<GLFWWindowPolicy> Window;
#endif


#ifdef USE_SDL
struct SDLWindowPolicy
{
protected:
  vec2 size()const;
  bool wasResized();

  void Initialize(uvec2);
  void Deinitialize();

  vector<Event> PollEvents();
  string8 clipboard()const;
  void setClipboard(string8 const&s)const;
  void Swap()const;
  void Resize(uvec2);

private:
  SDL_Window *m_sdl_window = nullptr;
  void *m_gl_context = nullptr;
  static vector<Event> m_events;
  static uvec2 m_window_size;
  static bool m_resized;
};
typedef WindowControl<SDLWindowPolicy> Window;
#endif

}
