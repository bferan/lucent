#pragma once

#include <iostream>

namespace lucent
{

std::string ReadFile(const std::string& path, std::ios::openmode mode = std::ios::in, bool* success = nullptr);

}