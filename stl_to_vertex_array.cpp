// File: stl_to_vertex_array.cpp

#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <sstream>
#include <string>
#include <iomanip>
#include <cstdint>
#include <filesystem>

struct Vec3 {
    float x, y, z;
};

struct Triangle {
    Vec3 normal;
    Vec3 v1, v2, v3;
};

bool isBinarySTL(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(80, std::ios::beg);
    uint32_t numTriangles = 0;
    file.read(reinterpret_cast<char*>(&numTriangles), 4);
    return size == 84 + numTriangles * 50;
}

std::vector<Triangle> parseBinarySTL(const std::string& filename) {
    std::vector<Triangle> triangles;
    std::ifstream file(filename, std::ios::binary);
    if (!file) return triangles;

    file.seekg(80); // skip header
    uint32_t numTriangles;
    file.read(reinterpret_cast<char*>(&numTriangles), 4);

    for (uint32_t i = 0; i < numTriangles; ++i) {
        Triangle tri;
        file.read(reinterpret_cast<char*>(&tri.normal), sizeof(Vec3));
        file.read(reinterpret_cast<char*>(&tri.v1), sizeof(Vec3));
        file.read(reinterpret_cast<char*>(&tri.v2), sizeof(Vec3));
        file.read(reinterpret_cast<char*>(&tri.v3), sizeof(Vec3));
        file.ignore(2); // skip attribute byte count
        triangles.push_back(tri);
    }

    return triangles;
}

std::vector<Triangle> parseASCIISTL(const std::string& filename) {
    std::vector<Triangle> triangles;
    std::ifstream file(filename);
    if (!file) return triangles;

    std::string line;
    Triangle tri;
    int vertexCount = 0;

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string word;
        ss >> word;

        if (word == "facet") {
            ss >> word; // normal
            ss >> tri.normal.x >> tri.normal.y >> tri.normal.z;
        } else if (word == "vertex") {
            Vec3 v;
            ss >> v.x >> v.y >> v.z;
            if (vertexCount == 0) tri.v1 = v;
            else if (vertexCount == 1) tri.v2 = v;
            else if (vertexCount == 2) tri.v3 = v;
            vertexCount++;
        } else if (word == "endfacet") {
            triangles.push_back(tri);
            vertexCount = 0;
        }
    }

    return triangles;
}

std::vector<float> convertToVertexArray(const std::vector<Triangle>& tris) {
    std::vector<float> data;
    for (const auto& tri : tris) {
        const Vec3& n = tri.normal;

        auto pushVertex = [&](const Vec3& v, float u, float v_) {
            data.push_back(v.x);
            data.push_back(v.y);
            data.push_back(v.z);
            data.push_back(u);
            data.push_back(v_);
            data.push_back(n.x);
            data.push_back(n.y);
            data.push_back(n.z);
        };

        pushVertex(tri.v1, 0.0f, 0.0f);
        pushVertex(tri.v2, 1.0f, 0.0f);
        pushVertex(tri.v3, 0.5f, 1.0f);
    }
    return data;
}

void exportToHeader(const std::vector<float>& vertexArray, const std::string& outputPath) {
    std::ofstream out(outputPath);
    out << "#pragma once\n";
    out << "const int vertexCount = " << (vertexArray.size() / 8) << ";\n";
    out << "const float vertices[] = {\n";
    for (size_t i = 0; i < vertexArray.size(); ++i) {
        out << std::fixed << std::setprecision(1) << vertexArray[i] << "f";
        if (i + 1 < vertexArray.size()) out << ", ";
        if ((i + 1) % 8 == 0) out << "\n";
    }
    out << "};\n";
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <model.stl> <output.h>\n";
        return 1;
    }

    std::string path = argv[1];
    std::string output = argv[2];
    std::vector<Triangle> triangles;

    if (isBinarySTL(path)) {
        triangles = parseBinarySTL(path);
    } else {
        triangles = parseASCIISTL(path);
    }

    std::vector<float> vertexArray = convertToVertexArray(triangles);
    exportToHeader(vertexArray, output);

    std::cout << "Header file generated: " << output << "\n";
    return 0;
}