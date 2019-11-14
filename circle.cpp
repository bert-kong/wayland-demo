#include<stdint.h>
#include "circle.hxx"

void Circle::put_pixel(const int x, const int y) {
    const int stride = m_width * 4;
    uint32_t *p = reinterpret_cast<uint32_t *>(m_base + (stride * y));
    p[x] = m_color;
}

/**
 * function to draw all other 7 pixels
 * present at symmetric position
 */

// C-program for circle drawing
// using Bresenhams Algorithm
// in computer-graphics
 
// Function to put pixels
// at subsequence points
void Circle::draw_circle(const int xc, const int yc, int x, int y) {
    put_pixel(xc+x, yc+y);
    put_pixel(xc-x, yc+y);
    put_pixel(xc+x, yc-y);
    put_pixel(xc-x, yc-y);
    put_pixel(xc+y, yc+x);
    put_pixel(xc-y, yc+x);
    put_pixel(xc+y, yc-x);
    put_pixel(xc-y, yc-x);
}
 
// Function for circle-generation
// using Bresenham's algorithm

/**
 *  circle(x, y, r, color, buf)
 *     center (x, y)
 *     radius r
 *     colro and buf
 */
void Circle::draw(void *buf, const uint32_t color) {
    int x = 0, y = m_radius;
    int d = 3 - 2 * m_radius;

    m_base = reinterpret_cast<uint8_t *>(buf);

    update(color);
    while (y >= x) {
        // for each pixel we will
        // draw all eight pixels
        draw_circle(m_pos[0], m_pos[1], x, y);
        x++;
 
        // check for decision parameter
        // and correspondingly 
        // update d, x, y
        if (d > 0)
        {
            y--; 
            d = d + 4 * (x - y) + 10;
        }
        else {
            d = d + 4 * x + 6;
        }

        draw_circle(m_pos[0], m_pos[1], x, y);
    }
}

void Circle::update(const uint32_t color) {
    int x, y;

    x = m_pos[0], y = m_pos[1];

    /* update position */
    x += m_vel[0];
    y += m_vel[1];
    
    if ((m_x + m_width)-m_radius<x || (m_x + m_radius)>x) {
        /* change direction */
        m_vel[0] *=-1;
        m_color = color;
        return;
    }

    if ((m_y + m_height)-m_radius<y || (m_y + m_radius)>y) {
        /* change direction */
        m_vel[1] *=-1;
        m_color = color;
        return;
    }

    m_pos[0] = x, m_pos[1] = y;
}
 
