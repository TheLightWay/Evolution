// respack.cpp : resource packer
//

#include <cstring>
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
#include <cstdlib>
#include "CLI11.hpp"
#define STBI_ONLY_PNG
#include "stb_image.h"


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

std::string base_filename(std::string const & path, std::string const & delims = "/\\")
{
  return path.substr(path.find_last_of(delims) + 1);
}

std::string filename_without_ext( std::string const & path )
{
    std::string ret = base_filename( path );
    return ret.substr(0, ret.find_last_of('.') );
}

long fsize( FILE* stream )
{
	auto curr_pos = ftell ( stream );
	fseek ( stream, 0, SEEK_END );
	auto ret = ftell ( stream );
	fseek ( stream, curr_pos, SEEK_SET );
	return ret;
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
    std::string short_name = filename_without_ext( name ); 
    if(std::fprintf(output, "\nstatic const char shader_%.*s_%s[%i] ={\n", 
		(unsigned)short_name.length(), short_name.c_str(), type, (unsigned)fsize( input ) ) < 0 )
        return write_error();

    size_t size = 0;
    for(unsigned char buf[65536];;)
    {
        size_t n = std::fread(buf, 1, sizeof(buf), input);
        if(std::ferror(input) || !n)
			return read_error();
        size += n;

        if(std::feof(input))n--;
        for(size_t i = 0; i < n; i++)
            if(std::fprintf(output, (i + 1) & 31 ? "'\\x%02X\'," : "'\\x%02X\',\n", unsigned(buf[i])) < 0)
                return write_error();

        if(n == sizeof(buf))
            continue;
        if(std::fprintf(output, "'\\x%02X\'\n }; \n\n", unsigned(buf[n])) < 0)
            return write_error();
        break;
    }
    return size;
}



bool load_image(std::string &name, FILE *output, int &width, int &height)
{
    int channels = -1;
    unsigned char* data = stbi_load( name.c_str(), &width, &height, &channels, 0 );
    if(nullptr == data)
    {
        std::printf("Cannot read PNG file %s, stbi_loand return nullptr\n", name.c_str() );
        return false;
    }

    if(channels != 4)
    {
        std::printf( "Wrong format of PNG file \"%s\", should be 8-bit RGBA\n", name.c_str() );
        std::free( data );
        return false;
    }

    unsigned data_size = ((width * height) * channels); 
    std::string short_name = filename_without_ext( name );
    if(std::fprintf(output, "\nstatic const char image_%.*s[%i] ={\n", 
		int(short_name.length()), short_name.c_str(), data_size ) < 0)
    {
        std::free( data );
        return write_error();
    }

    size_t n = data_size - 1;
    for(size_t i = 0; i < n; i++)
    {
        if(std::fprintf(output, (i + 1) & 31 ? "'\\x%02X\'," : "'\\x%02X\',\n", unsigned(data[i])) < 0)
        {
            std::free( data );
            return write_error();
        }
    }

    if(std::fprintf(output, "'\\x%02X\'\n }; \n\n", unsigned(data[n])) < 0)
    {
        std::free( data );
        return write_error();
    }
    return true;
}


bool process_files(const std::string &result, const std::string &include, 
    std::vector<std::string> &args, std::vector<Image> &images, std::vector<Shader> &shaders )
{
    //printf( "On process_files\n");
    File output(result.c_str(), "wb");  
    
    if(!output)
        return cannot_open(result.c_str());
    std::string short_res = base_filename( result );
    std::string short_inc = base_filename( include );
    //printf( "result: %s\n", base_filename( result ).c_str() );
    if(std::fprintf(output, "// %s : resource data\n//\n\n#include \"%s\"\n\n", short_res.c_str(), short_inc.c_str()) < 0)
        return write_error();

    for(size_t i = 0; i < args.size(); i++)
    {
        size_t len = args[i].length();
        switch(classify(args[i]))
        {
        case Type::image:
            {
                int width, height;
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
            std::printf("Invalid resource name \"%s\"!\n", args[i].c_str());  return false;
        }
    }

    if(std::fprintf(output, "\nconst ImageDesc images[] =\n{") < 0)return write_error();
    for(const auto &image : images)
    {
        std::string istr = filename_without_ext( image.name );
        if(std::fprintf(output, "\n    {\"%s\", image_%s, %uu, %uu},", istr.c_str(), istr.c_str(),
            unsigned(image.width), unsigned(image.height)) < 0)
            return write_error();
    }
    if(std::fprintf(output, "\n};\n") < 0)
        return write_error();

    if(std::fprintf(output, "\nconst ShaderDesc shaders[] =\n{") < 0)
        return write_error();
    for(const auto &shader : shaders)
    {
        std::string sstr = filename_without_ext( shader.name );
        if(std::fprintf(output, "\n    {\"%s\", shader_%s_%s, %uu},", sstr.c_str(), sstr.c_str(),
            shader.type == Type::shader_vert ? "vert" : "frag", unsigned(shader.length)) < 0)
            return write_error();
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

bool write_desc( const std::string &result,
    const std::vector<Image> &images, const std::vector<Shader> &shaders)
{
    //printf( "On write_desc\n");
    std::string short_name;
    File output(result.c_str(), "wb");  
    if(!output)
        return cannot_open(result.c_str());
    if(std::fprintf(output, "// %s : resource description\n//\n\n", result.c_str()) < 0)
        return write_error( "resource description");

    if(std::fprintf(output, "\nnamespace Image\n{\n    enum Index\n    {") < 0)
        return write_error( "namespace Image" );
    
    for(const auto &image : images)
    {
        short_name = filename_without_ext(image.name);
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
            short_name = filename_without_ext(shaders[i].name);
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
            short_name = filename_without_ext(shaders[i].name);
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

    //printf( "before parse cmdline: %i \n", n );
    CLI11_PARSE(cmd_line_parser, n, args);
    //printf( "after parse cmdline\n");

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

    //const size_t prefix = 6;
    std::vector<Image> images;  
    std::vector<Shader> shaders;  
    //png_init(0, 0);
    
    if(!process_files(result_cpp, result_h, files_list, images, shaders))
        return EXIT_FAILURE;
    return write_desc(result_h, images, shaders) ? EXIT_SUCCESS : EXIT_FAILURE;
}
