#include <glad/gl.h> // Must be first!
#include <SFML/Graphics.hpp>
#include <thread>
#include <atomic>
#include <iostream>
#include <cmath>
#include <algorithm>
#include "glyph.h"

// Text outline and its width, built once on the main thread before rendering starts
sf::VertexArray textOutline;
float textWidth = 0.f;

// Held keys are shared with the rendering thread so it can move smoothly every frame
std::atomic<bool> scrollLeftHeld(false);
std::atomic<bool> scrollRightHeld(false);
std::atomic<bool> zoomInHeld(false);
std::atomic<bool> zoomOutHeld(false);

// Fraction of the 2 unit wide viewport the text fills at startup, leaving a small margin
const float viewportFillSize = 1.9f;

// Scroll speed in viewport units per second, and how much zoom multiplies by per second
const float scrollSpeed = 1.f;
const float zoomRatePerSecond = 2.f;

// Key presses and releases share this so both events update the same held key flags
void setHeldKey(sf::Keyboard::Key key, bool isHeld) {

    if (key == sf::Keyboard::Key::Left) {

        scrollLeftHeld = isHeld;

    }

    if (key == sf::Keyboard::Key::Right) {

        scrollRightHeld = isHeld;

    }

    if (key == sf::Keyboard::Key::Up) {

        zoomInHeld = isHeld;

    }

    if (key == sf::Keyboard::Key::Down) {

        zoomOutHeld = isHeld;

    }
}

void renderingThread(sf::RenderWindow& window, std::atomic<bool>& running, std::atomic<bool>& contextReady)
{
    
    if (!window.setActive(true)) {
        std::cerr << "CRITICAL: Thread failed to claim context!" << std::endl;
        contextReady = true;
        return;
    }
    contextReady = true;

    // Initialize GLAD with SFML's function loader
    if (!gladLoadGL(sf::Context::getFunction)) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return;
    }

    // Hard verification check:
    if (sf::Shader::isAvailable()) {
        std::cout << "SUCCESS: Shaders are fully available on this thread!" << std::endl;
    } else {
        std::cerr << "ERROR: Context is active on thread, but driver reports NO SHADERS." << std::endl;
    }

    
    // match OpenGL's -1 to 1 canonical view with the Y axis pointing up.
	// Passing a negative height (-2.f) flips the Y-axis upside down!
	// Now: Top-Left is (-1.0, 1.0) and Bottom-Right is (1.0, -1.0)
	sf::View view(sf::Vector2f(0.f, 0.f), sf::Vector2f(2.f, -2.f));
	window.setView(view);

    // Background and alpha blending (for transparency)
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Apply scaling to keep the whole string visible in the viewport
    float textHeight = glyphTop - glyphBottom;
    float zoomScale = viewportFillSize / std::max(textWidth, textHeight);

    // The text point shown at the screen centre, starting in the middle so the string is centred
    float viewCenterX = textWidth / 2.f;
    float viewCenterY = (glyphTop + glyphBottom) / 2.f;

    // Frame rate is unlimited, so movement is based on elapsed time to keep speed consistent
    sf::Clock frameClock;

    while(running) {

        float elapsedSeconds = frameClock.restart().asSeconds();

        // Dividing by zoom keeps the on-screen scroll speed the same at any zoom level
        if (scrollLeftHeld) {

            viewCenterX -= scrollSpeed * elapsedSeconds / zoomScale;

        }

        if (scrollRightHeld) {

            viewCenterX += scrollSpeed * elapsedSeconds / zoomScale;

        }

        // Multiplying makes zoom feel even, changing by the same ratio each second at any size
        if (zoomInHeld) {

            zoomScale *= std::pow(zoomRatePerSecond, elapsedSeconds);
            
        }
        if (zoomOutHeld) {

            zoomScale /= std::pow(zoomRatePerSecond, elapsedSeconds);

        }

        // Shift the viewed point to the origin first, then scale, so zooming stays centred on screen
        sf::Transform textTransform;
        textTransform.scale({zoomScale, zoomScale});
        textTransform.translate({-viewCenterX, -viewCenterY});

        glClear(GL_COLOR_BUFFER_BIT);
		window.draw(textOutline, textTransform);
		window.display();
    }

	window.setActive(false);
}


int main(int argc, char* argv[]) {

    // Render the command-line string, or the test sentence when none is given
    std::string text = "The quick brown fox jumps over the lazy dog";
    if (argc > 1) {

        text = argv[1];

    }

	sf::ContextSettings settings;
    settings.majorVersion = 2;
    settings.minorVersion = 1;
    settings.attributeFlags = sf::ContextSettings::Default;
	
	sf::RenderWindow window(
        sf::VideoMode({800, 800}), 
        "SFML + GLAD (OpenGL 2.1)",
        sf::Style::Default, 
        sf::State::Windowed,  
        settings
	);



    // set additional window properties
    window.setKeyRepeatEnabled(false);
    
    // DO NOT LIMIT FRAME RATE! Use elapsed time to control updates.

    // deactivate its OpenGL context
    if( !window.setActive(false) ) {
        std::cerr << "Error setting up window, could not deactivate context" << std::endl;
        return -1;
    }

	textOutline = buildTextOutline(text, FONT_DIRECTORY);
	textWidth = text.size() * characterAdvance;

	std::atomic<bool> running(true);
    std::atomic<bool> contextReady(false);

    // Launch the rendering thread
    std::thread thread(renderingThread, std::ref(window), std::ref(running), std::ref(contextReady));

    // CRUCIAL: Pause the main thread until the rendering thread has successfully claimed the context.
    // This prevents pollEvent() from clashing with setActive(true) during initialization.
    while (!contextReady) {
        std::this_thread::yield(); 
    }

    // the event/logic/whatever loop
    while (window.isOpen()) {
		while ( const std::optional event = window.pollEvent() )
		{
			if ( event->is<sf::Event::Closed>() ) {
				running = false; 
                
                // Wait for the rendering thread to safely deactivate and exit
                if (thread.joinable()) {
                    thread.join();
                }

                window.close();
			}

            // Arrow keys scroll left and right and zoom in and out while held down
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {

                setHeldKey(keyPressed->code, true);

            }

            if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {

                setHeldKey(keyReleased->code, false);
                
			}
	
		}
    }

}