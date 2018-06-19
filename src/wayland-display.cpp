/*
 * wayland-display.cpp
 *
 *  Created on: Jun 18, 2018
 *      Author: bert
 */

#include <cstring>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <exception>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>


#include "wayland-display.hxx"
#include "ball.hxx"


WaylandDisplay::WaylandDisplay(const int width,
		                       const int height,
							   const uint32_t format):
  m_width(width), m_height(height), m_format(format),
  m_display(nullptr)
{
	m_ball[0] = new  Ball(20, 0xffff0000, {0, 0, width, height});
	m_ball[0]->setPosition(50, 100);

	m_ball[1] = new  Ball(20, 0xff0000ff, {0, 0, width, height});
	m_ball[0]->setPosition(300, 150);
	m_ball[0]->setVelocity(4, 1);

	connect2server();
}

WaylandDisplay::~WaylandDisplay(void) {
	if (m_ball[0]) {
		delete m_ball[0];
	}
	if (m_ball[1]) {
		delete m_ball[1];
	}
}

void WaylandDisplay::connect2server() {
	m_display = ::wl_display_connect(nullptr);

	m_registry = ::wl_display_get_registry(m_display);

	::wl_registry_add_listener(m_registry, &WaylandDisplay::s_registry_listener, this);

	/* send request to the server */
	::wl_display_dispatch(m_display);

	/* check for interfaces */
	if (m_compositor==nullptr) {

	}

	if (m_shell==nullptr) {

	}

	if (m_shm==nullptr) {

	}

	::wl_shm_add_listener(m_shm, &WaylandDisplay::s_shm_format_listener, this);

}

void WaylandDisplay::clear_buffer(void *buf) {
	::memset(buf, m_width * 4 * m_height, 0);
}

const int WaylandDisplay::create_anonymous_file(const uint32_t size) {
	char filename[] = "/tmp/weston-shared-XXXXXX";
	const int fd = ::mktemp(filename);

	const uint32_t fd_setting = ::fcntl(fd, F_GETFD);

	/* change an attributes of the file */
	::fcntl(fd, fd_setting | FD_CLOEXEC);
	/* set size of the file */
	::ftruncate(fd, size);
	::unlink(filename);

	return fd;
}

void WaylandDisplay::createSurface() {
	m_surface = ::wl_compositor_create_surface(m_compositor);

	m_shell_surface = ::wl_shell_get_shell_surface(m_shell, m_surface);

	::wl_shell_surface_add_listener(m_shell_surface, &WaylandDisplay::s_shell_surface_listener, this);

	::wl_shell_surface_set_toplevel(m_shell_surface);

}

void WaylandDisplay::createBuffer() {
	const uint32_t size = m_width * 4 * m_height * NR_BUFFERS;
	const uint32_t fd = create_anonymous_file(size);

	/* shared memory fd and size */
	m_shm_pool = wl_shm_create_pool(m_shm, fd, size);

	/* map the memory for the display client */
	m_memory = ::mmap(0,
			          size,
					  PROT_READ | PROT_WRITE,
					  MAP_SHARED,
					  fd,
					  0);

	if (m_memory==MAP_FAILED) {
		throw std::runtime_error("mmap anonymous failed");
	}

	uint32_t offset = 0;
	uint8_t *p = static_cast<uint8_t *>(m_memory);
	const uint32_t img_size = m_width * 4 * m_height;
	for (int i=0; i<NR_BUFFERS; i++) {
		m_image[i].buffer = wl_shm_pool_create_buffer(m_shm_pool,
													  offset,
													  m_width,
													  m_height,
													  m_width * 4,
													  m_format);

		m_image[i].pixels = static_cast<void>(p+offset);
		::wl_buffer_add_listener(m_image[i].buffer,
				                 &WaylandDisplay::s_buffer_listener,
								 &m_image[i]);
		::wl_display_roundtrip(m_display);

		offset += img_size;
	}
}

const int WaylandDisplay::acquireBuffer(void) {

	while (true) {
		for(int i=0; i<NR_BUFFERS; i++) {
			if (m_image[i].in_use) {
				continue;
			}

			m_image[i].in_use = true;
			return i;
		}

		/* no free buffer */
		::wl_display_roundtrip(m_display);
	}

	/* should not be here */
	return -1;
}

void WaylandDisplay::draw(const int index) {
	void *buf = m_image[index].pixels;

	WaylandDisplay::clear_buffer(buf);

	for (int i=0; i<NR_BALLS; i++) {
		m_ball[i]->draw(buf);
	}
}

void WaylandDisplay::presentBuffer(const int index) {
	int ret;

	/* check if the frame was displayed yet */
	while (m_frame_presented==false) {
		ret = ::wl_display_roundtrip(m_display);
		if (ret<0) {
			throw std::runtime_error("Wayland display roundtrip error");
		}
	}

	/* attach the ready-render buffer to the surface for displaying */
	::wl_surface_attach(m_surface, m_image[index].buffer, 0, 0);

	/* mark the surface dirty for rendering */
	::wl_surface_damage(m_surface, 0, 0, m_width, m_height);

	wl_callback *frame = ::wl_surface_frame(m_surface);
	::wl_callback_add_listener(frame, &WaylandDisplay::s_callback_listener, this);
	m_frame_presented = false;

	/* send a request - rendering surface, to the server */
	::wl_surface_commit(m_surface);
	::wl_display_flush(m_display);
}

/**
 * Wayland Listeners
 */

/////////////////////////////////////////////////////////////////////
struct wl_registry_listener WaylandDisplay::s_registry_listener = {
	WaylandDisplay::registry_handle_global_add,
	WaylandDisplay::registry_handle_global_remove,
};

void WaylandDisplay::registry_handle_global_add(void *data,
		                                        struct wl_registry *registry,
												uint32_t id,
												const char *interface_name,
												uint32_t version) {
	WaylandDisplay *display = static_cast<WaylandDisplay *>(data);

	if (::strcmp(interface_name, wl_compositor_interface.name)==0) {
		display->m_compositor = static_cast<struct wl_compositor *>(::wl_registry_bind(registry,
				                                                                       id,
																					   &wl_compositor_interface,
																					   1));

	}
	else if (::strcmp(interface_name, wl_shell_interface.name)==0) {
		display->m_shell = static_cast<struct wl_shell *>(::wl_registry_bind(registry,
				                                                             id,
																			 &wl_shell_interface,
																			 1));
	}
	else if (::strcmp(interface_name, wl_shm_interface.name)==0) {
		display->m_shm = static_cast<struct wl_shm *>(::wl_registry_bind(registry,
				                                                         id,
																		 &wl_shm_interface,
																		 1));
	}
}


void WaylandDisplay::registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
	::printf("debug ---> %s\n", __func__);
}


/////////////////////////////////////////////////////////////////////
struct wl_shm_listener WaylandDisplay::s_shm_format_listener = {
    WaylandDisplay::shm_memory_format_listner,
};

void WaylandDisplay::shm_memory_format_listner(void *data, struct wl_shm *shm, uint32_t fmt) {
	WaylandDisplay *display = static_cast<WaylandDisplay *>(data);

	switch (fmt) {
	case WL_SHM_FORMAT_ABGR8888:
		break;
	case WL_SHM_FORMAT_ARGB8888:
		display->m_format = fmt;
		break;
	}
}

/////////////////////////////////////////////////////////////////////
struct wl_surface_listener WaylandDisplay::s_shell_surface_listener = {
	WaylandDisplay::shell_surface_listener_ping,
	WaylandDisplay::shell_surface_listener_config,
};

void WaylandDisplay::shell_surface_listener_ping(void *data,
		                                         struct wl_shell_surface *shell_surface,
												 uint32_t serial) {
	::wl_shell_surface_pong(shell_surface, serial);
}

void WaylandDisplay::shell_surface_listener_config(void *,
		                                           struct wl_shell_surface *shell_surface,
												   uint32_t edge,
												   int32_t width,
												   int32_t height) {

}


/////////////////////////////////////////////////////////////////////
struct wl_callback_listener WaylandDisplay::s_callback_listener = {
    WaylandDisplay::frame_presented,
};

void WaylandDisplay::frame_presented(void *data, wl_callback *frame, uint32_t time) {
	WaylandDisplay *display = static_cast<WaylandDisplay *>(data);
	if (display) {
		display->m_frame_presented = true;
	}

	::wl_callback_destroy(frame);
}


///////////////////////////////////////////////////////////////////////
wl_buffer_listener WaylandDisplay::s_buffer_listener = {
	WaylandDisplay::buffer_release,
};

void WaylandDisplay::buffer_release(void *data, struct wl_buffer *buffer) {
	WaylandDisplay::Image *image = static_cast<WaylandDisplay::Image *>(data);

	if (image==nullptr) {
		return;
	}

	if (image->buffer!=buffer) {
		throw std::runtime_error("buffer callback error");
	}

	image->in_use = false;
}

