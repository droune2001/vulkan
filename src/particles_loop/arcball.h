#ifndef _VULKAN_ARCBALL_2018_10_03_H_
#define _VULKAN_ARCBALL_2018_10_03_H_

#include <stdint.h> // uint32_t
#include "glm_usage.h"

class ArcBall
{
public:
    ArcBall();
    ArcBall(uint32_t screen_width, uint32_t screen_height);

    // grab the ball at coords x,y in screen space
    void click(uint32_t x, uint32_t y);

    // set the current screen-space point while dragging.
    void drag(uint32_t x, uint32_t y);

    glm::mat4 view_matrix();

private:
    uint32_t _screen_width = 640;
    uint32_t _screen_height = 480;
};

#endif //_VULKAN_ARCBALL_2018_10_03_H_
