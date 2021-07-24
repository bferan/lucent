#pragma once

#include <iostream>
#include <fstream>
#include <string>

namespace lucent
{

inline std::string ReadFile(const std::string& path, std::ios::openmode mode = std::ios::in)
{
    std::ifstream file(path, mode);
    std::string buf;

    file.seekg(0, std::ios::end);
    buf.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    buf.assign((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    return buf;
}

}