#include "chalk.h"

namespace chalk
{
// Modernized: trailing return types, descriptive variable names

auto magenta(const std::string& text) -> std::string { return "\033[35m" + text + "\033[0m"; }
auto bold(const std::string& text) -> std::string { return "\033[1m" + text + "\033[0m"; }
auto yellow(const std::string& text) -> std::string { return "\033[33m" + text + "\033[0m"; }
auto green(const std::string& text) -> std::string { return "\033[32m" + text + "\033[0m"; }
auto blue(const std::string& text) -> std::string { return "\033[34m" + text + "\033[0m"; }
} // namespace chalk
