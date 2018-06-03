//gui library itself
#include "gui/resource_control.h"
#include "gui/elements/layout.h"
#include "gui/elements/text_edit.h"
#include "gui/elements/line_edit.h"
#include "gui/elements/button.h"
#include "gui/elements/label.h"
#include "gui/elements/selector.h"

//scene infrastructure and assorted sugar products
#include "base_classes/texture_atlas.h"
#include "base_classes/camera.h"
#include "base_classes/utility/pbrt_envmap.h"
#include "base_classes/utility/compute_shader.h"
#include "base_classes/policies/resource.h"
#include "base_classes/policies/profiling.h"
#include "base_classes/policies/id.h"
#include "base_classes/policies/window.h"
#include "base_classes/policies/serialization.h"
#include "base_classes/policies/profiling.h"

#include "glm/gtc/matrix_transform.hpp"
#include <utfcpp/utf8.h>
#include <GLFW/glfw3.h>
#include <algorithm>

using namespace GUI;

int main()
{
  //pretty self-explanatory. set window state and some basic gl caps
  auto &window = Window::Get();
  window.Resize(600, 300);

  //gl state wrappers. invented because glEnable stuck out like a sore thumb while i was hiding gl calls and because i've seen glGets eating up like 30% frame time on web
  //implemented with template magic/abuse
  GLState::Enable<GL_DEPTH_TEST, GL_BLEND, GL_MULTISAMPLE, GL_DEPTH_WRITEMASK>();
  GLState::BlendFunc::Set(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  GLState::DepthFunc::Set(GL_LESS);

  //shader batch files are pretty nifty; see the files
  ShaderManager::Get().LoadAllShadersFromFile("shd_test.glsl");
  ShaderManager::Get().LoadAllShadersFromFile("shd_support.glsl");

  //glyphs and textures are loaded in such manner since to fit atlas in the most efficient way we want to know sizes for all elements beforehand
  //should we want dynamic texture loading we would simply extend TextureManager through an adapter
  FontManager fonts;

  //i automatically render sdf atlas from .ttf files and charsets since working with separate tool and then loading atlases is a bother

  //btw, Val is a macro for const auto&
  //yes, this is pretty much a loaded automatic shotgun aimed at my proverbial kneecap
  //hovewer this is REALLY handy. to the extent i am willing to risk dangling references(which are actually caught by sanitizers since llvm 6, also; JUST PAY ATTENTION TO WHAT YOU WRITE)
  //the point of this is that i have only macros set on blue in my ide and so i can immediately see which parts of code contain actual mutating variables and which are just declarative stuff
  //i call this approach ``chinese counterfeit rust"
  //can't stress enough how much easier it makes reading the code, you gotta see how this looks on a screen with proper colors
  Val ascii_range = []{ string8 s; for(char i=32; i<127; ++i) s += i; return s; };
  Val default_font = fonts.Register({ "resources/UbuntuMono-R.ttf", ascii_range() + u8"ёйцукенгшщзхъфывапролджэячсмитьбюЁЙЦУКЕНГШЩЗХЪФЫВАПРОЛДЖЭЯЧСМИТЬБЮ" });
  //btw sdf atlas is cached
  fonts.LoadRegisteredFonts();

  TextureManager textures;
  //loading textures into an atlas was never easier
  //Val cat1 = textures.Register("resources/ortodox_grafix.png");
  Animation spinner(textures, "resources/animations/spinner/spinner");
  textures.LoadRegisteredTextures(4);

  //loading environment for pbrt
  //btw pbrt isn't ``the thing" in this demo, gui is ``the thing".
  Val unlz = [](string file){ Archive a; code_policy::deserialize_from_vec(Resource::Load(file), a); return Compression::Extract(a); };

  Val brdf_lut = [&]{ fImage lut; code_policy::deserialize_from_vec(unlz("resources/brdf_lut.lz"), lut); return GLtex2d{ lut, 2 }; }();
  Val skybox = EnvironmentGenerator::LoadEnvmap([&]{ EnvironmentGenerator::Environment e; code_policy::deserialize_from_vec(unlz("resources/cube.lz"), e); return e; }());

  //G:: is a wrapper for gui calls, can be replaced by actual resource control
  //hey, dear imgui just uses global variables
  G::theme() = Theme{ 10
      , vec4(0.2, 0.2, 0.2, 0.7)
      , to_rgba(0x596475A0)
      , to_rgba(0x626975FF)
      , to_rgba(0x461E5CCF)
      , vec4(0.9, 0.4, 0.1, 1)
      , vec4(0.2, 0.2, 0.2, 1)
      , vec4(1, 0.9, 0.9, 0.9)
      , vec4(1)
      , default_font };

  //setting initial state for gui elements
  //once again, dear imgui just uses statics to store state and passes it as arguments
  //i store information that might be continuously useful to us in /gui/elements classes that describe familiar gui objects, hovewer if you look at their definition you'll see that those objects don't have any complex state. they are drawn declaratively and their variables can be changed as we like and it just works.
  //ID is a macro to hash a string into hopefully unique id
  //naughty dog says they never had any collisions with such system. i elect to believe them
  //ID also checks collisions in debug build
  G::Get<Layout>(ID(Window1)).pos = vec2(-10);
  G::Get<Layout>(ID(Window1)).size = vec2(1.5, 2);

  G::Get<Selector>(ID(Selector_FILE)).text = "shd_test.glsl";
  G::Get<Selector>(ID(Selector_VS)).text = "vs_base_3d";
  G::Get<Selector>(ID(Selector_PS)).text = "ps_material_based_render";
  G::Get<Selector>(ID(Selector_Model)).text = "resources/dragon.obj";

  G::Get<LineEdit>(ID(rough)).text = "1.";
  G::Get<LineEdit>(ID(ri)).text = "1.";

  //timers for animations
  float t = 0;
  uint tooltip_timer = 0;
  //state for quitting and animating a spinner(on error, since it turned out there's no part of this app that would load anything with a noticeable delay, which would justify a worker thread)
  bool should_quit = false, running = true;

  //a bunch of vars for the editor logic. all in all the editor is programmed in around 250 lines. the power of declarative approach
  string selected_shader_file, saved_text, selected_model_file;
  vector<string> shader_file_names, vertex_shaders, fragment_shaders;
  map<string, string> shader_files = { { "shd_support.glsl", Resource::LoadText("shd_support.glsl") },
                                       { "shd_test.glsl",    Resource::LoadText("shd_test.glsl") } };

  //shader that is applied to the scene
  //you can change this by selecting vs and fs and pressing "run"
  auto render = make_unique<GLshader>("vs_material_based_render", "ps_material_based_render");

  //declaring in such way since the state of elements is literally just variables that can be abused anyhow you want. the power of declarative approach
  auto& error_log = G::Get<TextEdit>(ID(ErrorLog)).text;

  //it's just a declaration that to us resource such and such is essentially a field with data, that we can freely access
  //no confusion or doubts unlike with proper variable or auto
  //much more readable than const
  //this is what i'm talking about with Val
  Val model_file_names = vector<string>{ "resources/buddha.obj", "resources/bunny.obj", "resources/dragon.obj" };
  Val text_edit = G::Get<TextEdit>(ID(TextEdit));

  //parses shaders and puts them into global shader text pool
  //global captures are such malpractice, i wish i could just use structured bindings already, but odds are you're on a compiler without c++17 support
  //and why again we have & but STILL don't have a const& capture? c++, i swear.
  Val ParseSources = [&]{
    vertex_shaders.clear();
    fragment_shaders.clear();
    for(auto &i: ParseShaderSources(text_edit.text))
    {
      Val prefix = i.first.substr(0, 3);
      if(prefix == "vs_")
        vertex_shaders.emplace_back(i.first);
      else
        if(prefix == "ps_")
          fragment_shaders.emplace_back(i.first);
        else
          continue;

      ShaderManager::Get().ForceShaderSource(move(i.first), move(i.second));
    }
  };

  //load chosen file into textfiled, check what shaders it have, assign choices to selectiors
  Val LoadAction = [&](Val new_selection){
    auto& text_edit = G::Get<TextEdit>(ID(TextEdit));
    selected_shader_file = new_selection;

    Val found = shader_files.find(new_selection);
    if(found != shader_files.cend())
      text_edit.text = found->second;
    else
    {
      auto text = Resource::LoadText(new_selection);
      if(!utf8::is_valid(text.cbegin(), text.cend()))
        text = "";

      text_edit.text = shader_files.emplace(new_selection, text).first->second;
      saved_text = move(text);
    }

    text_edit.write_history();

    shader_file_names.clear();
    std::transform(shader_files.cbegin(), shader_files.cend(), std::back_inserter(shader_file_names), [](Val v){ return v.first; });

    ParseSources();
  };

  //self-explanatory
  Val SaveAction = [&]{
    if(saved_text != text_edit.text)
    {
      saved_text = text_edit.text;

      ParseSources();

      vector<char> str(saved_text.cbegin(), saved_text.cend());
      Resource::Save(selected_shader_file, str);
    }
  };

  Val RunAction = [&]{
    Val selected_vs = G::Get<Selector>(ID(Selector_VS)).text
        , selected_ps = G::Get<Selector>(ID(Selector_PS)).text;

    ParseSources();

    bool valid;
    GLshader new_shader(valid, error_log, selected_vs, selected_ps);
    if(valid)
    {
      running = true;
      render = make_unique<GLshader>(move(new_shader));
    }
  };

  //scene objects and settings for demo itself
  Val render_cube = GLshader("vs_skybox", "ps_skybox");
  Val sky = Skybox(1);
  Mesh mesh1;
  Camera cam1(glm::perspective(glm::radians(55.f), window.size().x / window.size().y, 0.001f, 100.f), glm::lookAt(vec3(0, 0, -1.), vec3(0), vec3(0, 1, 0)));

  G::Get<HorizontalSlider>(ID(metallicity)).bar = 0.885;
  G::Get<HorizontalSlider>(ID(roughness)).bar = 0.286;

  //now global loop starts
  while(!should_quit)
  {
    //Renderer processes events and draws actual gui graphics
    //Elements from gui/elements draw through it
    //renderer + objects essentially form the gui framework
    Renderer &base = G::renderer();

    {
      //take new events from window
      CAUTOTIMER(events_poll);
      base.ConsumeEvents(window.PollEvents());
    }

    {
      //drawing the pbrt demo
      Val rotation = (0.5 + G::Draw<HorizontalSlider>(ID(bar), vec2(0.3, -1), vec2(1, 0.05), 0.05).bar) * M_PI * 2.;

      Val metallicity = G::Draw<HorizontalSlider>(ID(metallicity), vec2(0.3, -0.88), vec2(1, 0.05), 0.05).bar
          , roughness = G::Draw<HorizontalSlider>(ID(roughness), vec2(0.3, -0.94), vec2(1, 0.05), 0.05).bar;

      G::Draw<Label>(ID(met_count), vec2(1.31, -0.88), vec2(0.3, 0.05), "metallicity :" + std::to_string(metallicity));
      G::Draw<Label>(ID(rou_count), vec2(1.31, -0.94), vec2(0.3, 0.05), "roughness :" + std::to_string(roughness));

      Val model_selected = G::Draw<Selector>(ID(Selector_Model), vec2(1.31, -0.82), vec2(0.3, 0.05), model_file_names).text;
      if(model_selected != selected_model_file)
      {
        selected_model_file = model_selected;
        mesh1 = { [&]{ Mesh::Model m; code_policy::deserialize_from_vec(unlz(model_selected), m); return m; }() };
      }

      //switch from fbos to screen
      window.DrawToScreen(true);

      GLState::ClearColor(0.7);
      GLState::Enable<GL_DEPTH_TEST, GL_MULTISAMPLE, GL_TEXTURE_CUBE_MAP_SEAMLESS>();
      GLState::Clear(GL_DEPTH_BUFFER_BIT);

      if(render)
      {
        Val model = glm::rotate(glm::translate(glm::rotate(mat4(1), float(rotation - M_PI), vec3(0, 1, 0)), vec3(-0.1, 0, 0)), float(rotation), vec3(0, 1, 0));
        Val c_ = cos(rotation) * 0.3
            , s_ = sin(rotation) * 0.3
            , c = cos(t * M_PI * 2)
            , s = sin(t * M_PI * 2);

        Val cam_world = vec3(s_, 0, c_);
        cam1.setView(glm::lookAt(cam_world, vec3(0), vec3(0, 1, 0)));

        //i use GLbind for all gl resources, it checks lifetime collisions
        //shader
        auto b = GLbind(*render);
        b.Uniforms("MVPMat", cam1.MVP(model),
                   "ModelViewMat", cam1.MV(model),
                   "NormalViewMat", cam1.NV(model),
                   "NormalMat", cam1.N(model),
                   "irradiance_cubetex", 0,
                   "specular_cubetex", 1,
                   "brdf_lut", 2,
                   "camera_world", cam_world,
                   "light_pos", vector<vec3>{ vec3(cam1.V() * vec4(6 * c, 6 * s, 0, 1)), vec3(cam1.V() * vec4(2 * c, 0, 2 * s, 1)),
                                              vec3(cam1.V() * vec4(c, s, -2, 1)),        vec3(cam1.V() * vec4(-1.5 * s, 1 * c, -2, 1)) },
                   "light_color", vector<vec4>{ vec4(1, 0, 0, 2), vec4(0, 0, 1, 2),
                                                vec4(1, 0, 1, 2), vec4(0, 1, 0, 2) },
                   "albedo", vec3(to_rgba(0xD4AF3700)),
                   "metallicity", metallicity,
                   "roughness", roughness,
                   "exposure", 1.,
                   "max_lod", skybox.mip_levels);

        //textures
        GLbind(skybox.irradiance, 0);
        GLbind(skybox.specular, 1);
        GLbind(brdf_lut, 2);

        //draw model
        mesh1.Draw();
      }
      {
        //same thing for skybox
        //i really like how GLbind works with raii and scopes
        auto b = GLbind(render_cube);
        b.Uniforms("MVPMat", cam1.MVP(),
                   "skybox_tex", 0,
                   "exposure", 1.);

        GLbind(skybox.specular, 0);

        sky.Draw();
      }
    }

    {
      //now drawing the editor gui
      CAUTOTIMER(obj);
      const auto last_tooltip_timer = tooltip_timer;

      //layout meant to be a movable window base, so it takes lambda, passes own coordinates to it and draws it
      //as you may have noticed by now, i love lambdas
      G::Draw<Layout>(ID(Window1), [&](Val pos, Val size){

        Val button_w = 0.18
            , button_h = 0.06
            , padding = 0.01;

        //if hovering and timeout - draw popup
        //compare this to extending the parent of actual objects to handle popups in oop
        Val ShowTooltip = [&](Val hovered, Val p, string message){
          if(hovered)
          {
            ++tooltip_timer;
            if(tooltip_timer > 60)
              G::Draw<Label>(ID(Tooltip), p + size * vec2(0.05), size * vec2(message.length() * 0.03, 0.05), message);
          }
        };

        //text editor itself. comes with line wrapping, scroll, select, copy, paste, history, line numbers, can draw like literally millions of lines in release(debug performance mainly crippled but utf8 internal errorchecking)
        //can gtk or qt textbox handle millons of lines of text?
        //is it implemented in 500 loc?
        //this is the power of functional programming
        G::Draw<TextEdit>(ID(TextEdit), pos + size * vec2(0, button_h + padding * 2), size * vec2(1, 1. - (button_h + padding * 2)), 0.05);
        if(!error_log.empty())
        {
          running = false;
          G::Draw<TextEdit>(ID(ErrorLog), pos + size * vec2(1 + padding, 0), size, 0.05, true);
        }

        //buttons and tips
        Val button_size = size * vec2(button_w, button_h);

        Val save_button_pos = pos + size * vec2(padding);
        Val save_button = G::Draw<Button>(ID(Save), save_button_pos, button_size, "Save");
        ShowTooltip(save_button.hovered && !save_button.pressed, save_button_pos, "Ctrl + s");

        if(save_button.pressed)
          SaveAction();

        //select source file, edit shaders, select vertex and fragment shaders, run. on error will show textfield with error text
        Val file_selector_pos = pos + size * vec2(button_w + padding * 3, padding);
        Val file_selector = G::Draw<Selector>(ID(Selector_FILE), file_selector_pos, button_size, shader_file_names);
        ShowTooltip(file_selector.hovered && !file_selector.active, file_selector_pos, "Select sources");

        if(file_selector.text != selected_shader_file)
          LoadAction(file_selector.text);

        Val vs_selector_pos = pos + size * vec2(button_w * 2 + padding * 5, padding);
        Val vs_selector = G::Draw<Selector>(ID(Selector_VS), vs_selector_pos, button_size, vertex_shaders);
        ShowTooltip(vs_selector.hovered && !vs_selector.active, vs_selector_pos, "Select vertex shader");

        Val ps_selector_pos = pos + size * vec2(button_w * 3 + padding * 7, padding);
        Val ps_selector = G::Draw<Selector>(ID(Selector_PS), ps_selector_pos, button_size, fragment_shaders);
        ShowTooltip(ps_selector.hovered && !ps_selector.active, ps_selector_pos, "Select fragment shader");


        Val run_button_pos = pos + size * vec2(button_w * 4 + padding * 9, padding);
        Val run_button = G::Draw<Button>(ID(Run), run_button_pos, button_size, "Run");
        ShowTooltip(run_button.hovered && !run_button.pressed, run_button_pos, "Ctrl + r");

        if(run_button.pressed)
          RunAction();

        //on errror show the spinning thingie, we can do animations unlike dear imgui!
        if(running)
          return;

        base.Clip(pos, size);
        base.Draw<Sprite>(pos + size * vec2(1. - padding - (button_w + button_h) / 2, button_h + padding * 3), size * vec2(button_h), spinner.currentFrame(t));
      });

      //increment
      t = glm::mod(t + 0.01, 1.);
      //reset timer if nothing hovered
      tooltip_timer = tooltip_timer == last_tooltip_timer ? 0u : tooltip_timer;
    }

    {
      //process whichever events weren't claimed by drawn elements
      CAUTOTIMER(events);
      for(Val e: base.ProcessEvents())
        switch(e.type())
        {
          case Event::Type::Key:
          {
            switch(e.key().c)
            {
              case GLFW_KEY_ESCAPE: should_quit = true; break;
              case GLFW_KEY_S: if(e.key().s & Event::State::Ctrl) SaveAction(); break;
              case GLFW_KEY_R: if(e.key().s & Event::State::Ctrl) RunAction(); break;
            }
            break;
          }
        }
    }

    {
      //actually render the gui
      CAUTOTIMER(rend);
      base.Render();
    }

    //that's all folks!
    window.Swap();
  }

  return 0;
}
