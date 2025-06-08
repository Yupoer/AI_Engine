#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <sstream>
#include <string>
#include <iomanip>
#include <cstdint>
#include <algorithm>

struct Vec3 {
    float x, y, z;
};

struct Triangle {
    Vec3 normal;
    Vec3 v1, v2, v3;
};

Vec3 computeCentroid(const std::vector<Vec3>& vertices) {
    Vec3 sum = {0.0f, 0.0f, 0.0f};
    if (vertices.empty()) return sum;

    for (const auto& v : vertices) {
        sum.x += v.x;
        sum.y += v.y;
        sum.z += v.z;
    }
    
    float count = static_cast<float>(vertices.size());
    sum.x /= count;
    sum.y /= count;
    sum.z /= count;
    return sum;
}

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

std::vector<float> convertToVertexArray(const std::vector<Triangle>& tris, Vec3& minCoord, Vec3& maxCoord) {
    std::vector<float> data;
    
    // 初始化最小和最大座標
    if (!tris.empty()) {
        minCoord = maxCoord = tris[0].v1;
    }
    
    for (const auto& tri : tris) {
        const Vec3& n = tri.normal;
        
        // 更新最小和最大座標
        auto updateMinMax = [&](const Vec3& v) {
            minCoord.x = std::min(minCoord.x, v.x);
            minCoord.y = std::min(minCoord.y, v.y);
            minCoord.z = std::min(minCoord.z, v.z);
            maxCoord.x = std::max(maxCoord.x, v.x);
            maxCoord.y = std::max(maxCoord.y, v.y);
            maxCoord.z = std::max(maxCoord.z, v.z);
        };
        
        updateMinMax(tri.v1);
        updateMinMax(tri.v2);
        updateMinMax(tri.v3);

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

void exportToHeader(const std::vector<float>& vertexArray, const std::string& outputPath, const std::string& variableName, const Vec3& minCoord, const Vec3& maxCoord) {
    std::ofstream out(outputPath);
    out << "#pragma once\n";
    out << "const int " << variableName << "Count = " << (vertexArray.size() / 8) << ";\n";
    
    // 計算中心點
    Vec3 center;
    center.x = (minCoord.x + maxCoord.x) * 0.5f;
    center.y = (minCoord.y + maxCoord.y) * 0.5f;
    center.z = (minCoord.z + maxCoord.z) * 0.5f;
    
    // 輸出最小座標
    out << "const float " << variableName << "Min[] = {"
        << std::fixed << std::setprecision(6) 
        << minCoord.x << "f, " << minCoord.y << "f, " << minCoord.z << "f};\n";
    
    // 輸出最大座標
    out << "const float " << variableName << "Max[] = {"
        << std::fixed << std::setprecision(6) 
        << maxCoord.x << "f, " << maxCoord.y << "f, " << maxCoord.z << "f};\n";
    
    // 輸出中心點座標
    out << "const float " << variableName << "Center[] = {"
        << std::fixed << std::setprecision(6) 
        << center.x << "f, " << center.y << "f, " << center.z << "f};\n";
    
    // 輸出質心座標
    Vec3 centerOfMass;
    std::vector<Vec3> vertexPositions;
    for (size_t i = 0; i < vertexArray.size(); i += 8) {
        Vec3 v;
        v.x = vertexArray[i];
        v.y = vertexArray[i + 1];
        v.z = vertexArray[i + 2];
        vertexPositions.push_back(v);
    }
    centerOfMass = computeCentroid(vertexPositions);
    
    out << "const float " << variableName << "CenterMass[] = {"
        << std::fixed << std::setprecision(6) 
        << centerOfMass.x << "f, " << centerOfMass.y << "f, " << centerOfMass.z << "f};\n";
    
    out << "const float " << variableName << "Vertices[] = {\n";
    for (size_t i = 0; i < vertexArray.size(); ++i) {
        out << std::fixed << std::setprecision(1) << vertexArray[i] << "f";
        if (i + 1 < vertexArray.size()) out << ", ";
        if ((i + 1) % 8 == 0) out << "\n";
    }
    out << "};\n";
}

void processSTLFile(const char* filename, std::vector<float>& vertices, Vec3& centerOfMass) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "無法打開文件: " << filename << std::endl;
        return;
    }

    // 跳過 STL 頭部
    file.seekg(80);

    // 讀取三角形數量
    uint32_t numTriangles;
    file.read(reinterpret_cast<char*>(&numTriangles), sizeof(numTriangles));

    std::vector<Vec3> vertexPositions;
    vertices.clear();
    vertices.reserve(numTriangles * 9); // 每個三角形 3 個頂點，每個頂點 3 個浮點數

    // 讀取每個三角形
    for (uint32_t i = 0; i < numTriangles; ++i) {
        // 跳過法線
        file.seekg(12, std::ios::cur);

        // 讀取三個頂點
        for (int j = 0; j < 3; ++j) {
            Vec3 v;
            file.read(reinterpret_cast<char*>(&v.x), sizeof(float));
            file.read(reinterpret_cast<char*>(&v.y), sizeof(float));
            file.read(reinterpret_cast<char*>(&v.z), sizeof(float));

            // 添加到頂點列表
            vertices.push_back(v.x);
            vertices.push_back(v.y);
            vertices.push_back(v.z);

            // 保存頂點位置用於計算質心
            vertexPositions.push_back(v);
        }

        // 跳過屬性字節計數
        file.seekg(2, std::ios::cur);
    }

    // 計算質心
    centerOfMass = computeCentroid(vertexPositions);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <model.stl> [output.h]\n";
        return 1;
    }

    std::string path = argv[1];
    std::string output;
    
    // 從檔案名稱中提取不含副檔名的部分（不使用 std::filesystem）
    std::string baseName;
    size_t lastSlash = path.find_last_of("/\\");
    size_t lastDot = path.find_last_of(".");
    if (lastSlash != std::string::npos) {
        baseName = path.substr(lastSlash + 1, lastDot - lastSlash - 1);
    } else {
        baseName = path.substr(0, lastDot);
    }
    
    if (argc >= 3) {
        output = argv[2];
    } else {
        output = baseName + ".h";
    }
    
    std::vector<Triangle> triangles;

    if (isBinarySTL(path)) {
        triangles = parseBinarySTL(path);
    } else {
        triangles = parseASCIISTL(path);
    }

    Vec3 minCoord, maxCoord;
    std::vector<float> vertexArray = convertToVertexArray(triangles, minCoord, maxCoord);
    exportToHeader(vertexArray, output, baseName, minCoord, maxCoord);

    std::cout << "Header file generated: " << output << "\n";
    return 0;
}