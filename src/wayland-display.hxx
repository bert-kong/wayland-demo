/*
 * wayland-display.hxx
 *
 *  Created on: Jun 18, 2018
 *      Author: bert
 */

#ifndef WAYLAND_DISPLAY_HXX_
#define WAYLAND_DISPLAY_HXX_


#include <wayland-client.h>

class WaylandDisplay {
public:
	enum {
		NR_BUFFERS = 3,
		WIDTH      = 640,
		HEIGHT     = 480,
		NR_BALLS = 2,
	};

public:
	class Image {
	public:
		Image():
			buffer(nullptr), pixels(nullptr), in_use(false)
		{
		}
		~Image();

	private:
		wl_buffer *buffer;
		void *pixels;
		bool in_use;
	};
public:
	WaylandDisplay(const int width, const int height, const uint32_t format);
	~WaylandDisplay();

	void createSurface();
	void createBuffer();

	const int acquireBuffer();
	void draw(const int buf);
	void presentBuffer(const int buf);

private:
	void connect2server();
	const int create_anonymous_file(const uint32_t size);
	void clear_buffer(void *buf);

private:
	const int m_width;
	const int m_height;
	const uint32_t m_format;

	wl_display *m_display;
	wl_registry *m_registry;
	wl_compositor *m_compositor;
	wl_shell *m_shell;
	wl_surface *m_surface;
	wl_shell_surface *m_shell_surface;
	wl_shm *m_shm; /* shared memory */
	wl_shm_pool *m_shm_pool;
	wl_buffer *m_buffer;


	Image m_image[NR_BUFFERS];
	void *m_memory;
	bool m_frame_presented;

	Ball *m_ball[NR_BALLS];


private:
	const static wl_registry_listener s_registry_listener;
	static void registry_handle_global_add(void *data, struct wl_registry *, uint32_t, const char *, uint32_t);
	static void registry_handle_global_remove(void *data, struct wl_registry *, uint32_t);

	/* frame presented callback */
	const static wl_callback_listener s_callback_listener;
	static void frame_presented(void *data, struct wl_callback *frame, uint32_t time);

	/* surface listener */
	const static wl_shell_surface_listener s_shell_surface_listener;
	static void shell_surface_listener_ping(void *, struct wl_shell_surface*, uint32_t serial);
	static void shell_surface_listener_config(void *,
			                                  struct wl_shell_surface *,
											  uint32_t edges,
											  int32_t width,
											  int32_t height);

	/* buffer listener */
	const static wl_buffer_listener s_buffer_listener;
	static void buffer_release(void *, struct wl_buffer *);

	/* shared memory format listener */
	const static wl_shm_listener s_shm_format_listener;
	static void shm_memory_format_listner(void *data, struct wl_shm *, uint32_t fmt);

};

#endif /* WAYLAND_DISPLAY_HXX_ */
