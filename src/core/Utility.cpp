#include "core/Utility.hpp"

#include <iostream>
#include <fstream>

namespace lucent
{

std::string ReadFile(const std::string& path, std::ios::openmode mode, bool* success)
{
    std::ifstream file(path, mode);
    std::string buf;

    if (!file.is_open())
    {
        if (success) *success = false;
        return {};
    }

    file.seekg(0, std::ios::end);
    buf.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    buf.assign((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    if (success) *success = true;
    return buf;
}

}
