#pragma once

#include "rendering/StaticMesh.hpp"

namespace lucent
{

class Material;

//! A collection of meshes with corresponding materials
class Model
{
public:
    Model() = default;

    explicit Model(StaticMesh mesh, Material* material = nullptr);

    void AddMesh(StaticMesh mesh, Material* material = nullptr);

public:
    // STL interface
    auto begin() const { return m_Primitives.cbegin();}
    auto end() const { return m_Primitives.cend(); };

public:
    struct Primitive
    {
        StaticMesh mesh;
        Material* material;
    };

private:
    std::vector<Primitive> m_Primitives;

};

}
