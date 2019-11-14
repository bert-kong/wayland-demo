#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <cairo/cairo.h>
#include "swapchain.hxx"


/**
 * cairo fractal 
 */
#define INITIAL_LENGTH 50    // length of first branch
#define BRANCHES       14 
#define PI             3.1415926

static double rand_fl(){
  return (double)rand() / (double)RAND_MAX;
}
 
static void draw_tree(unsigned char *buf, 
                      const int w, const int h,
                      double offsetx, double offsety,
                      double directionx, double directiony, 
                      double size,
                      double rotation, int depth);



using namespace std;

#if 1
char *Point::to_string() 
{
    ::sprintf(repr_format, "(%d, %d)", m_x, m_y);
    return repr_format;
}
#endif

/**
 * Color class implementation
 */
class Color {
public:
    enum {
        WHITE   = 0xFFFFFFFF,
        RED     = 0xFFFF0000,
        GREEN   = 0xFF00FF00,
        BLUE    = 0xFF0000FF,
        YELLOW  = 0xFFFFFF00,
        CYAN    = 0xFF00FFFF,
        MAGENTA = 0xFFFF00FF,
        DEEPPINK= 0xFFFF1493,
        SKYBLUE = 0xFF87CEEB,
        BLACK   = 0xFF000000,
    };

public:
    Color(uint8_t r=0, uint8_t g=0, uint8_t b=0, uint8_t a=0):
        m_r(r), m_g(g), m_b(b), m_a(a) {
    }

public:
    static uint32_t rgb(uint32_t r, uint32_t g, uint32_t b) {
        return 0xFF000000 | (r<<16) | (g<<8) | b;
    }

    static uint32_t alpha(const uint32_t color) {
        return (color>>24) & 0xFF;
    }

    static uint32_t red(const uint32_t color) {
        return (color>>16) & 0xFF;
    }

    static uint32_t green(const uint32_t color) {
        return (color>>8) & 0xFF;
    }

    static uint32_t blue(const uint32_t color) {
        return (color) & 0xFF;
    }


private:
    uint8_t m_r, m_g, m_b, m_a;
};

const uint32_t Swapchain::ColorTable[] = {
    Color::RED,
    Color::GREEN,
    Color::BLUE,
    Color::YELLOW,
    Color::CYAN,
    Color::MAGENTA,
    Color::SKYBLUE,
    Color::WHITE,
    Color::DEEPPINK,
};



Circle *Swapchain::circle = new Circle[4];

/**
 * Swapchain Implementation
 */
Swapchain::Swapchain(struct wl_display *display) :
    m_display(NULL), m_registry(NULL), m_compositor(NULL),
    m_shell(nullptr), m_shm(nullptr), m_surface(NULL), 
    m_shell_surface(nullptr), m_pool(nullptr),
    m_images(), m_memory(nullptr), m_memory_size(0), 
    m_backend_buffer_index(0), m_shm_format(-1), m_fifo_ready(true),
    m_mode(Swapchain::DRAW_BALL)
{

    /* already configured */
    if (display) {
	    return;
    }

    ::srand(.10); 

    circle[0].setVelocity(3, 2);
    circle[1].setVelocity(4, 5);
    circle[2].setVelocity(1, 3);
    circle[3].setVelocity(7, 4);

    circle[0].setPosition(200, 100);
    circle[1].setPosition(90, 150);
    circle[2].setPosition(300, 200);
    circle[3].setPosition(250, 110);

    ::printf("Swapchain instance address %p\n", this);
    init_connection();
}

Swapchain::~Swapchain() {
    /**
     * release shell surface and surface
     */  
    if (m_shell_surface) {
        ::printf("debug ---> destroy shell surface\n");
        ::wl_shell_surface_destroy(m_shell_surface);
    }

    if (m_surface) {
        ::printf("debug ---> destroy surface\n");
        ::wl_surface_destroy(m_surface);
    }

    /**
     * destroy pool, buffer and memory
     */
    if (m_memory) {
       ::munmap(m_memory, m_memory_size);
    }

    for(int i=0; i<NUM_OF_BUFFERS; i++) {
        if (m_images[i].buffer) {
            wl_buffer_destroy(m_images[i].buffer);
        }
    }

    if (m_pool) {
        ::wl_shm_pool_destroy(m_pool);
    }

    /**
     * release the display
     */

    if (m_display) {
        ::printf("debug ---> disconnecting from the server\n");
        ::wl_display_disconnect(m_display);
    }
}

/**
 * main loop 
 */

const int Swapchain::swapchainAcquireBuffer() {
    int i, ret;

    while (true) {
        for(i=0; i<Swapchain::NUM_OF_BUFFERS; i++) {
            if (m_images[i].busy==false) {
                m_images[i].busy = true;
                return i;
            }
        }

        /** 
         * no released buffer
         */

        /* block until all pending request are processed by the server */
        ret = ::wl_display_roundtrip(m_display);

        if (ret<0) {
            throw std::runtime_error("Wayland display roundtrip error");
        }
    }

    // should not be here
    return -1;
    
}

void Swapchain::swapchainPresentBuffer(const int index) {
    /**
     * buffer was updated and ready to display
     */

    int ret;
    /* check the frame callback event, listener not called return */
    
    while (m_fifo_ready==false) {
        ret = ::wl_display_roundtrip(m_display);
        if (ret<0) {
            throw std::runtime_error("Wayland display dispatch failed");
        }
    }

    const int width = m_images[index].width;
    const int height = m_images[index].height;

    /* attached the backend buffer to the surface for drawing */
    ::wl_surface_attach(m_surface, m_images[index].buffer, 0, 0);

    /* mark the surface as dirty for rendering */
    ::wl_surface_damage(m_surface, 0, 0, width, height);

    /**
     * get the callback frame structure for listener
     * install the callback
     */
    m_frame = ::wl_surface_frame(m_surface);
    ::wl_callback_add_listener(m_frame, &s_frame_listener, this);
    m_fifo_ready = false;

    /* commit the damaged surface */
    ::wl_surface_commit(m_surface);
    ::wl_display_flush(m_display);
}


void Swapchain::init_connection() 
{
    /**
     * get the reference of the display from the server 
     */
    m_display = ::wl_display_connect(nullptr);

    if (m_display==nullptr) 
    {
        ::sprintf(m_error_msg, "debug ---> failed to get the display reference from the server");
        throw runtime_error(m_error_msg);
    }
            

    /**
     * get the registry reference
     */
    m_registry = ::wl_display_get_registry(m_display);
    ::printf("get the registry using display handle\n");
    if (!m_registry) {
        ::sprintf(m_error_msg, "debug ---> failed to get the registry reference from the server");
        throw runtime_error(m_error_msg);
    }

    /** 
     * enumerate and register interfaces and get references:
     *     compositor
     *     shell surface
     *     shared memory
     *     ...
     */
    ::wl_registry_add_listener(m_registry, &Swapchain::s_registry_listener, this);

    /*
     * invoke the registry listener 
     * get references for weston compositor, shell, shm
     */
    ::wl_display_dispatch(m_display);

    if (m_compositor==nullptr) {
        ::sprintf(m_error_msg, "debug ---> failed to get compositor");
        throw runtime_error(m_error_msg);
    }

    if (m_shell==nullptr) {
        ::sprintf(m_error_msg, "debug ---> failed to get shell");
        throw runtime_error(m_error_msg);
    }

    if (m_shm==nullptr) {
        ::sprintf(m_error_msg, "debug ---> failed to get shared memory");
	    throw runtime_error(m_error_msg);
    }

    ::wl_shm_add_listener(m_shm, &Swapchain::s_shm_format, this);
    ::wl_display_dispatch(m_display);

    ::printf("debug ---> connection successfully established (compositor, shell) = (%p, %p)\n", m_compositor, m_shell);
}


/**
 * static methods
 */
const int Swapchain::create_anonymous_file(const int size) {
    char filename[] = "/tmp/weston-shared-XXXXXX";
    const int fd = ::mkstemp(filename);

    /* get the original file settings */
    const uint32_t fd_setting = ::fcntl(fd, F_GETFD);
    
    ::fcntl(fd, fd_setting | FD_CLOEXEC);
    ::ftruncate(fd, size);

    ::unlink(filename);
    return fd;
}

/**
 * helper functions
 */
void Swapchain::draw_colorbars(const uint32_t width,
                                    const uint32_t height,
                                    void *buf) {
    const int nr_pixels = width/8;

    const uint8_t *base = (uint8_t *) buf;
    const int stride = width * 4;

    uint32_t *p;
    for(int x=0; x<height; x++) {
        p = (uint32_t *) (base + (x * stride));
        for(int y=0; y<width; y++) {
            switch(y/nr_pixels) {
                case 0:
                    p[y] = Color::WHITE;
                    break;
                case 1:
                    p[y] = Color::YELLOW;
                    break;
                case 2:
                    p[y] = Color::CYAN;
                    break;
                case 3:
                    p[y] = Color::GREEN;
                    break;
                case 4:
                    p[y] = Color::MAGENTA;
                    break;
                case 5:
                    p[y] = Color::RED;
                    break;
                case 6:
                    p[y] = Color::BLUE;
                    break;
                case 7:
                    p[y] = Color::BLACK;
                    break;
            }
        }
    }
}

void Swapchain::draw_color(const uint32_t width, 
	             const uint32_t height, 
		     void *buf,
		     const uint32_t color) {

    uint8_t *base = static_cast<uint8_t *>(buf);

    uint32_t *p;
    for(int y=0; y<height; y++) {
        p = (uint32_t *) (base + (y * width * 4));
        for(int x=0; x<width; x++) {
             p[x] = color;
        }
    }
}

#define ROTATION_SCALE 0.75 
#define SCALE 5

static cairo_t *create_cairo_context(cairo_surface_t *surf, unsigned char *buf, const int w, const int h) {
    int pitch = w * 4;
    surf = cairo_image_surface_create_for_data(buf, CAIRO_FORMAT_ARGB32, w, h, pitch);
    cairo_t *ct = cairo_create(surf);
    return ct;
}

static void tree_drawing(cairo_t * ct, 
                         double offsetx, double offsety,
                         double directionx, double directiony, 
                         double size,
                         double rotation, int depth,
                         const uint32_t color) {

    uint32_t a, r, g, b;

    a = Color::alpha(color)/255;
    r = Color::red(color)/255;
    g = Color::green(color)/255;
    b = Color::blue(color)/255;

    cairo_set_line_width(ct, 1);
    cairo_set_source_rgba(ct, r, g, b, a);
    cairo_move_to(ct, (int)offsetx, (int)offsety);
    cairo_line_to(ct, (int)(offsetx + directionx * size), (int)(offsety + directiony * size));
    cairo_stroke(ct);

    if (depth > 0) {
      // draw left branch
       tree_drawing(ct,
                    offsetx + directionx * size,
                    offsety + directiony * size,
                    directionx * cos(rotation) + directiony * sin(rotation),
                    directionx * -sin(rotation) + directiony * cos(rotation),
                    size * rand_fl() / SCALE + size * (SCALE - 1) / SCALE,
                    rotation * ROTATION_SCALE,
                    depth - 1, color);
 
       // draw right branch
       tree_drawing(ct,
                    offsetx + directionx * size,
                    offsety + directiony * size,
                    directionx * cos(-rotation) + directiony * sin(-rotation),
                    directionx * -sin(-rotation) + directiony * cos(-rotation),
                    size * rand_fl() / SCALE + size * (SCALE - 1) / SCALE,
                    rotation * ROTATION_SCALE,
                    depth - 1, color);
    }
}

void draw_tree(unsigned char *buf, 
               const int w, const int h,
               double offsetx, double offsety,
               double directionx, double directiony, 
               double size,
               double rotation, int depth, const uint32_t color) {

      cairo_surface_t *surf = nullptr;
      cairo_t *ct = create_cairo_context(surf, buf, w, h);
      tree_drawing(ct, offsetx, offsety, directionx, directiony, size, rotation, depth, color);

      cairo_destroy(ct);
      cairo_surface_destroy(surf);
}
 



/**
 * instance methods
 */


void Swapchain::createSurface() 
{
    /* asking the server/compositor to create surface */
    m_surface = ::wl_compositor_create_surface(m_compositor);
    if (m_surface==nullptr) {
        ::sprintf(m_error_msg, "debug ---> failed to create surface");
        throw runtime_error(m_error_msg);
    }

    /* get shell surface handle */
    m_shell_surface = ::wl_shell_get_shell_surface(m_shell, m_surface);
    if (m_shell_surface==nullptr) {
        ::sprintf(m_error_msg, "debug ---> failed to get/create shell surface");
        throw runtime_error(m_error_msg);
    }


    /**
     * add the shell surface listener
     */
    ::wl_shell_surface_add_listener(m_shell_surface,
                                    &Swapchain::s_shell_surface_listener,
                                    this);

    //::wl_shell_surface_set_transient(m_shell_surface, m_surface, rect_x, rect_y, 0);
    ::wl_shell_surface_set_toplevel(m_shell_surface);
    
#if 0
    /**
     * get the surface frame
     */
    m_frame = ::wl_surface_frame(m_surface);
    
    /* add the draw callback to the surface frame listener */
    ::wl_callback_add_listener(m_frame, &s_frame_listener, this);
#endif

    ///::wl_shell_surface_set_user_data(m_shell_surface, m_surface);

    //::wl_surface_set_user_data(m_surface, NULL);
    ::printf("debug ---> %s, (surface, shell_surface, callback_frame)=(%p, %p, %p)\n", 
                                __func__, m_surface, m_shell_surface, m_frame);
}
 
void Swapchain::close_handles()
{
    ::printf("debug bckong ---> %s\n", __func__);
    if (m_compositor) 
    {
        ::wl_compositor_destroy(m_compositor);
        m_compositor = nullptr;
    }

    if (m_shell)
    {
        ::wl_shell_destroy(m_shell);
        m_shell = nullptr;
    }

    if (m_shm)
    {
        ::wl_shm_destroy(m_shm);
        m_shm = nullptr;
    }

    if (m_registry)
    {
        ::wl_registry_destroy(m_registry);
        m_registry = nullptr;
    }

    if (m_display)
    {
        ::wl_display_disconnect(m_display);
        m_display = nullptr;
    }
}

/**
 * class Swapchain Static methods
 */
void Swapchain::registry_handle_global_add(void *data,
                                           wl_registry *registry,
                                           uint32_t id,
                                           const char *interface,
                                           uint32_t version) {

    Swapchain *client = (Swapchain *) data;
    /**
     * enumerate interfaces
     */

    ::printf("debug ---> %s, (%s, %d)\n", __func__, interface, id);

    /* get compositor handle */
    if (::strcmp(interface, wl_compositor_interface.name)==0) {
        client->m_compositor = (wl_compositor *) ::wl_registry_bind(registry, 
                                                                    id, 
                                                                    &wl_compositor_interface,
                                                                    1);
    }
    /* get/bind shell handle */
    else if (::strcmp(interface, wl_shell_interface.name)==0) {
        client->m_shell = (wl_shell *) ::wl_registry_bind(registry,
                                                          id,
                                                          &wl_shell_interface,
                                                          1);
    }
    /* get/bind shm handle */
    else if (::strcmp(interface, ::wl_shm_interface.name)==0) {
        client->m_shm = (wl_shm *) ::wl_registry_bind(registry,
                                                      id,
                                                      &wl_shm_interface,
                                                      1);
    }
    else if (::strcmp(interface, ::wl_seat_interface.name)==0) {
    }
}

void Swapchain::registry_handle_global_remove(void *data,
                                              wl_registry *registry,
                                              uint32_t name) {
    Swapchain *client = (Swapchain *) data;
    ::printf("debug ---> %s\n", __func__);

    client->close_handles();

}

/**
 * shm - shared memory listener
 */
void Swapchain::shm_memory_format(void *data,
	                              struct wl_shm *shm, 
                                  uint32_t fmt) {

    Swapchain *wsi = reinterpret_cast<Swapchain *>(data);
    printf("debug bckong ---> format 0x%08X\n", fmt);

    switch(fmt) {
	case WL_SHM_FORMAT_ARGB8888:
        wsi->m_shm_format = fmt;
	    ::printf("debug ---> %s:argb\n", __func__);
	    break;
	case WL_SHM_FORMAT_XRGB8888:
	    ::printf("debug ---> %s:xrgb\n", __func__);
	    break;
	case WL_SHM_FORMAT_RGB565:
	    ::printf("debug ---> %s:rgb\n", __func__);
	    break;
	case WL_SHM_FORMAT_RGB888:
		break;

	case WL_SHM_FORMAT_BGRA8888:
		break;

	default:
	    ::printf("debug ---> %s:default\n", __func__);
    }
}

void Swapchain::shell_surface_ping(void *data,
                                      struct wl_shell_surface *shell_surface,
                                      uint32_t serial) {

    Swapchain *client = reinterpret_cast<Swapchain *>(data);
    ::printf("debug bckong ---> %s:%p\n", __func__, client);
    ::wl_shell_surface_pong(shell_surface, serial);
}

void Swapchain::shell_surface_config(void *data,
                                        struct wl_shell_surface *shell_surface,
                                        uint32_t edges,
                                        int32_t width,
                                        int32_t height) {
    ::printf("debug ---> %s\n", __func__);
}

/**
 * class Swapchain Static variable initializations
 */
const struct wl_registry_listener Swapchain::s_registry_listener = {
    Swapchain::registry_handle_global_add,
    Swapchain::registry_handle_global_remove,
};

const struct wl_shm_listener Swapchain::s_shm_format = {
    Swapchain::shm_memory_format,
};

const struct wl_shell_surface_listener Swapchain::s_shell_surface_listener = {
    Swapchain::shell_surface_ping,
    Swapchain::shell_surface_config,
};

const struct wl_buffer_listener Swapchain::buffer_listener = {
    Swapchain::buffer_release,
};

const struct wl_callback_listener Swapchain::s_frame_listener = {
    Swapchain::frame_done,
};

void Swapchain::createBuffer() {
    // TODO: number of buffer setting
    const int stride = WIDTH * 4;
    const int img_size = stride * HEIGHT;
    const int total_memory_size = img_size * Swapchain::NUM_OF_BUFFERS;
    const int fd = Swapchain::create_anonymous_file(total_memory_size);

    /**
     * create a shared buffer pool 
     *    memory mapped assocated file descriptor
     */
    m_pool = ::wl_shm_create_pool(m_shm, fd, total_memory_size);

    /**
     * mapping image memory to user
     */
    m_memory = ::mmap(0,
                      total_memory_size,
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED,
                      fd,
                      0);

    if (m_memory==MAP_FAILED) 
    {
        ::sprintf(m_error_msg, "anonymous file mmap call failed\n");
        throw runtime_error(m_error_msg);
    }
    m_memory_size = total_memory_size;

    uint8_t *p = static_cast<uint8_t *>(m_memory);


    /* create buffers */
    int offset = 0;
    for(int i=0; i<Swapchain::NUM_OF_BUFFERS; i++) {
        /* buffer with the memory associated fd */
        m_images[i].buffer = ::wl_shm_pool_create_buffer(m_pool, 
                                                         offset,
                                                         WIDTH,
                                                         HEIGHT,
                                                         stride,
                                                         ::WL_SHM_FORMAT_ARGB8888);
        if (m_images[i].buffer==nullptr) {
            ::sprintf(m_error_msg, "wayland pool create buffer failed\n");
            throw runtime_error(m_error_msg);
        }
        ::printf("debug ---> %s: (%p, 0x%x)\n", __func__, m_images[i].buffer, offset);
        
        //m_images[i].pixel = static_cast<uint32_t *>(p + offset);
        m_images[i].pixel = p + offset;
        offset += img_size;

        ::wl_buffer_add_listener(m_images[i].buffer, 
                                 &Swapchain::buffer_listener,
                                 &m_images[i]);

        // TODO queue

        ::wl_display_roundtrip(m_display);
    }
    
    ::close(fd);
}

/**
 * static methods
 */

void Swapchain::buffer_release(void *data, wl_buffer *buffer) {
    Swapchain::ImageData *image = reinterpret_cast<Swapchain::ImageData *>(data);

    ::printf("debug ---> buffer released\n");

    if (image->buffer!=buffer) {
        throw std::runtime_error("buffer release : wrong buffer passed in\n");
    }

    image->busy = false;
}

void Swapchain::frame_done(void *data, struct wl_callback *frame, uint32_t time) {
    Swapchain *wsi = reinterpret_cast<Swapchain *>(data);

    wsi->m_fifo_ready = true;

    /* destroy callback struct */
    ::wl_callback_destroy(frame);
}

void Swapchain::clear_buffer(void *buf, 
                       const int width, 
                       const int height,
                       const uint32_t color) {
    const int stride = width * 4;
    uint8_t *base = (uint8_t *) buf;
    uint32_t *p;

    for(int y=0; y<height; y++) {
        p = (uint32_t *) (base + (y * stride));
        for(int x=0; x<width; x++) {
            p[x] = color;
        }
    }
}


void Swapchain::draw(const int index) {
    const uint32_t width  = m_images[index].width;
    const uint32_t height = m_images[index].height;
    uint8_t *pixel = m_images[index].pixel;
	const int color = ::rand()%(sizeof(Swapchain::ColorTable)/sizeof(uint32_t));


    ::printf("debug ---> %s : drawing on buffer %d, color %d\n", __func__, index, color);

    /**
     * draw on the current buffer 
     */
#if 0
    if (color>6) {
        Swapchain::draw_colorbars(width, height, pixel);
        return;
    }
#endif

    switch(m_mode) {
        case Swapchain::DRAW_BALL: 
            Swapchain::clear_buffer(pixel, width, height, 0xa0000000);
            Swapchain::circle[0].draw(pixel, Swapchain::ColorTable[color]);
            Swapchain::circle[1].draw(pixel, Swapchain::ColorTable[color]);
            Swapchain::circle[2].draw(pixel, Swapchain::ColorTable[color]);
            Swapchain::circle[3].draw(pixel, Swapchain::ColorTable[color]);
            break;

        case Swapchain::DRAW_COLOR: 
            Swapchain::clear_buffer(pixel, width, height, 0xa0000000);
            Swapchain::draw_colorbars(width, height, pixel);
#if 0
            if (color==7) {
                Swapchain::draw_colorbars(width, height, pixel);
            }
            else {
	            Swapchain::draw_color(width, height, pixel, Swapchain::ColorTable[color]);
            }
#endif
            break;

        case Swapchain::DRAW_FRACTAL:
            Swapchain::clear_buffer(pixel, width, height, 0x20000000);
            draw_tree((unsigned char *) pixel, width, height,
                      (float)width/2.0,
                      height - 10.0,
                      0.0, -1.0,
                      INITIAL_LENGTH,
                      PI/8.,
                      BRANCHES, Swapchain::ColorTable[color]);
            ::sleep(1);

            break;
    }
    
}



#if 0
void Swapchain::cairo_draw(const uint32_t width,
		     const uint32_t height,
		     void *buf,
		     const int pattern) {

    cairo_surface_t *surf = cairo_image_surface_create_for_data(static_cast<uint8_t char *>(buf),
                                                                CAIRO_FORMAT_ARGB32,
                                                                width,
                                                                height,
                                                                width * 4);


    cairo_t *ctx = cairo_create(surf);
}
#endif
