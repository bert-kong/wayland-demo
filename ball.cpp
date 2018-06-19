/*
 * ball.cpp
 *
 *  Created on: Jun 18, 2018
 *      Author: bert
 *  code for drawing circle:
 *  https://www.geeksforgeeks.org/bresenhams-circle-drawing-algorithm/
 */

#include "ball.hxx"

void Ball::update() {
	int x, y;
	bool changed_direction = false;

	(x = m_pos[0], y = m_pos[1]);

	x += m_vel[0];
	y += m_vel[1];

	if ((m_x + m_radius)>x || (m_x + m_width)-m_radius<x) {
		m_vel[0] *= -1;
		changed_direction = true;
	}

	if ((m_y + m_radius)>0 || (m_y + m_height)-m_radius<y) {
		m_vel[1] *= -1;
		changed_direction = true;
	}

	if (changed_direction) {
		return;
	}

	m_pos[0] = x, m_pos[1] = y;

}

void Ball::draw(void *buf) {
	m_canvas = static_cast<uint8_t *>(buf);

	update();

	int x = 0;
	int d = 3 - (2 * m_radius);
	int y = m_radius;
	while (y>x) {
		draw_circle(m_pos[0], m_pos[1], x, y);
		x++;

		if (d>0) {
			y--;
			d = d + (4 * (x-y)) + 10;
		}
		else {
			d = d + (4 * x) + 6;
		}

		draw_circle(m_pos[0], m_pos[1], x, y);
	}

}

void Ball::draw_circle(int xc, int yc, int x, int y) {
    put_pixel(xc+x, yc+y);
    put_pixel(xc-x, yc+y);
    put_pixel(xc+x, yc-y);
    put_pixel(xc-x, yc-y);

    put_pixel(xc+y, yc+x);
    put_pixel(xc-y, yc+x);
    put_pixel(xc+y, yc-x);
    put_pixel(xc-y, yc-x);
}

void Ball::put_pixel(const int x, const int y) {
	const int stride = m_width * 4;

	uint32_t *p = static_cast<uint32_t *>(m_canvas + (stride * y));
	p[x] = m_color;
}


