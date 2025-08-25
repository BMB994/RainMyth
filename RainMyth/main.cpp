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
const float GRAVITY = 9.81f * 100; // 9.81 m/s^2 assumes 1 pixel is a meter. We want it to be 100 pixels is a meter
const float RAINDROP_MIN_SIZE = 0.5f;
const float RAINDROP_MAX_SIZE = 1.5f;
const float RAINDROP_SPAWN_RATE = 175.0f; // Raindrops rate. Do not run this too high
const float WALK_SPEED = 50.0f; // Pixels per second
const float RUN_SPEED = 200.0f; // Pixels per second
const float PERSON_WIDTH = 40.0f;
const float PERSON_HEIGHT = 100.0f;
const float MAX_WETNESS = 1000.0f; // A threshold for the maximum visual wetness

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
        std::uniform_real_distribution<> yDist(-100.0f, -50.0f);

        // Randomly set the size and initial x-position
        float size = static_cast<float>(sizeDist(gen));
        float x = static_cast<float>(xDist(gen));
        float y = static_cast<float>(yDist(gen));

        // Initialize the shape (thin rectangle for a droplet)
        shape.setSize(sf::Vector2f(size, size * 2.0f)); // Make drops longer than they are wide
        shape.setFillColor(sf::Color(173, 216, 230, 200)); // Light blue with transparency
        shape.setPosition(x, y); // Start just above the top of the window

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

	// Checks if the raindrop has hit a platform. This can definitely be improved: a rain drop should be gone if it's past the platform and in the y range of the platform
    bool isOnPlatty(sf::Vector2u windowSize) const {
		// Hardcoded platform position and size for simplicity. I'd like to make them constant but it depends on the window size
        if (shape.getPosition().x > windowSize.x / 8.0f - 100.0f && shape.getPosition().x < windowSize.x / 8.0f + 100.0f) {
            if(shape.getPosition().y > windowSize.y - 250.0f - 25.0f && shape.getPosition().y < windowSize.y - 250.0f + 25.0f) {
                return true;
			}
            else {
                return false;
            }
        }
        else if (shape.getPosition().x > windowSize.x * 7.0f / 8.0f - 100.0f && shape.getPosition().x < windowSize.x * 7.0f / 8.0f + 100.0f) {
            if (shape.getPosition().y > windowSize.y - 250.0f - 25.0f && shape.getPosition().y < windowSize.y - 250.0f + 25.0f) {
                return true;
            }
            else {
                return false;
            }
        }
        else {
            return false;
        }
        
    }

    // Returns the area of the raindrop for wetness calculation
    float getArea() const {
        return shape.getSize().x * shape.getSize().y;
    }

    // Returns the global bounding box for collision detection
    sf::FloatRect getBounds() const {
        return shape.getGlobalBounds();
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

        // Remove raindrops that are on a platty
        drops.erase(std::remove_if(drops.begin(), drops.end(),
            [this](const Raindrop& drop) {
                return drop.isOnPlatty(this->windowSize);
            }),
            drops.end());

        for (int i = 0; i < RAINDROP_SPAWN_RATE; ++i) {
                drops.emplace_back(windowSize);
        }

    }

    // Draws all raindrops
    void draw(sf::RenderWindow& window) {
        for (auto& drop : drops) {
            drop.draw(window);
        }
    }

    // Returns a reference to the drops vector for collision checks
    const std::vector<Raindrop>& getDrops() const {
        return drops;
    }
private:
    std::vector<Raindrop> drops;
    sf::Vector2u windowSize;
};

// A class to represent the person in the simulation
class Person {
public:
    Person(sf::Vector2f position)
        : totalWetness(0.0f), isMoving(false), currentSpeed(0.0f) {
        shape.setSize(sf::Vector2f(PERSON_WIDTH, PERSON_HEIGHT));
        shape.setOrigin(PERSON_WIDTH / 2.0f, PERSON_HEIGHT / 2.0f);
        shape.setPosition(position);
        shape.setFillColor(sf::Color(139, 69, 19)); // Brown
    }

    // Starts movement towards a target position
    void startMove(sf::Vector2f target, float speed) {
        if (!isMoving) {
            targetPosition = target;
            currentSpeed = speed;
            isMoving = true;
        }
    }

    // Resets the person's wetness and position
    void reset(sf::Vector2f position) {
        totalWetness = 0.0f;
        isMoving = false;
        shape.setPosition(position);
        updateColor();
    }

    void update(float deltaTime) {
        if (isMoving) {
            // Calculate the direction vector
            sf::Vector2f direction = targetPosition - shape.getPosition();

            // Check if we are close to the target to stop
            float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);
            if (distance < 5.0f) { // Arbitrary small threshold
                isMoving = false;
                shape.setPosition(targetPosition);
            }
            else {
                // Normalize the direction vector and move
                direction /= distance;
                shape.move(direction * currentSpeed * deltaTime);
            }
        }
    }

    void draw(sf::RenderWindow& window) {
        updateColor();
        window.draw(shape);
    }

    // Accumulates wetness from a raindrop
    void addWetness(float area) {
        totalWetness += area;
    }

    // Returns the person's bounding box for collision detection
    sf::FloatRect getBounds() const {
        return shape.getGlobalBounds();
    }

    // Returns the total accumulated wetness
    float getWetness() const {
        return totalWetness;
    }

private:
    sf::RectangleShape shape;
    sf::Vector2f targetPosition;
    float currentSpeed;
    float totalWetness;
    bool isMoving;

    // Updates the color based on the current wetness
    void updateColor() {
        // Linearly interpolate between brown and light blue based on wetness
        float normalizedWetness = std::min(totalWetness, MAX_WETNESS) / MAX_WETNESS;

        sf::Uint8 red = static_cast<sf::Uint8>(139 + normalizedWetness * (173 - 139));
        sf::Uint8 green = static_cast<sf::Uint8>(69 + normalizedWetness * (216 - 69));
        sf::Uint8 blue = static_cast<sf::Uint8>(19 + normalizedWetness * (230 - 19));

        shape.setFillColor(sf::Color(red, green, blue));
    }
};


int main()
{
    sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(desktopMode, "Rain Simulation", sf::Style::Fullscreen);
    window.setFramerateLimit(60);

    bool isFullScreen = true;
    const int defaultWindowWidth = 1280;
    const int defaultWindowHeight = 720;

    sf::Vector2u windowSize = window.getSize();

    // Create the rain system
    RainSystem rainSystem(windowSize);

    // Create the platforms
    sf::RectangleShape startPlatform;
    startPlatform.setSize(sf::Vector2f(200, 50));
    startPlatform.setOrigin(100, 25);
    startPlatform.setPosition(windowSize.x / 8.0f, windowSize.y - 250.0f);
    startPlatform.setFillColor(sf::Color(100, 100, 100));

    sf::RectangleShape endPlatform;
    endPlatform.setSize(sf::Vector2f(200, 50));
    endPlatform.setOrigin(100, 25);
    endPlatform.setPosition(windowSize.x * 7.0f / 8.0f, windowSize.y - 250.0f);
    endPlatform.setFillColor(sf::Color(100, 100, 100));

    // Create the person
    Person person(startPlatform.getPosition() - sf::Vector2f(0, -150));

    // Set up text for displaying wetness
    sf::Font font;
    if (!font.loadFromFile("Assets\\Font\\Roboto-Regular.ttf")) {
        
        return EXIT_FAILURE;
    }
    sf::Text wetnessText;
    wetnessText.setFont(font);
    wetnessText.setCharacterSize(24);
    wetnessText.setFillColor(sf::Color::White);
    wetnessText.setPosition(10.0f, 10.0f);

    sf::Clock clock;

    while (window.isOpen())
    {
        float deltaTime = clock.restart().asSeconds();

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                // Toggle fullscreen
                if (event.key.code == sf::Keyboard::Escape)
                {
                    window.close();

                }

                // Start the simulation with 'W' for walk or 'R' for run
                if (event.key.code == sf::Keyboard::W) {
                    person.reset(startPlatform.getPosition() + sf::Vector2f(0, -150));
                    person.startMove(endPlatform.getPosition() + sf::Vector2f(0, -150), WALK_SPEED);
                }
                if (event.key.code == sf::Keyboard::R) {
                    person.reset(startPlatform.getPosition() + sf::Vector2f(0, -150));
                    person.startMove(endPlatform.getPosition() + sf::Vector2f(0, -150), RUN_SPEED);
                }
            }
        }

        // --- Simulation Logic ---
        rainSystem.update(deltaTime);
        person.update(deltaTime);

        // Check for collisions between raindrops and the person
        sf::FloatRect personBounds = person.getBounds();
        for (const auto& drop : rainSystem.getDrops()) {
            if (personBounds.intersects(drop.getBounds())) {
                person.addWetness(drop.getArea());
            }
        }

        // Check if the person is under a platform and should not get wet
        if (startPlatform.getGlobalBounds().intersects(personBounds) || endPlatform.getGlobalBounds().intersects(personBounds)) {

        }

        // Update the wetness text
        //std::stringstream ss;
        //ss << "Total Wetness: " << std::fixed << std::setprecision(2) << person.getWetness();
        //wetnessText.setString(ss.str());

        // Clear the window
        window.clear(sf::Color::Black);

        // --- Rendering Logic ---
        window.draw(startPlatform);
        window.draw(endPlatform);
        rainSystem.draw(window);
        person.draw(window);
        window.draw(wetnessText);

        window.display();
    }

    return 0;
}
