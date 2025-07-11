#include "chalk.h"

namespace chalk {
    std::string magenta(const std::string& text) {
        return "\033[35m" + text + "\033[0m";
    }
    std::string bold(const std::string& text) {
        return "\033[1m" + text + "\033[0m";
    }
    std::string yellow(const std::string& text) {
        return "\033[33m" + text + "\033[0m";
    }
    std::string green(const std::string& text) {
        return "\033[32m" + text + "\033[0m";
    }
    std::string blue(const std::string& text) {
        return "\033[34m" + text + "\033[0m";
    }
}
