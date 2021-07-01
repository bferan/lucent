#pragma once

#include <iostream>
#include <fstream>
#include <string>

namespace lucent
{

inline std::string ReadFile(const std::string& path)
{
    std::ifstream file(path);
    std::string buf;

    file.seekg(0, std::ios::end);
    buf.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    buf.assign((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    return buf;
}

}