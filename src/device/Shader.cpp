#include "Shader.hpp"

namespace lucent
{

// Info log
void ShaderInfoLog::Error(const std::string& text)
{
    error += text;
    error += '\n';
}

Descriptor* Shader::Lookup(DescriptorID id)
{
    auto it = std::lower_bound(descriptors.begin(), descriptors.end(), id,
        [](auto& desc, auto& name)
        {
            return desc.hash < name.hash;
        });

    if (it == descriptors.end() || id.hash != it->hash)
    {
        LC_ERROR("ERROR: Failed to find descriptor with ID \"{}\"", id.name);
        return nullptr;
    }
    return it;
}

}
