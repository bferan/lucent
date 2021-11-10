#pragma once

#include "core/Hash.hpp"

namespace lucent
{

struct DescriptorID
{
    uint32 hash;
    std::string_view name;
};

inline constexpr DescriptorID operator ""_id(const char* name, size_t size)
{
    auto nameView = std::string_view(name, size);
    auto hash = Hash<uint32>(nameView);
    return { hash, nameView };
}

/// A named slot exposed by a Pipeline to bind graphics resources or uniform values
struct Descriptor
{
    uint32 hash: 32;
    uint32 set: 2;
    uint32 binding: 4;
    uint32 offset: 16;
    uint32 size: 16;
};
static_assert(sizeof(Descriptor) == 12);

}