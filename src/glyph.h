#pragma once
#include <SFML/Graphics.hpp>
#include <string>

// The space used per character
const float characterAdvance = 0.525f;

// Lowest and highest points reached by the character
const float glyphBottom = -0.25f;
const float glyphTop = 0.7f;

// Builds line segments outlining every character of the text, laid out left to right from x = 0
sf::VertexArray buildTextOutline(const std::string& text, const std::string& fontDirectory);
