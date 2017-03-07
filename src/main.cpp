// main.cpp : entry point
//

#include "graph.h"
#include "stream.h"



const char *gl_error_text()
{
    switch(glGetError())
    {
    case GL_NO_ERROR:                       return nullptr;
    case GL_INVALID_ENUM:                   return "invalid enumerant";
    case GL_INVALID_VALUE:                  return "invalid value";
    case GL_INVALID_OPERATION:              return "invalid operation";
    case GL_INVALID_FRAMEBUFFER_OPERATION:  return "invalid framebuffer operation";
    case GL_OUT_OF_MEMORY:                  return "out of memory";
    case GL_STACK_UNDERFLOW:                return "stack underflow";
    case GL_STACK_OVERFLOW:                 return "stack overflow";
    case GL_TABLE_TOO_LARGE:                return "table too large";
    default:                                return "unknown error";
    }
}

bool check_gl_error()
{
    const char *text = gl_error_text();  if(!text)return true;
    std::printf("GL error: %s\n", text);  return false;
}

bool load_restart(World &world, const char *path)
{
    InFileStream stream;
    if(!stream.open(path))
    {
        std::printf("Cannot open restart file \"%s\"!\n", path);  return false;
    }
    stream >> world;
    if(!stream.close())
    {
        std::printf("Invalid restart file \"%s\"!\n", path);  return false;
    }
    return true;
}

bool save_restart(World &world)
{
    OutFileStream stream;
    if(stream.open("default.save~"))
    {
        stream << world;
        if(stream.close() && !std::rename("default.save~", "default.save"))
        {
            std::printf("Restart successfully saved.\n");
            print_checksum(world, stream);  return true;
        }
    }
    std::printf("Cannot save restart!\n");  return false;
}

bool main_loop(SDL_Window *window, char **args, int n)
{
    //glEnable(GL_DEPTH_TEST);  glDepthFunc(GL_GREATER);  glClearDepth(0);
    glEnable(GL_FRAMEBUFFER_SRGB);  glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);

    World world;
    if(n <= 1)world.init();
    else if(!load_restart(world, args[1]))return false;
    Camera cam(window);  Representation graph;
    graph.update(window, world, cam, true);

    if(!check_gl_error())return false;

    bool play = false;
    for(SDL_Event evt;;)
    {
        if(!SDL_PollEvent(&evt))
        {
            if(play)
            {
                world.next_step();  graph.update(window, world, cam, false);
            }
            graph.draw(world, cam);  if(!check_gl_error())return false;
            SDL_GL_SwapWindow(window);  continue;
        }
        switch(evt.type)
        {
        case SDL_MOUSEBUTTONDOWN:
            if(evt.button.button != SDL_BUTTON_RIGHT)continue;
            graph.select(world, cam, evt.button.x, evt.button.y);  break;

        case SDL_MOUSEMOTION:
            if(!(evt.motion.state & SDL_BUTTON_LMASK))continue;
            cam.move(evt.motion.xrel, evt.motion.yrel);  break;

        case SDL_MOUSEWHEEL:
            cam.rescale(evt.wheel.y);  break;

        case SDL_WINDOWEVENT:
            if(evt.window.event != SDL_WINDOWEVENT_RESIZED)continue;
            cam.resize(evt.window.data1, evt.window.data2);  break;

        case SDL_KEYDOWN:
            switch(evt.key.keysym.sym)
            {
            case SDLK_SPACE:
                play = !play;  break;

            case SDLK_RIGHT:
                world.next_step();  graph.update(window, world, cam, true);
                play = false;  break;

            case SDLK_F5:
                save_restart(world);  break;

            default:
                continue;
            }
            break;

        case SDL_QUIT:
            return true;

        default:
            continue;
        }
    }
}

bool sdl_error(const char *text)
{
    std::printf("%s%s\n", text, SDL_GetError());  return false;
}

int init(char **args, int n)
{
    if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) || SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3))
        return sdl_error("Failed to set OpenGL version: ");
    if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE))
        return sdl_error("Failed to set core profile: ");
#ifdef DEBUG
    if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG))
        return sdl_error("Failed to use debug context: ");
#endif

    if(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1))return sdl_error("Failed to enable double-buffering: ");
    //if(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24))return sdl_error("Failed to enable depth buffer: ");
    if(SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1))return sdl_error("Failed to enable sRGB framebuffer: ");
    if(SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1) || SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4))
        return sdl_error("Failed to enable multisampling: ");

    const int width = 800, height = 600;
    SDL_Window *window = SDL_CreateWindow("Evolution",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if(!window)return sdl_error("Cannot create window: ");

    const ImageDesc &icon = images[Image::icon];
    SDL_Surface *icon_surf = SDL_CreateRGBSurfaceFrom(const_cast<char *>(icon.pixels),
        icon.width, icon.height, 32, 4 * icon.width, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
    SDL_SetWindowIcon(window, icon_surf);
    SDL_FreeSurface(icon_surf);

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if(*SDL_GetError())return sdl_error("Cannot create OpenGL context: ");

    bool res = main_loop(window, args, n);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    return res;
}

int main(int n, char **args)
{
    if(SDL_Init(SDL_INIT_VIDEO))return sdl_error("SDL_Init failed: ");
    bool res = init(args, n);  SDL_Quit();  return res ? 0 : -1;
}
