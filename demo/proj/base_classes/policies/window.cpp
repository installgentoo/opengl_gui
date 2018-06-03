#include "window.h"
#include "base_classes/gl/state.h"
#include <glm/gtc/epsilon.hpp>

using namespace code_policy;


template<class m_policy> bool WindowControl<m_policy>::equalPos(float l, float r)const
{
  return glm::epsilonEqual(l, r, m_pixel.y);
}

template<class m_policy> bool WindowControl<m_policy>::equalPos(vec2 const&l, vec2 const&r)const
{
  return glm::all(glm::epsilonEqual(l, r, m_pixel));
}

template<class m_policy> bool WindowControl<m_policy>::equalPos(vec4 const&l, vec4 const&r)const
{
  return glm::all(glm::epsilonEqual(l, r, vec4(m_pixel, m_pixel)));
}

template<class m_policy> void WindowControl<m_policy>::DrawToScreen(bool clear)
{
  if(m_policy::wasResized())
  {
    m_window_size = m_policy::size();

    Val min_side = glm::min(m_window_size.x, m_window_size.y);
    m_aspect = vec2(min_side) / m_window_size;
    m_pixel = vec2(1) / m_window_size;
  }

  GLState::Viewport(m_window_size.x, m_window_size.y);
  GLState::BindScreenFramebuffer();

  if(clear)
    GLState::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


#ifdef USE_GLFW
#include <GLFW/glfw3.h>

namespace code_policy { template struct WindowControl<GLFWWindowPolicy>; }

vector<Event> GLFWWindowPolicy::m_events;
uvec2         GLFWWindowPolicy::m_window_size;
bool          GLFWWindowPolicy::m_resized = true;

static uint glfwToState(int action, int mod)
{
  uint a = 0;
  switch(action)
  {
    case GLFW_RELEASE: a = Event::State::Release; break;
    case GLFW_PRESS:   a = Event::State::Press; break;
    case GLFW_REPEAT:  a = Event::State::Repeat; break;
    default: CASSERT(0, "GLFW sent wrong action code");
  }

  uint m = 0;
  m |= (mod & GLFW_MOD_SHIFT) ?   Event::State::Shift : 0u;
  m |= (mod & GLFW_MOD_CONTROL) ? Event::State::Ctrl : 0u;
  m |= (mod & GLFW_MOD_ALT) ?     Event::State::Alt : 0u;
  m |= (mod & GLFW_MOD_SUPER) ?   Event::State::Win : 0u;

  return (Event::State::Action & a) | (Event::State::Mod & m);
}


vec2 GLFWWindowPolicy::size()const
{
  return m_window_size;
}

bool GLFWWindowPolicy::wasResized()
{
  if(!m_resized)
    return false;

  m_resized = false;
  return true;
}

void GLFWWindowPolicy::Initialize(uvec2 size)
{
  if(!glfwInit())
    CERROR("Error initializing glfw");
  //glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_FALSE);//GL_TRUE
  glfwWindowHint(GLFW_SAMPLES, 4);

  if(!(m_glfw_window = glfwCreateWindow(cast<int>(size.x), cast<int>(size.y), "Engine", nullptr, nullptr)))
  {
    glfwTerminate();
    CERROR("Error creating window");
  }
  m_window_size = size;
  glfwSetWindowPos(m_glfw_window, 100, 100);
  glfwMakeContextCurrent(m_glfw_window);
  glfwSetWindowSizeCallback(m_glfw_window,  GLFWWindowPolicy::WindowSizeChangeCb);
  glfwSetCursorPosCallback(m_glfw_window,   GLFWWindowPolicy::MouseMoveCb);
  glfwSetMouseButtonCallback(m_glfw_window, GLFWWindowPolicy::MouseButtonCb);
  glfwSetKeyCallback(m_glfw_window,         GLFWWindowPolicy::KeyCb);
  glfwSetScrollCallback(m_glfw_window,      GLFWWindowPolicy::ScrollCb);
  glfwSetCharCallback(m_glfw_window,        GLFWWindowPolicy::UnicodeCharCb);
  glfwSwapInterval(1);

  if(gl3wInit())
    CERROR("GL3W failed to initialize OpenGL");
  if(!gl3wIsSupported(3, 3))
    CERROR("Opengl 3.3 not supported");

  //GLState::EnableDebugContext(1);

  GLState::ClearColor(0.f);

  CINFO("OpenGL initialized");
}

void GLFWWindowPolicy::Deinitialize()
{
  glfwDestroyWindow(m_glfw_window);
  glfwTerminate();
  CINFO("Terminated glfw");
}

vector<Event> GLFWWindowPolicy::PollEvents()
{
  glfwPollEvents();
  vector<Event> events = move(m_events);
  m_events.clear();
  return events;
}

string8 GLFWWindowPolicy::clipboard()const
{
  char const*s = glfwGetClipboardString(m_glfw_window);
  if(!s)
    return { };

  return s;
}

void GLFWWindowPolicy::setClipboard(string8 const&s)const
{
  glfwSetClipboardString(m_glfw_window, s.c_str());
}

void GLFWWindowPolicy::Swap()const
{
  glfwSwapBuffers(m_glfw_window);
}

void GLFWWindowPolicy::Resize(uvec2 size)
{
  glfwSetWindowSize(m_glfw_window, cast<int>(size.x), cast<int>(size.y));
  m_window_size = size;
  m_resized = true;

  double x, y;
  glfwGetCursorPos(m_glfw_window, &x, &y);
  this->MouseMoveCb(m_glfw_window, x, y);
}

void GLFWWindowPolicy::WindowSizeChangeCb(GLFWwindow*, int width, int height)
{
  m_window_size = { cast<uint>(width), cast<uint>(height) };
  m_resized = true;
}

void GLFWWindowPolicy::MouseMoveCb(GLFWwindow*, double x, double y)
{
  Val min_side = glm::min(m_window_size.x, m_window_size.y);
  m_events.emplace_back(Event::MouseMove{ vec2(x * 2 - m_window_size.x, double(m_window_size.y) - y * 2) / vec2(min_side) });
}

void GLFWWindowPolicy::MouseButtonCb(GLFWwindow*, int button, int action, int mod)
{
  m_events.emplace_back(Event::MouseButton{ cast<uint>(button), glfwToState(action, mod) });
}

void GLFWWindowPolicy::KeyCb(GLFWwindow*, int key, int, int action, int mod)
{
  m_events.emplace_back(Event::Key{ cast<uint>(key), glfwToState(action, mod) });
}

void GLFWWindowPolicy::ScrollCb(GLFWwindow*, double x, double y)
{
  m_events.emplace_back(Event::Scroll{{ x, y }});
}

void GLFWWindowPolicy::UnicodeCharCb(GLFWwindow*, uint code)
{
  m_events.emplace_back(Event::Unichar{ code });
}
#endif


#ifdef USE_SDL
#include "SDL2/SDL.h"

namespace code_policy { template struct WindowControl<SDLWindowPolicy>; }

vector<Event> SDLWindowPolicy::m_events;
uvec2         SDLWindowPolicy::m_window_size;
bool          SDLWindowPolicy::m_resized = true;

vec2 SDLWindowPolicy::size()const
{
  return m_window_size;
}

bool SDLWindowPolicy::wasResized()
{
  if(!m_resized)
    return false;

  m_resized = false;
  return true;
}

void SDLWindowPolicy::Initialize(uvec2 size)
{
  if(SDL_Init(SDL_INIT_VIDEO) != 0)
    CERROR("Error initializing SDL: "<<SDL_GetError());

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

  if(!(m_sdl_window = SDL_CreateWindow("Engine", 100, 100, cast<int>(size.x), cast<int>(size.y), SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE)))
    CERROR("Error creating window");

  m_window_size = size;

  if(!(m_gl_context = SDL_GL_CreateContext(m_sdl_window)))
    CERROR("SDL couldn't create context: "<<SDL_GetError());

  SDL_GL_SetSwapInterval(1);

  if(gl3wInit())
    CERROR("GL3W failed to initialize OpenGL");
  if(!gl3wIsSupported(3, 3))
    CERROR("Opengl 3.3 not supported");

  GLState::ClearColor(0.f);

  CINFO("OpenGL initialized");
}

void SDLWindowPolicy::Deinitialize()
{
  SDL_GL_DeleteContext(m_gl_context);
  SDL_DestroyWindow(m_sdl_window);
  SDL_Quit();
  CINFO("Terminated glfw");
}

vector<Event> SDLWindowPolicy::PollEvents()
{
  SDL_Event e;
  while(SDL_PollEvent(&e)){
    //impl
  }

  vector<Event> events = move(m_events);
  m_events.clear();
  return events;
}

string8 SDLWindowPolicy::clipboard()const
{
  char*s = SDL_GetClipboardText();
  if(!s)
    return { };

  string8 str = s;
  SDL_free(s);
  return str;
}

void SDLWindowPolicy::setClipboard(string8 const&s)const
{
  SDL_SetClipboardText(s.c_str());
}

void SDLWindowPolicy::Swap()const
{
  SDL_GL_SwapWindow(m_sdl_window);
}

void SDLWindowPolicy::Resize(uvec2 size)
{
  SDL_SetWindowSize(m_sdl_window, cast<int>(size.x), cast<int>(size.y));
  m_window_size = size;
  m_resized = true;
}
#endif
