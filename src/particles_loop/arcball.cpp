#include "arcball.h"

ArcBall::ArcBall()
{
}

ArcBall::ArcBall(uint32_t screen_width, uint32_t screen_height)
    :   _screen_width(screen_width),
        _screen_height(screen_height)
{

}

void ArcBall::click(uint32_t x, uint32_t y)
{

}

// set the current screen-space point while dragging.
void ArcBall::drag(uint32_t x, uint32_t y)
{

}

glm::mat4 ArcBall::view_matrix()
{
    return glm::mat4(1.0f);
}

//
