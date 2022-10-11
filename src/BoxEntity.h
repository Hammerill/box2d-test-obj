#pragma once

#include "PhysicsObj.h"

/// Child class of the PhysicsObj which represents either box or 
/// rectangle in the WorldManager environment.
class BoxEntity: public PhysicsObj
{
private:
    /// Default values of position and angle of box entity.
    /// Used while creating a box or resetting it to default.
    float x, y, w, h, angle, vel_x, vel_y;

    /// @brief Shape of the box entity.
    b2PolygonShape boxShape;

public:
    /// @brief Create a new box entity.
    /// @param path_to_texture path to the image file of texture.
    /// @param x_box position X of upper left corner of the box in Box2D meters.
    /// @param y_box position Y of upper left corner of the box in Box2D meters.
    /// @param w_box width of the box in Box2D meters.
    /// @param h_box height of the box in Box2D meters.
    /// @param angle_box angle of the box in radians.
    /// @param vel_x velocity on X of the box.
    /// @param vel_y velocity on Y of the box.
    BoxEntity(const char* path_to_texture, float x_box, float y_box, float w_box, float h_box, float angle_box, float vel_x = 0, float vel_y = 0);
    
    /// @brief Create a new box entity.
    /// @param texture link to the texture. Can be used in order to improve perfomance with many boxes with same texture.
    /// @param x_box position X of upper left corner of the box in Box2D meters.
    /// @param y_box position Y of upper left corner of the box in Box2D meters.
    /// @param w_box width of the box in Box2D meters.
    /// @param h_box height of the box in Box2D meters.
    /// @param angle_box angle of the box in radians.
    /// @param vel_x velocity on X of the box.
    /// @param vel_y velocity on Y of the box.
    BoxEntity(SDL_Texture* texture, float x_box, float y_box, float w_box, float h_box, float angle_box, float vel_x = 0, float vel_y = 0);
    
    /// Register this box in the world and set its texture. Should be 
    /// called only when no world calculations are performing.
    /// @param world link to the world where box should be registered.
    /// @param renderer the rendering context.
    void Register(b2World* world, SDL_Renderer* renderer);

    /// @brief Set default position and angle of the box.
    void Reset();

    /// @brief Render this box.
    /// @param renderer the rendering context.
    /// @param x_offset camera X offset in pixels.
    /// @param y_offset camera Y offset in pixels.
    /// @param zoom camera zoom coefficient.
    void Render(SDL_Renderer* renderer, float x_offset, float y_offset, float zoom);
};
