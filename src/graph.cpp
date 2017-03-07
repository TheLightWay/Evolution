// graph.cpp : world rendering
//

#include "graph.h"
#include "stream.h"
#include <algorithm>
#include <cassert>
#include <cmath>



void print_checksum(const World &world, const OutStream &stream)
{
    const uint32_t *checksum = static_cast<const uint32_t *>(stream.checksum());
    std::printf("Time: %llu, Checksum:", (unsigned long long)world.current_time);
    for(unsigned i = 0; i < Hash::result_size / 4; i++)
        std::printf(" %08lX", (unsigned long)bswap32(checksum[i]));
    std::printf("\n");
}



// Camera struct

void Camera::update_scale()
{
    scale = std::exp2(tile_order - scale_step * log_scale);
}

Camera::Camera(SDL_Window *window) : x(0), y(0), log_scale(8 / scale_step)
{
    SDL_GetWindowSize(window, &width, &height);  update_scale();
}

void Camera::apply() const
{
    glViewport(0, 0, width, height);
}


void Camera::resize(int w, int h)
{
    width = w;  height = h;
}

void Camera::move(int dx, int dy)
{
    x -= std::lround(dx * scale);
    y -= std::lround(dy * scale);
}

void Camera::rescale(int delta)
{
    log_scale += delta;
    //if(log_scale < min_scale)log_scale = min_scale;
    //if(log_scale > max_scale)log_scale = max_scale;
    double old = scale;  update_scale();

    int x0, y0;
    SDL_GetMouseState(&x0, &y0);
    x -= std::lround(x0 * (scale - old));
    y -= std::lround(y0 * (scale - old));
}



// Representation class

struct Vertex
{
    GLfloat x, y;
};

struct Triangle
{
    GLubyte p0, p1, p2;

    Triangle() = default;

    Triangle(int p0, int p1, int p2) : p0(p0), p1(p1), p2(p2)
    {
    }
};

struct FoodData
{
    GLfloat x, y, rad, type;

    void set(const Config &config, const Food &food)
    {
        x = food.pos.x * draw_scale;
        y = food.pos.y * draw_scale;
        rad = config.base_radius * draw_scale;
        type = food.type - Food::Grass;
    }
};

struct CreatureData
{
    GLfloat x, y, rad[3];
    GLubyte angle, signal, energy, life;

    void set(const Config &config, const Creature &cr)
    {
        GLfloat energy_mul = config.base_r2 * draw_scale * draw_scale / (1 << 16);
        GLfloat life_mul = energy_mul * config.hide_mul / config.life_mul;

        x = cr.pos.x * draw_scale;
        y = cr.pos.y * draw_scale;

        GLfloat sqr = cr.passive_energy * energy_mul - cr.max_life * life_mul;
        rad[0] = sqrt(sqr);  sqr += cr.max_energy * energy_mul;
        rad[1] = sqrt(sqr);  sqr += cr.max_life * life_mul;
        rad[2] = sqrt(sqr);

        angle = cr.angle;  signal = cr.flags;
        energy = std::lround(255.0 * cr.energy / cr.max_energy);
        life = std::lround(255.0 * cr.total_life / cr.max_life);
    }
};

struct GuiQuad
{
    GLshort x, y;
    GLubyte tx, ty, width, height;

    GuiQuad() = default;

    GuiQuad(GLshort x, GLshort y, GLubyte tx, GLubyte ty, GLubyte width, GLubyte height) :
        x(x), y(y), tx(tx), ty(ty), width(width), height(height)
    {
    }
};


GLuint load_shader(GLint type, const char *name, const char *data, GLint size)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &data, &size);

    char msg[65536];  GLsizei len;
    glCompileShader(shader);  glGetShaderInfoLog(shader, sizeof(msg), &len, msg);
    if(len)std::printf("%s shader log:\n%s\n", name, msg);  return shader;
}

void Representation::create_program(Pass pass, Shader::Index id)
{
    prog[pass] = glCreateProgram();  const ShaderDesc &shader = shaders[id];
    GLuint vert = load_shader(GL_VERTEX_SHADER, "Vertex", shader.vert_src, shader.vert_len);
    GLuint frag = load_shader(GL_FRAGMENT_SHADER, "Fragment", shader.frag_src, shader.frag_len);
    glAttachShader(prog[pass], vert);  glAttachShader(prog[pass], frag);

    char msg[65536];  GLsizei len;
    glLinkProgram(prog[pass]);  glGetProgramInfoLog(prog[pass], sizeof(msg), &len, msg);
    if(len)std::printf("Shader program \"%s\" log:\n%s\n", shader.name, msg);

    glDetachShader(prog[pass], vert);  glDetachShader(prog[pass], frag);
    glDeleteShader(vert);  glDeleteShader(frag);
}


void Representation::make_food_shape()
{
    constexpr int n = 3, m = 2 * n - 2;
    elem_count[pass_food] = 3 * m + 3;
    obj_count[pass_food] = 0;

    Vertex vertex[2 * n];
    for(int i = 0; i < 2 * n; i++)
    {
        double r = i & 1 ? 1.0 : 0.25;
        vertex[i].x = r * std::sin(i * (pi / n));
        vertex[i].y = r * std::cos(i * (pi / n));
    }

    Triangle triangle[m + 1];
    for(int i = 0; i < m; i += 2)
    {
        triangle[i + 0] = Triangle(i, i + 1, i + 2);
        triangle[i + 1] = Triangle(m, i,     i + 2);
    }
    triangle[m] = Triangle(m, m + 1, 0);

    glBindBuffer(GL_ARRAY_BUFFER, buf[vtx_food]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf[idx_food]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW);
}

void Representation::make_creature_shape()
{
    constexpr int n = 8;
    elem_count[pass_creature] = 3 * n;
    obj_count[pass_creature] = 0;

    Vertex vertex[n + 1];
    double r = 1 / std::cos(pi / n);
    for(int i = 0; i < n; i++)
    {
        vertex[i].x = r * std::sin(i * (2 * pi / n));
        vertex[i].y = r * std::cos(i * (2 * pi / n));
    }
    vertex[n].x = vertex[n].y = 0;

    Triangle triangle[n];  triangle[0] = Triangle(n, n - 1, 0);
    for(int i = 1; i < n; i++)triangle[i] = Triangle(n, i - 1, i);

    glBindBuffer(GL_ARRAY_BUFFER, buf[vtx_creature]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf[idx_creature]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW);
}

void Representation::make_quad_shape()
{
    elem_count[pass_gui] = 4;
    obj_count[pass_gui] = 0;

    Vertex vertex[4] =
    {
        {0, 0}, {0, 1}, {1, 0}, {1, 1}
    };

    glBindBuffer(GL_ARRAY_BUFFER, buf[vtx_gui]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);
}


void register_attribute(GLuint index, int vec_size, GLenum type, GLboolean norm, size_t stride, size_t offs)
{
    glEnableVertexAttribArray(index);  if(index)glVertexAttribDivisor(index, 1);
    glVertexAttribPointer(index, vec_size, type, norm, stride, reinterpret_cast<void *>(offs));
}

Representation::Representation()
{
    create_program(pass_food, Shader::food);
    i_transform[pass_food] = glGetUniformLocation(prog[pass_food], "transform");

    create_program(pass_creature, Shader::creature);
    i_transform[pass_creature] = glGetUniformLocation(prog[pass_creature], "transform");

    create_program(pass_gui, Shader::gui);
    i_transform[pass_gui] = glGetUniformLocation(prog[pass_gui], "transform");
    i_gui = glGetUniformLocation(prog[pass_gui], "gui");


    glGenVertexArrays(pass_count, arr);
    glGenBuffers(buf_count, buf);
    glGenTextures(1, &tex_gui);

    glBindVertexArray(arr[pass_food]);
    glBindBuffer(GL_ARRAY_BUFFER, buf[vtx_food]);
    register_attribute(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glBindBuffer(GL_ARRAY_BUFFER, buf[inst_food]);
    register_attribute(1, 4, GL_FLOAT, GL_FALSE, sizeof(FoodData), 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf[idx_food]);

    glBindVertexArray(arr[pass_creature]);
    glBindBuffer(GL_ARRAY_BUFFER, buf[vtx_creature]);
    register_attribute(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glBindBuffer(GL_ARRAY_BUFFER, buf[inst_creature]);
    register_attribute(1, 2, GL_FLOAT, GL_FALSE, sizeof(CreatureData), 0);
    register_attribute(2, 3, GL_FLOAT, GL_FALSE, sizeof(CreatureData), 2 * sizeof(GLfloat));
    register_attribute(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(CreatureData), 5 * sizeof(GLfloat));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf[idx_creature]);

    glBindVertexArray(arr[pass_gui]);
    glBindBuffer(GL_ARRAY_BUFFER, buf[vtx_gui]);
    register_attribute(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glBindBuffer(GL_ARRAY_BUFFER, buf[inst_gui]);
    register_attribute(1, 2, GL_SHORT, GL_FALSE, sizeof(GuiQuad), 0);
    register_attribute(2, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(GuiQuad), 2 * sizeof(GLshort));

    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, tex_gui);  const ImageDesc &image = images[Image::gui];
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    make_food_shape();
    make_creature_shape();
    make_quad_shape();
}

Representation::~Representation()
{
    for(int pass = 0; pass < pass_count; pass++)glDeleteProgram(prog[pass]);
    glDeleteVertexArrays(pass_count, arr);  glDeleteBuffers(buf_count, buf);
    glDeleteTextures(1, &tex_gui);
}


namespace Gui
{
    constexpr unsigned digit_width  =  8;
    constexpr unsigned icon_width   = 16;
    constexpr unsigned line_height  = 16;
    constexpr unsigned line_spacing = 24;

    constexpr unsigned flag_pos     = 16;
    constexpr unsigned flag_width   =  8;
    constexpr unsigned flag_height  =  8;
    constexpr unsigned flag_row     =  3;
    constexpr unsigned end_flag = 1 << 7;

    constexpr unsigned panel_width = 5 * icon_width + 20 * digit_width;

    constexpr unsigned icon_offset  = 16;
    constexpr unsigned icon_row     =  8;

    enum Icon
    {
        i_weight = Slot::invalid + 1, i_target, i_volume,
        i_angle, i_radius, i_damage, i_life, i_speed, i_none = 0
    };

    struct TypeIcons
    {
        Icon base, angle1, angle2, radius;
        uint8_t flag_count;
    };

    const TypeIcons icons[Slot::invalid + 1] =
    {
        {i_none,   i_none,  i_none,  i_none,   0},  // neiron
        {i_none,   i_none,  i_none,  i_none,   0},  // mouth
        {i_volume, i_none,  i_none,  i_none,   0},  // stomach
        {i_volume, i_none,  i_none,  i_none,   0},  // womb
        {i_none,   i_angle, i_angle, i_radius, 6},  // eye
        {i_none,   i_angle, i_angle, i_none,   6},  // radar
        {i_damage, i_angle, i_angle, i_radius, 0},  // claw
        {i_life,   i_none,  i_none,  i_none,   0},  // hide
        {i_speed,  i_angle, i_none,  i_none,   0},  // leg
        {i_none,   i_none,  i_angle, i_none,   0},  // rotator
        {i_none,   i_none,  i_none,  i_none,   3},  // signal
        {i_none,   i_none,  i_none,  i_none,   0},  // invalid
    };
}

int write_number(GuiQuad *&data, uint32_t num, int x, int y)
{
    do
    {
        x -= Gui::digit_width;
        unsigned tx = (num % 10) * Gui::digit_width;  num /= 10;
        *data++ = GuiQuad(x, y, tx, 0, Gui::digit_width, Gui::line_height);
    }
    while(num);  return x;
}

void put_icon(GuiQuad *&data, int x, int y, unsigned index)
{
    index += Gui::icon_offset;
    unsigned tx = (index % Gui::icon_row) * Gui::icon_width;
    unsigned ty = (index / Gui::icon_row) * Gui::line_height;
    *data++ = GuiQuad(x, y, tx, ty, Gui::icon_width, Gui::line_height);
}

void put_flags(GuiQuad *&data, int x, int y, unsigned flags, unsigned flag_count)
{
    unsigned flag = Gui::end_flag >> flag_count;
    unsigned rows = (flag_count - 1) / Gui::flag_row + 1;
    y += (Gui::line_height - rows * Gui::flag_height) / 2;
    for(unsigned i = 0; i < flag_count; i++)
    {
        unsigned ty = flags & (flag << i) ? Gui::flag_pos : Gui::flag_pos + Gui::flag_height;
        *data++ = GuiQuad(x, y, i * Gui::flag_width, ty, Gui::flag_width, Gui::flag_height);
        x += Gui::flag_width;  if((i + 1) % Gui::flag_row)continue;
        x -= Gui::flag_row * Gui::flag_width;  y += Gui::flag_height;
    }
}

size_t Representation::Selection::make_creature(const World &world, bool skipUnused)
{
    constexpr unsigned num_width = 4 * Gui::digit_width;
    constexpr unsigned slot_offset = num_width + Gui::icon_width;

    proc.process(world.config, cr->genome);
    std::vector<GuiQuad> buf(22 * proc.slot_count);
    GuiQuad *data = buf.data();

    int y = 0;
    for(uint32_t i = 0; i < proc.slot_count; i++)
    {
        if(skipUnused && !proc.slots[i].used)continue;

        int x = num_width;
        write_number(data, i, x, y);
        put_icon(data, x, y, proc.slots[i].type);
        const Gui::TypeIcons &icons = Gui::icons[proc.slots[i].type];
        if(icons.base)
        {
            write_number(data, proc.slots[i].base - 1, x += slot_offset, y);
            put_icon(data, x, y, icons.base);
        }
        if(icons.angle1)
        {
            write_number(data, proc.slots[i].angle1, x += slot_offset, y);
            put_icon(data, x, y, icons.angle1);
        }
        if(icons.angle2)
        {
            write_number(data, proc.slots[i].angle2, x += slot_offset, y);
            put_icon(data, x, y, icons.angle2);
        }
        if(icons.radius)
        {
            write_number(data, proc.slots[i].radius, x += slot_offset, y);
            put_icon(data, x, y, icons.radius);
        }
        if(icons.flag_count)
        {
            x += Gui::icon_width + Gui::flag_width;
            put_flags(data, x, y, proc.slots[i].flags, icons.flag_count);
        }
        y += Gui::line_spacing;
    }

    glBufferData(GL_ARRAY_BUFFER, (data - buf.data()) * sizeof(GuiQuad), buf.data(), GL_STATIC_DRAW);
    return data - buf.data();
}

bool hit_test(const World::Tile &tile,
    uint64_t x0, uint64_t y0, uint64_t max_r2, const Creature *&sel, uint64_t prev_id)
{
    for(const Creature *cr = tile.first; cr; cr = cr->next)
    {
        int32_t dx = cr->pos.x - x0;
        int32_t dy = cr->pos.y - y0;
        uint64_t r2 = int64_t(dx) * dx + int64_t(dy) * dy;
        if(r2 >= max_r2)continue;

        if(cr->id == prev_id && sel)return true;  sel = cr;
    }
    return false;
}

const Creature *hit_test(const World &world, uint64_t x0, uint64_t y0, uint32_t rad, uint64_t prev_id)
{
    uint32_t x1 = (x0 - rad) >> tile_order & world.config.mask_x;
    uint32_t y1 = (y0 - rad) >> tile_order & world.config.mask_y;
    uint32_t x2 = (x0 + rad) >> tile_order & world.config.mask_x;
    uint32_t y2 = (y0 + rad) >> tile_order & world.config.mask_y;

    uint64_t r2 = uint64_t(rad) * rad;
    bool test[] = {true, x1 != x2, y1 != y2, x1 != x2 && y1 != y2};  const Creature *sel = nullptr;
    if(test[0] && hit_test(world.tiles[x1 | (y1 << world.config.order_x)], x0, y0, r2, sel, prev_id))return sel;
    if(test[1] && hit_test(world.tiles[x2 | (y1 << world.config.order_x)], x0, y0, r2, sel, prev_id))return sel;
    if(test[2] && hit_test(world.tiles[x1 | (y2 << world.config.order_x)], x0, y0, r2, sel, prev_id))return sel;
    if(test[3] && hit_test(world.tiles[x2 | (y2 << world.config.order_x)], x0, y0, r2, sel, prev_id))return sel;
    return sel;
}

void Representation::select(const World &world, Camera &cam, int32_t x, int32_t y)
{
    constexpr int click_zone = 8;

    uint64_t x0 = cam.x + std::lround(x * cam.scale);
    uint64_t y0 = cam.y + std::lround(y * cam.scale);
    uint32_t rad = std::min<long>(tile_size, world.config.base_radius + std::lround(click_zone * cam.scale));
    sel.cr = hit_test(world, x0, y0, rad, sel.id);
    if(!sel.cr)
    {
        sel.id = uint64_t(-1);  obj_count[pass_gui] = 0;  return;
    }
    if(sel.cr->id == sel.id)return;

    sel.id = sel.cr->id;  sel.pos = sel.cr->pos;

    glBindBuffer(GL_ARRAY_BUFFER, buf[inst_gui]);
    obj_count[pass_gui] = sel.make_creature(world, true);
}

void Representation::update(SDL_Window *window, const World &world, Camera &cam, bool checksum)
{
    obj_count[pass_food] = world.total_food_count;
    glBindBuffer(GL_ARRAY_BUFFER, buf[inst_food]);  FoodData *food_ptr = nullptr;
    glBufferData(GL_ARRAY_BUFFER, obj_count[pass_food] * sizeof(FoodData), nullptr, GL_STREAM_DRAW);
    if(obj_count[pass_food])food_ptr = static_cast<FoodData *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

    obj_count[pass_creature] = world.total_creature_count;
    glBindBuffer(GL_ARRAY_BUFFER, buf[inst_creature]);  CreatureData *creature_ptr = nullptr;
    glBufferData(GL_ARRAY_BUFFER, obj_count[pass_creature] * sizeof(CreatureData), nullptr, GL_STREAM_DRAW);
    if(obj_count[pass_creature])creature_ptr = static_cast<CreatureData *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

#ifndef NDEBUG
    FoodData *food_end = food_ptr + obj_count[pass_food];
    CreatureData *creature_end = creature_ptr + obj_count[pass_creature];
#endif
    sel.cr = nullptr;
    for(const auto &tile : world.tiles)
    {
        for(const auto &food : tile.foods)if(food.type > Food::Sprout)
            (food_ptr++)->set(world.config, food);

        for(const Creature *cr = tile.first; cr; cr = cr->next)
        {
            if(cr->id == sel.id)sel.cr = cr;
            (creature_ptr++)->set(world.config, *cr);
        }
    }
    assert(food_ptr == food_end);
    assert(creature_ptr == creature_end);

    if(sel.cr)
    {
        cam.x += sel.cr->pos.x - sel.pos.x;
        cam.y += sel.cr->pos.y - sel.pos.y;
        sel.pos = sel.cr->pos;
    }

    if(food_ptr)
    {
        glBindBuffer(GL_ARRAY_BUFFER, buf[inst_food]);
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }
    if(creature_ptr)
    {
        glBindBuffer(GL_ARRAY_BUFFER, buf[inst_creature]);
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    if(checksum || !(world.current_time % 1000))
    {
        OutStream stream;  stream.initialize();
        stream << world;  stream.finalize();
        print_checksum(world, stream);
    }

    char buf[256];
    snprintf(buf, sizeof(buf),
        "Evolution - Time: %llu, Food: %lu, Creature: %lu", (unsigned long long)world.current_time,
        (unsigned long)world.total_food_count, (unsigned long)world.total_creature_count);
    SDL_SetWindowTitle(window, buf);
}

void Representation::draw(const World &world, const Camera &cam)
{
    cam.apply();
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);

    double mul_x = 2 / (cam.width  * cam.scale);
    double mul_y = 2 / (cam.height * cam.scale);
    uint64_t x = cam.x & world.config.full_mask_x;
    uint64_t y = cam.y & world.config.full_mask_y;
    int order_x = world.config.order_x + tile_order;
    int order_y = world.config.order_y + tile_order;
    int nx = (x + cam.width  * cam.scale) * std::exp2(-order_x);
    int ny = (y + cam.height * cam.scale) * std::exp2(-order_y);
    double dx = mul_x * std::exp2(order_x), x0 = x * mul_x + 1;
    double dy = mul_y * std::exp2(order_y), y0 = y * mul_y + 1;

    for(int pass = 0; pass < pass_gui; pass++)
    {
        glBindVertexArray(arr[pass]);  glUseProgram(prog[pass]);
        for(int i = 0; i <= ny; i++)for(int j = 0; j <= nx; j++)
        {
            glUniform4f(i_transform[pass], j * dx - x0, y0 - i * dy, mul_x * tile_size, -mul_y * tile_size);
            glDrawElementsInstanced(GL_TRIANGLES, elem_count[pass], GL_UNSIGNED_BYTE, nullptr, obj_count[pass]);
        }
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    mul_x = 2.0 / cam.width;  mul_y = 2.0 / cam.height;
    glBindVertexArray(arr[pass_gui]);  glUseProgram(prog[pass_gui]);
    glUniform4f(i_transform[pass_gui], 1 - Gui::panel_width * mul_x, 1, mul_x, -mul_y);
    glUniform1i(i_gui, 0);  glActiveTexture(GL_TEXTURE0);  glBindTexture(GL_TEXTURE_2D, tex_gui);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, elem_count[pass_gui], obj_count[pass_gui]);
}
