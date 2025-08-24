#include <iostream>
#include <vector>
#include <memory>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <algorithm>
#include <cmath>
#include <random>

// Constants for the simulation
const float GRAVITY = 9.81f * 10.0f; // Exaggerated gravity for visibility
const float RAINDROP_MIN_SIZE = 2.0f;
const float RAINDROP_MAX_SIZE = 5.0f;
const int MAX_RAINDROPS = 500; // The maximum number of drops to simulate

// A class to represent a single raindrop
class Raindrop {
public:
    // Constructor initializes a raindrop with a random size and position at the top
    Raindrop(sf::Vector2u windowSize) {
        // Use a random device and engine to generate random numbers
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> xDist(0.0f, windowSize.x);
        std::uniform_real_distribution<> sizeDist(RAINDROP_MIN_SIZE, RAINDROP_MAX_SIZE);

        // Randomly set the size and initial x-position
        float size = static_cast<float>(sizeDist(gen));
        float x = static_cast<float>(xDist(gen));

        // Initialize the shape (thin rectangle for a droplet)
        shape.setSize(sf::Vector2f(size, size * 2.0f)); // Make drops longer than they are wide
        shape.setFillColor(sf::Color(173, 216, 230, 200)); // Light blue with transparency
        shape.setPosition(x, -shape.getSize().y - 100); // Start just above the top of the window

        // Initialize the velocity
        velocity = sf::Vector2f(0.0f, 0.0f);
    }

    // Updates the raindrop's position based on gravity and a time step
    void update(float deltaTime) {
        // Apply gravity to the velocity
        velocity.y += GRAVITY * deltaTime;

        // Update the position
        shape.move(velocity * deltaTime);
    }

    // Draws the raindrop on the window
    void draw(sf::RenderWindow& window) {
        window.draw(shape);
    }

    // Checks if the raindrop has fallen off the bottom of the screen
    bool isOffScreen(sf::Vector2u windowSize) const {
        return shape.getPosition().y > windowSize.y;
    }

private:
    sf::RectangleShape shape;
    sf::Vector2f velocity;
};

// A class to manage the entire rain system
class RainSystem {
public:
    RainSystem(sf::Vector2u windowSize) : windowSize(windowSize) {}

    // Updates all raindrops and respawns those that are off-screen
    void update(float deltaTime) {
        for (auto& drop : drops) {
            drop.update(deltaTime);
        }

        // Remove raindrops that are off-screen. This is a wild line of code
        drops.erase(std::remove_if(drops.begin(), drops.end(),
            [this](const Raindrop& drop) {
                return drop.isOffScreen(this->windowSize);
            }),
            drops.end());

        // Continuously spawn new raindrops up to the maximum limit
        //if (drops.size() < MAX_RAINDROPS) {
            for (int i = 0; i < 5; ++i) { // Spawn a few drops at a time for a steady flow
                drops.emplace_back(windowSize);
            }
        //}
    }

    // Draws all raindrops
    void draw(sf::RenderWindow& window) {
        for (auto& drop : drops) {
            drop.draw(window);
        }
    }

private:
    std::vector<Raindrop> drops;
    sf::Vector2u windowSize;
};

int main()
{
    // Use getDesktopMode to automatically get the monitor's resolution
    sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
    // Create a full-screen window initially.
    sf::RenderWindow window(desktopMode, "Rain Simulation", sf::Style::Fullscreen);
    window.setFramerateLimit(60);

    // Keep track of the window state
    bool isFullScreen = true;

    // Default window dimensions for non-fullscreen mode
    const int defaultWindowWidth = 1280;
    const int defaultWindowHeight = 720;

    // Get the window size for positioning other elements
    sf::Vector2u windowSize = window.getSize();

    // Create an instance of our rain system
    RainSystem rainSystem(windowSize);

    // Use a clock to measure the time between frames for consistent physics
    sf::Clock clock;

    while (window.isOpen())
    {
        float deltaTime = clock.restart().asSeconds();

        sf::Event event;

        // Get inputs
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            // Toggle fullscreen with the F11 key
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F11)
            {
                if (isFullScreen) {
                    window.create(sf::VideoMode(defaultWindowWidth, defaultWindowHeight), "Rain Simulation", sf::Style::Default);
                    isFullScreen = false;
                }
                else {
                    window.create(desktopMode, "Rain Simulation", sf::Style::Fullscreen);
                    isFullScreen = true;
                }
                window.setFramerateLimit(60);
                windowSize = window.getSize();
                rainSystem = RainSystem(windowSize); // Re-initialize rain system for new window size
            }
        }

        // --- Simulation Logic ---
        rainSystem.update(deltaTime);

        // Clear window data
        window.clear();

        // --- Rendering Logic ---
        rainSystem.draw(window);

        // Update window
        window.display();
    }

    return 0;
}
