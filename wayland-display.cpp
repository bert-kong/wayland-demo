/*
 * wayland-display.cpp
 *
 *  Created on: Jun 18, 2018
 *      Author: bert
 */

#include <cstring>
#include <cstdlib>
#include <stdexcept>
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

	uint32_t offset = 0;
	for (int i=0; i<NR_BUFFERS; i++) {
		m_image[i].buffer = wl_shm_pool_create_buffer(m_shm_pool,
													  offset,
													  m_width,
													  m_height,
													  m_width * 4,
													  m_format);

		offset += m_width * 4 * m_height;
	}

}

const int WaylandDisplay::acquireBuffer(void) {

	for(int i=0; i<NR_BUFFERS; i++) {
		if (m_image[i].in_use) {
			continue;
		}
		return i;
	}
}

void WaylandDisplay::draw(const int index) {
	void *buf = m_image[index].pixels;


}

void WaylandDisplay::presentBuffer(const int index) {

}

