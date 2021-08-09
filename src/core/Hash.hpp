#pragma once

namespace lucent
{

template<typename T>
struct FNVTraits
{
};

template<>
struct FNVTraits<uint32>
{
    static constexpr uint32 kPrime = 16777619u;
    static constexpr uint32 kOffset = 2166136261u;
};

template<>
struct FNVTraits<uint64>
{
    static constexpr uint64 kPrime = 1099511628211u;
    static constexpr uint64 kOffset = 14695981039346656037u;
};

// Currently supports 32 & 64 bit FNV-1a hashing
template<typename T>
constexpr T Hash(std::string_view string, T offset = FNVTraits<T>::kOffset)
{
    for (char c : string)
    {
        offset ^= static_cast<uint8>(c);
        offset *= FNVTraits<T>::kPrime;
    }
    return offset;
}

template<typename T, typename V>
T HashBytes(const V& value, T offset = FNVTraits<T>::kOffset)
{
    auto view = std::string_view((const char*)&value, sizeof(V));
    return Hash(view, offset);
}

}