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

WaylandDisplay::WaylandDisplay(const int width,
		                       const int height,
							   const uint32_t format):
  m_width(width), m_height(height), m_format(format),
  m_display(nullptr)
{
	connect2server();
}

WaylandDisplay::~WaylandDisplay(void) {

}

void WaylandDisplay::connect2server() {

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

	for (int i=0; i<2; i++) {
		circle[i].draw(buf);
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
wl_callback_listener WaylandDisplay::s_callback_listener = {
    WaylandDisplay::frame_presented,
};

void WaylandDisplay::frame_presented(void *data, wl_callback *frame, uint32_t time) {
	WaylandDisplay *display = static_cast<WaylandDisplay *>(data);
	if (display) {
		display->m_frame_presented = true;
	}

	::wl_callback_destroy(frame);
}

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


