#pragma once
#include <juce_core/juce_core.h>

#include <string>

// Texto UTF-8 -> juce::String
inline juce::String TR(const char* s) { return juce::String::fromUTF8(s); }
inline juce::String U8(const std::string& s) { return juce::String::fromUTF8(s.c_str(), (int) s.size()); }
