#include "Window.h"

Window Window::m_instance;

Window* Window::get()
{
    return &m_instance;
}

void Window::init(int width, int height, const std::string& title)
{
    m_sfmlWindow = new sf::RenderWindow(sf::VideoMode(width, height), title);
}

void Window::update()
{
    sf::Event event;

    while (m_sfmlWindow->pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
        {
            m_sfmlWindow->close();
        }
    }

    m_sfmlWindow->display();
    m_sfmlWindow->clear();
}

bool Window::isOpen()
{
    return m_sfmlWindow->isOpen();
}