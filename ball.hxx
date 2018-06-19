/*
 * ball.hxx
 *
 *  Created on: Jun 18, 2018
 *      Author: bert
 */

#ifndef BALL_HXX_
#define BALL_HXX_

class Ball {
public:
	Ball(const int radius, const uint32_t color, const int rect[2]) :
		m_radius(radius), m_color(color), m_width(rect[0]), m_height(rect[1]),
		m_pos({0, 0}), m_vel({2, 3})
	{
	}

	~Ball() {
	}

	void draw(void *buf);

	/* public interface */
	void setPosition(int x, int y) {
		(m_pos[0] = x, m_pos[1] = y);
	}
	void setVelocity(int vx, int vy) {
		(m_vel[0] = vx, m_vel[1] = vy);
	}


private:
	void update();
	void put_pixel(const int x, const int y);
	void draw_circle(const int xc, const int yc, int x, int y);

private:
	int m_radius;
	int m_pos[2];
	int m_vel[2];
	uint32_t m_color;

	/* canvas for drawing a ball */
	uint8_t *m_canvas;
	int m_width, m_height;
};



#endif /* BALL_HXX_ */
