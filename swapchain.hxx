#include <stdio.h>
#include <stdint.h>
#include <wayland-client.h>
#include <cairo/cairo.h>

#include "circle.hxx"

class Point {
public:
    Point(int x=0, int y=0):
        m_x(x), m_y(y) {
    }

    ~Point() {}

    char *to_string();
        

private:
    int m_x, m_y;

private:
    char repr_format[100];
};


class Swapchain {
public:
    struct ImageData {
	ImageData():
	    buffer(nullptr), pixel(nullptr),
	    width(WIDTH), height(HEIGHT),
	    size(WIDTH * 4 * HEIGHT), busy(false) {
	}

	wl_buffer *buffer;
	uint8_t *pixel;
	const uint32_t  width;
	const uint32_t  height;
	const uint32_t  size;
    bool busy;
    };


public:
    enum {
        DRAW_BALL,
        DRAW_COLOR,
        DRAW_FRACTAL,
    };

    enum {
	FRONT_END_BUFFER = 0,
	BACK_END_BUFFER  = 1,
	NUM_OF_BUFFERS   = 3,
    };

    enum {
	WIDTH  = 400,
	HEIGHT = 300,
    };
	

public:
    Swapchain(struct wl_display *display=nullptr);
    ~Swapchain();

    void createSurface();
    void createBuffer();

    void draw(const int index);
    const int swapchainAcquireBuffer();
    void swapchainPresentBuffer(int);

    void setMode(const int mode) {
        m_mode = mode;
    }

    wl_display *display() {
        return m_display;
    }

    void close_handles();


public:

private:
    void init_connection();


protected:
    struct wl_display       *m_display;
    struct wl_registry      *m_registry;

    struct wl_compositor    *m_compositor;
    struct wl_shell         *m_shell;
    struct wl_shm           *m_shm;

    struct wl_surface       *m_surface;
    struct wl_shell_surface *m_shell_surface;

    struct wl_callback      *m_frame;

    struct wl_shm_pool      *m_pool;

    /**
     * drawing context
     */
    ImageData        m_images[NUM_OF_BUFFERS];
    void             *m_memory;
    uint32_t         m_memory_size;
    int m_backend_buffer_index;

    bool m_fifo_ready;

    /* shared memory format */
    int m_shm_format;

    /* cairo variables */
    //cairo_surface_t *m_cairo_surface;
    //cairo_context_t *ctx;

    int m_mode;

private:
    char m_error_msg[200];


private:
    static void clear_buffer(void *buf, const int, const int, const uint32_t color);
    static void buffer_release(void *data, struct wl_buffer *);
    static void frame_done(void *data, struct wl_callback *frame, uint32_t time);

#if 0
    static void cairo_draw(const uint32_t width,
		           const uint32_t height,
		           void *buf,
		           const int pattern);
#endif

    static const int create_anonymous_file(const int size); 
    static void draw_colorbars(const uint32_t width,
	                           const uint32_t height,
				               void *buf);

    static void draw_color(const uint32_t width,
                           const uint32_t height,
                           void *buf,
                           const uint32_t color);

    /**
     * registry listener callbacks
     */
    static void registry_handle_global_add(void *data,
                                           wl_registry *,
                                           uint32_t id,
                                           const char* interface,
                                           uint32_t version);

    static void registry_handle_global_remove(void *data,
                                              wl_registry *,
                                              uint32_t name);

    /**
     * shared memory format for an image
     */
    static void shm_memory_format(void *data,
                                  struct wl_shm *shm,
                                  uint32_t fmt);

    /**
     * shell surface listener callbacks
     */
    static void shell_surface_ping(void *data, 
                                   wl_shell_surface *,
                                   uint32_t serial);

    static void shell_surface_config(void *data,
                                     wl_shell_surface *,
                                     uint32_t edge,
                                     int32_t width,
                                     int32_t height);

 
private:
    /**
     * Wayland Listeners
     */
    static const struct wl_registry_listener s_registry_listener;
    static const struct wl_shm_listener s_shm_format;
    static const struct wl_shell_surface_listener s_shell_surface_listener;
    static const struct wl_buffer_listener buffer_listener;
    static const struct wl_callback_listener s_frame_listener;

    /* color table */
    static const uint32_t ColorTable[];

    static Circle *circle;
};
        
