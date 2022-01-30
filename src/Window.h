#pragma once

#include <string>
#include <SFML/Graphics.hpp>

class Window
{
public:
    static Window* get();
    void init(int width, int height, const std::string& title);
    void update();
    bool isOpen();
    sf::RenderWindow* getSFMLWindow();

private:
    static Window m_instance;
    sf::RenderWindow* m_sfmlWindow;
};