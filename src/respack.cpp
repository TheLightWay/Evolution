// respack.cpp : resource packer
//

#include "pnglite.h"
#include <cstring>
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
#include <cstdlib>
#include "CLI11.hpp"



enum class Type
{
    invalid, image, shader_vert, shader_frag
};

struct Image
{
    std::string name;
    size_t width, height;

    Image(const char *name, size_t name_len, size_t width, size_t height) :
        name(name, name_len), width(width), height(height)
    {
    }
};

struct Shader
{
    Type type;
    std::string name;
    size_t length;

    Shader(Type type, const char *name, size_t name_len, size_t length) :
        type(type), name(name, name_len), length(length)
    {
    }
};

/*
Not using
inline bool check_postfix(const char *str, size_t &len, const char *postfix, size_t n)
{
    if(len <= n || std::memcmp(str + len - n, postfix, n))
        return false;
    len -= n;  
    return true;
}*/

Type classify(const std::string &name )
{
    /*
    !!!GOVNOCODE!!!
    Type type;
    size_t len = name.length();
    if(check_postfix(name.c_str(), len, ".png", 4))
        type = Type::image;
    else if(check_postfix(name.c_str(), len, ".vert", 5))
        type = Type::shader_vert;
    else if(check_postfix(name.c_str(), len, ".frag", 5))
        type = Type::shader_frag;
    else 
        return Type::invalid;

    for(size_t i = 0; i < len; i++)
    {
        if((name[i] < 'a' || name[i] > 'z') && name[i] != '_')
            return Type::invalid;
    }*/
    static const std::string png_ext = ".png";
    static const std::string vert_ext = ".vert";
    static const std::string frag_ext = ".frag";
    Type type = Type::invalid;
    
    size_t pos = name.find_last_of( '.' );
    if( pos != std::string::npos )
    {
        std::string name_ext = name.substr( pos, name.length() );
        if( name_ext == png_ext )
            type = Type::image;
        else if( name_ext == vert_ext )
            type = Type::shader_vert;
        else if( name_ext == frag_ext )
            type = Type::shader_frag;
    }

    return type;
}


class File
{
    FILE *stream;

    File() = delete;
    File(const File &file) = delete;
    File &operator = (const File &file) = delete;

public:
    File(const char *path, const char *mode) : stream(std::fopen(path, mode))
    {
    }

    operator FILE * ()
    {
        return stream;
    }

    ~File()
    {
        if(stream)std::fclose(stream);
    }
};

size_t cannot_open(const char *path)
{
    std::printf("Cannot open file \"%s\"!\n", path);  return 0;
}

size_t read_error( const char* s = nullptr )
{
    if( s ) 
        std::printf( "Read error: %s\n", s );
    else
        std::printf("Read error!\n"); 
    return 0;
}

size_t write_error( const char* s = nullptr )
{
    if( s ) 
        std::printf( "Write error: %s\n", s );
    else
        std::printf("Write error!\n");  
    return 0;
}

template<class T>
T base_filename(T const & path, T const & delims = "/\\")
{
  return path.substr(path.find_last_of(delims) + 1);
}

size_t load_shader(const char* type, std::string &name, FILE *output)
{
    File input(name.c_str(), "rb");
    if(!input)
        return cannot_open(name.c_str());
    
    if(std::setvbuf(input, nullptr, _IONBF, 0))
    {
        std::printf("I/O error!\n");  
        return 0;
    }
    if(std::feof(input))
    {
        std::printf("Empty file \"%s\"!\n", name.c_str());  
        return 0;
    }
    std::string short_name = base_filename( name ); 
    if(std::fprintf(output, "\nstatic const char *shader_%.*s_%s =\n    \"", short_name.length(), short_name.c_str(), type) < 0)
        return write_error();

    size_t size = 0;
    for(unsigned char buf[65536];;)
    {
        size_t n = std::fread(buf, 1, sizeof(buf), input);
        if(std::ferror(input) || !n)return read_error();
        size += n;

        if(std::feof(input))n--;
        for(size_t i = 0; i < n; i++)
            if(std::fprintf(output, (i + 1) & 31 ? "\\x%02X" : "\\x%02X\"\n    \"", unsigned(buf[i])) < 0)
                return write_error();

        if(n == sizeof(buf))
            continue;
        if(std::fprintf(output, "\\x%02X\";\n", unsigned(buf[n])) < 0)
            return write_error();
        break;
    }
    return size;
}


bool load_image(std::string &name, FILE *output, size_t &width, size_t &height)
{
    png_t png;
    int err = png_open_file_read(&png, name.c_str());
    if(err)
    {
        std::printf("Cannot open PNG file \"%s\": %s\n", name.c_str(), png_error_string(err));
        png_close_file(&png);  
        return false;
    }
    if(png.depth != 8 || png.color_type != PNG_TRUECOLOR_ALPHA)
    {
        std::printf("Wrong format of PNG file \"%s\", should be 8-bit RGBA\n", name.c_str());
        png_close_file(&png);  
        return false;
    }
    width = png.width;  height = png.height;
    std::vector<unsigned char> buf(size_t(4) * png.width * png.height);
    err = png_get_data(&png, buf.data());  png_close_file(&png);
    if(err)
    {
        std::printf("Cannot read PNG file \"%s\": %s\n", name.c_str(), png_error_string(err));
        return false;
    }
    std::string short_name = base_filename( name );
    if(std::fprintf(output, "\nstatic const char *image_%.*s =\n    \"", int(short_name.length()), short_name.c_str()) < 0)
        return write_error();

    size_t n = buf.size() - 1;
    for(size_t i = 0; i < n; i++)
        if(std::fprintf(output, (i + 1) & 31 ? "\\x%02X" : "\\x%02X\"\n    \"", unsigned(buf[i])) < 0)
            return write_error();

    if(std::fprintf(output, "\\x%02X\";\n", unsigned(buf[n])) < 0)
        return write_error();
    return true;
}


bool process_files(const std::string &result, const std::string &include, size_t prefix, 
    std::vector<std::string> &args, std::vector<Image> &images, std::vector<Shader> &shaders )
{
    File output(result.c_str(), "wb");  
    
    if(!output)
        return cannot_open(result.c_str());
    
    if(std::fprintf(output, "// %s : resource data\n//\n\n#include \"%s\"\n\n",result.c_str() + prefix, include.c_str() + prefix) < 0)
        return write_error();

    for(size_t i = 0; i < args.size(); i++)
    {
        size_t len = args[i].length();
        switch(classify(args[i]))
        {
        case Type::image:
            {
                size_t width, height;
                if(!load_image(args[i], output, width, height))
                    return false;
                images.emplace_back(args[i].c_str(), len, width, height);  
                break;
            }
        case Type::shader_vert:
            {
                size_t size = load_shader("vert", args[i], output);
                if(size)
                    shaders.emplace_back(Type::shader_vert, args[i].c_str(), len, size);
                else 
                    return false;  
                break;
            }
        case Type::shader_frag:
            {
                size_t size = load_shader("frag", args[i], output);
                if(size)
                    shaders.emplace_back(Type::shader_frag, args[i].c_str(), len, size);
                else 
                    return false;  
                break;
            }
        default:
            std::printf("Invalid resource name \"%s\"!\n", args[i]);  return false;
        }
    }

    if(std::fprintf(output, "\nconst ImageDesc images[] =\n{") < 0)return write_error();
    for(const auto &image : images)
    {
        if(std::fprintf(output, "\n    {\"%s\", image_%s, %uu, %uu},", image.name.c_str(), image.name.c_str(),
            unsigned(image.width), unsigned(image.height)) < 0)return write_error();
    }
    if(std::fprintf(output, "\n};\n") < 0)
        return write_error();

    if(std::fprintf(output, "\nconst ShaderDesc shaders[] =\n{") < 0)
        return write_error();
    for(const auto &shader : shaders)
    {
        if(std::fprintf(output, "\n    {\"%s\", shader_%s_%s, %uu},", shader.name.c_str(), shader.name.c_str(),
            shader.type == Type::shader_vert ? "vert" : "frag", unsigned(shader.length)) < 0)return write_error();
    }
    if(std::fprintf(output, "\n};\n") < 0)return write_error();
        return true;
}

const char *header_tail = R"(
struct ImageDesc
{
    const char *name;
    const char *pixels;
    unsigned width, height;
};

struct ShaderDesc
{
    const char *name;
    const char *source;
    unsigned length;
};

extern const ImageDesc images[];
extern const ShaderDesc shaders[];
)";

bool write_desc( const std::string &result, size_t prefix,
    const std::vector<Image> &images, const std::vector<Shader> &shaders)
{
    std::string short_name;
    File output(result.c_str(), "wb");  
    if(!output)
        return cannot_open(result.c_str());
    if(std::fprintf(output, "// %s : resource description\n//\n\n", result.c_str() + prefix) < 0)
        return write_error( "resource description");

    if(std::fprintf(output, "\nnamespace Image\n{\n    enum Index\n    {") < 0)
        return write_error( "namespace Image" );
    
    for(const auto &image : images)
    {
        short_name = base_filename(image.name);
        if(std::fprintf(output, "\n        %s,",short_name.c_str() ) < 0 )
            return write_error( short_name.c_str() );
    }
    
    if(std::fprintf(output, "\n    };\n}\n") < 0)
        return write_error( "}}" );

    if(std::fprintf(output, "\nnamespace VertShader\n{\n    enum Index\n    {") < 0)
        return write_error( "namespace VertShader" );
    for(size_t i = 0; i < shaders.size(); i++)
    {
        if(shaders[i].type == Type::shader_vert)
        {
            short_name = base_filename(shaders[i].name);
            if(std::fprintf(output, "\n        %s = %u,", short_name.c_str(), unsigned(i)) < 0)
                return write_error( short_name.c_str() );
        }
    }

    if(std::fprintf(output, "\n    };\n}\n") < 0)
        return write_error( "2}}");

    if(std::fprintf(output, "\nnamespace FragShader\n{\n    enum Index\n    {") < 0)
        return write_error( "namespace FragShader" );
    
    for(size_t i = 0; i < shaders.size(); i++)
    {
        if(shaders[i].type == Type::shader_frag)
        {
            short_name = base_filename(shaders[i].name);
            if(std::fprintf(output, "\n        %s = %u,", short_name.c_str(), unsigned(i)) < 0)
                return write_error( short_name.c_str() );
        }
    }
    
    if(std::fprintf(output, "\n    };\n}\n") < 0)
        return write_error( "3}}");

    return std::fprintf(output, "\n%s", header_tail) >= 0;
}

int main(int n, char **args)
{
    std::string result_dir;
    std::string result_basename;
    std::vector<std::string> shaders_list;
    std::vector<std::string> images_list;

    
    CLI::App cmd_line_parser( "respack command line parser");
    
    cmd_line_parser.add_option( "-d, --outdir", result_dir, "Output directory" );
    cmd_line_parser.add_option( "-f, --outfile", result_basename, "Output base resources file name" );
    cmd_line_parser.add_option( "-s, --shaderslist", shaders_list, "Shaders files list" );
    cmd_line_parser.add_option( "-i, --imageslist", images_list, "Images files list" );

    CLI11_PARSE(cmd_line_parser, n, args);

    std::string result_h = result_dir + "/";
    result_h += result_basename;
    result_h += ".h";
    std::string result_cpp = result_dir + "/";
    result_cpp += result_basename;
    result_cpp += ".cpp";
    
    std::vector<std::string> files_list;
    
    files_list.insert(files_list.end(),
        std::make_move_iterator(shaders_list.begin()),
        std::make_move_iterator(shaders_list.end()));
    files_list.insert(files_list.end(),
        std::make_move_iterator(images_list.begin()),
        std::make_move_iterator(images_list.end()));

    const size_t prefix = 6;
    std::vector<Image> images;  
    std::vector<Shader> shaders;  
    png_init(0, 0);
    
    if(!process_files(result_cpp, result_h, prefix, files_list, images, shaders))
        return EXIT_FAILURE;
    return write_desc(result_h, prefix, images, shaders) ? EXIT_SUCCESS : EXIT_FAILURE;
}
