#include "glyph.h"
#include <fstream>
#include <sstream>
#include <iostream>

// Number of straight pieces used to approximate each curve
const int curveSegmentCount = 20;

// OpenGL cannot draw curves, so we evaluate points along the cubic Bezier and join them with lines
sf::Vector2f evaluateCubicBezier(sf::Vector2f startPoint, sf::Vector2f firstControlPoint, sf::Vector2f secondControlPoint, sf::Vector2f endPoint, float curveParameter) {
    
    float remainingParameter = 1.f - curveParameter;
    float startWeight = remainingParameter * remainingParameter * remainingParameter;
    float firstControlWeight = 3.f * remainingParameter * remainingParameter * curveParameter;
    float secondControlWeight = 3.f * remainingParameter * curveParameter * curveParameter;
    float endWeight = curveParameter * curveParameter * curveParameter;
    return startWeight * startPoint + firstControlWeight * firstControlPoint + secondControlWeight * secondControlPoint + endWeight * endPoint;
    
}

// Each line segment needs both endpoints because the vertex array draws independent lines
void appendLineSegment(sf::VertexArray& outline, sf::Vector2f fromPoint, sf::Vector2f toPoint) {

    outline.append(sf::Vertex{fromPoint, sf::Color::White});
    outline.append(sf::Vertex{toPoint, sf::Color::White});

}

// Reads the next x and y pair, shifted so the glyph sits at its own place in the text
sf::Vector2f readPoint(std::istringstream& lineStream, sf::Vector2f glyphOffset) {

    sf::Vector2f point;
    lineStream >> point.x >> point.y;
    return point + glyphOffset;

}

// Follows the glyph file's instructions and adds the resulting lines to the outline
void appendGlyphOutline(sf::VertexArray& outline, const std::string& glyphPath, float horizontalOffset) {

    sf::Vector2f glyphOffset(horizontalOffset, 0.f);
    sf::Vector2f currentPoint;
    sf::Vector2f pathStartPoint;

    // Report a missing file so a wrong font path does not silently draw nothing
    std::ifstream glyphFile(glyphPath);
    if (!glyphFile.is_open()) {

        std::cerr << "Missing glyph file: " << glyphPath << std::endl;
        return;

    }

    std::string line;
    while (std::getline(glyphFile, line)) {

        std::istringstream lineStream(line);
        char segmentType = ' ';
        lineStream >> segmentType;

        // Move starts a new path from the specified point
        if (segmentType == 'M') {

            currentPoint = readPoint(lineStream, glyphOffset);
            pathStartPoint = currentPoint;

        }

        // Line draws straight to the next point
        if (segmentType == 'L') {

            sf::Vector2f nextPoint = readPoint(lineStream, glyphOffset);
            appendLineSegment(outline, currentPoint, nextPoint);
            currentPoint = nextPoint;

        }

        // Curve is approximated by many short lines between sampled points on the Bezier curve
        if (segmentType == 'C') {

            sf::Vector2f firstControlPoint = readPoint(lineStream, glyphOffset);
            sf::Vector2f secondControlPoint = readPoint(lineStream, glyphOffset);
            sf::Vector2f endPoint = readPoint(lineStream, glyphOffset);
            sf::Vector2f previousSample = currentPoint;

            for (int segmentIndex = 1; segmentIndex <= curveSegmentCount; segmentIndex++) {

                float curveParameter = static_cast<float>(segmentIndex) / curveSegmentCount;
                sf::Vector2f sample = evaluateCubicBezier(currentPoint, firstControlPoint, secondControlPoint, endPoint, curveParameter);
                appendLineSegment(outline, previousSample, sample);
                previousSample = sample;

            }
            currentPoint = endPoint;
        }

        // Close connects back to where the path began so the shape forms a loop
        if (segmentType == 'Z') {

            appendLineSegment(outline, currentPoint, pathStartPoint);
            currentPoint = pathStartPoint;

        }
    }
}

sf::VertexArray buildTextOutline(const std::string& text, const std::string& fontDirectory) {

    sf::VertexArray outline(sf::PrimitiveType::Lines);
    float penPosition = 0.f;

    for (char character : text) {

        // Spaces have no glyph file, so they only move the pen to leave a gap between words
        if (character != ' ') {

            // Glyph files are named by the character's decimal code, for example 'Q' is gly_81
            int characterCode = static_cast<unsigned char>(character);
            std::string glyphPath = fontDirectory + "gly_" + std::to_string(characterCode);
            appendGlyphOutline(outline, glyphPath, penPosition);

        }

        // Every typewriter character has the same width, so the pen always moves the same distance
        penPosition += characterAdvance;

    }
    return outline;
}
