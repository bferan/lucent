#pragma once

namespace lucent
{

template<typename T>
struct FNVTraits
{
};

template<>
struct FNVTraits<uint32_t>
{
    static constexpr uint32_t kPrime = 16777619u;
    static constexpr uint32_t kOffset = 2166136261u;
};

template<>
struct FNVTraits<uint64_t>
{
    static constexpr uint64_t kPrime = 1099511628211u;
    static constexpr uint64_t kOffset = 14695981039346656037u;
};

// Currently supports 32 & 64 bit FNV-1a hashing
template<typename T>
constexpr T Hash(std::string_view string, T offset = FNVTraits<T>::kOffset)
{
    for (char c : string)
    {
        offset ^= static_cast<uint8_t>(c);
        offset *= FNVTraits<T>::kPrime;
    }
    return offset;
}

}