#include "Shader.hpp"

namespace lucent
{

// Info log
void ShaderInfoLog::Error(const std::string& text)
{
    error += text;
    error += '\n';
}

}
