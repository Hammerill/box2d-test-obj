#pragma once

/// @brief Class that manage everything.
class GameManager
{
private:

public:
    /// @brief Init video and enter main menu.
    /// @param path_to_font path to the font (*.ttf) file. If leave nullptr, Sand-Box2D wouldn't render any text.
    /// @param path_to_icon path to the icon that Sand-Box2D will set to program instance.
    /// Leave nullptr if you're setting icon in CMake.
    /// @param path_to_settings path to the JSON where user settings are stored.
    /// If doesn't exist - it'll create it. If leave nullptr, settings affect only current session.
    GameManager(const char* path_to_font = nullptr, const char* path_to_icon = nullptr, const char* path_to_settings = nullptr);
    ~GameManager();

    /// @brief Perform one step and call step functions on all the objects. To render them call Render().
    void Step();

    /// @brief Call render functions on all the objects and then show them on the screen.
    void Render();

    /// @brief Launch main game cycle. Exit from it when user want it.
    void Cycle();
};
