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

private:
	const static wl_registry_listener s_registry_listener;

	/* frame presented callback */
	const static wl_callback_listener s_callback_listener;
	static void frame_presented(void *data, struct wl_callback *frame, uint32_t time);

	/* buffer listener */
	const static wl_buffer_listener s_buffer_listener;
	static void buffer_release(void *, struct wl_buffer *);
};

#endif /* WAYLAND_DISPLAY_HXX_ */
