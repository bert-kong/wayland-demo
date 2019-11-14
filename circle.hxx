#include<stdint.h>

class Circle {
public:
    Circle(const int w=400, 
           const int h=300, 
           const int radius=20, 
           const uint32_t color=0xFFFF0000):
       m_width(w), m_height(h), m_radius(radius),
       m_color(color), m_base(nullptr) {

       m_vel[0] = 3;
       m_vel[1] = 2;

       m_pos[0] = 200;
       m_pos[1] = 100;

       /* rectangle, not used */
       m_x = 0, m_y = 0;
    }

    ~Circle() {
    }

    void setVelocity(int vx, int vy) {
        m_vel[0] = vx, m_vel[1] = vy;
    }

    void setPosition(int x, int y) {
        m_pos[0] = x, m_pos[1] = y;
    }

    void setRadius(int radius) {
        m_radius = radius;
    }

    void setColor(uint32_t color) {
        m_color = color;
    }

    void draw(void *buf, const uint32_t color);

private:
    void update(const uint32_t color);
    void draw_circle(const int xc, 
                     const int yc, 
                     int x, 
                     int y);

    void put_pixel(const int x, const int y);


private:
    /* rectangle */
    const int m_width;
    const int m_height;
    int m_x, m_y;

    int m_radius;
    uint32_t m_color;
    uint8_t *m_base;

    /* velocity */
    int m_vel[2];
    int m_pos[2];
};


