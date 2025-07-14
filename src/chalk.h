#pragma once
#include <string>

namespace chalk
{
// Modernized: trailing return types, descriptive parameter names

auto magenta(const std::string& text) -> std::string;
auto bold(const std::string& text) -> std::string;
auto yellow(const std::string& text) -> std::string;
auto green(const std::string& text) -> std::string;
auto blue(const std::string& text) -> std::string;
} // namespace chalk
