#include "Mesh.h"

#include <chrono>

#include "IRenderer.h"
#include "Core/Log.h"
#include "Math/Vector3.h"
#include "Math/Vector2.h"

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Vertex.h"

namespace Engine
{
    Mesh::~Mesh()
    {
        if (vertexBuffer)
        {
            renderer->ReleaseBuffer(vertexBuffer);
        }
        if (indexBuffer)
        {
            renderer->ReleaseBuffer(indexBuffer);
        }
    }

    std::shared_ptr<Mesh> Mesh::LoadFromOBJ(IRenderer* renderer, const char* fileName)
    {
        // Test: OBJ 파일 로드 시간 측정.
        auto startTime = std::chrono::high_resolution_clock::now();
        
        std::ifstream file(fileName);
        std::string line;

        if (!file.is_open())
        {
            Log::Error("Failed to open OBJ file: %s", fileName);
            return nullptr;
        }

        // v, vn, vt 데이터를 임시로 저장할 배열.
        std::vector<Vector3> position; // v.
        position.reserve(30000);
        std::vector<Vector3> normal; // vn.
        normal.reserve(30000);
        std::vector<Vector2> uvs; // vt.
        uvs.reserve(30000);

        // 최종 버텍스 버퍼에 들어갈 데이터 배열.
        std::vector<Vertex> vertices;
        vertices.reserve(50000);
        // 최종 인덱스 버퍼에 들어갈 데이터 배열.
        std::vector<UINT> indices;
        indices.reserve(150000);
        // 중복 방지 체크용 Map.
        std::unordered_map<uint64_t, UINT> map;

        std::vector<Vertex> tempVertices;
        std::vector<uint64_t> faces;

        while (std::getline(file, line))
        {
            if (line.size() < 3)
            {
                continue;
            }

            Vector3 v3Temp;
            Vector2 v2Temp;

            if (line[0] == 'v' && line[1] == ' ')
            {
                sscanf_s(
                    line.c_str(),
                    "v %f %f %f",
                    &v3Temp.x,
                    &v3Temp.y,
                    &v3Temp.z
                );
                position.emplace_back(v3Temp);
            }
            else if (line[0] == 'v' && line[1] == 't' && line[2] == ' ')
            {
                sscanf_s(
                    line.c_str(),
                    "vt %f %f",
                    &v2Temp.x,
                    &v2Temp.y
                );
                uvs.emplace_back(v2Temp);
            }
            else if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ')
            {
                sscanf_s(
                    line.c_str(),
                    "vn %f %f %f",
                    &v3Temp.x,
                    &v3Temp.y,
                    &v3Temp.z
                );
                normal.emplace_back(v3Temp);
            }
            else if (line[0] == 'f' && line[1] == ' ')
            {
                tempVertices.clear();
                faces.clear();
                const char* f = line.c_str() + 2;
                
                while (*f != '\0')
                {
                    while (*f == ' ')
                    {
                        f++;
                    }
                    if (*f == '\0')
                    {
                        break;
                    }

                    const char* tokenStart = f;
                    while (*f != ' ' && *f != '\0')
                    {
                        f++;
                    }

                    const char* face = tokenStart;
                    long posIndex = strtol(face, const_cast<char**>(&face), 10);
                    if (*face == '/')
                    {
                        face++;
                    }
                    long uvIndex = strtol(face, const_cast<char**>(&face), 10);
                    if (*face == '/')
                    {
                        face++;
                    }
                    long normalIndex = strtol(face, const_cast<char**>(&face), 10);

                    Vertex vertex;
                    if (posIndex > 0)
                    {
                        vertex.position = position[posIndex - 1];
                    }
                    if (uvIndex > 0)
                    {
                        vertex.uv = uvs[uvIndex - 1];
                    }
                    if (normalIndex > 0)
                    {
                        vertex.normal = normal[normalIndex - 1];
                    }

                    tempVertices.emplace_back(vertex);
                    uint64_t key = static_cast<uint64_t>(posIndex) | (static_cast<uint64_t>(uvIndex) << 21)
                        | (static_cast<uint64_t>(normalIndex) << 42);
                    faces.emplace_back(key);
                }
                if (tempVertices.size() > 3)
                {
                    int triIndex[] = {0, 1, 2, 0, 2, 3};
                    for (int i : triIndex)
                    {
                        if (map.find(faces[i]) == map.end())
                        {
                            vertices.emplace_back(tempVertices[i]);
                            indices.emplace_back(static_cast<UINT>(vertices.size() - 1));
                            map[faces[i]] = static_cast<UINT>(vertices.size() - 1);
                        }
                        else
                        {
                            indices.emplace_back(map[faces[i]]);
                        }
                    }
                }
                else if (tempVertices.size() == 3)
                {
                    for (int i = 0; i < 3; ++i)
                    {
                        if (map.find(faces[i]) == map.end())
                        {
                            vertices.emplace_back(tempVertices[i]);
                            indices.emplace_back(static_cast<UINT>(vertices.size() - 1));
                            map[faces[i]] = static_cast<UINT>(vertices.size() - 1);
                        }
                        else
                        {
                            indices.emplace_back(map[faces[i]]);
                        }
                    }
                }
                else
                {
                    continue;
                }
            }
            else
            {
                continue;
            }
        }

        auto mesh = std::make_shared<Mesh>();
        mesh->renderer = renderer;
        mesh->vertexBuffer = renderer->CreateVertexBuffer(vertices.data(), sizeof(Vertex) * vertices.size());
        mesh->indexBuffer = renderer->CreateIndexBuffer(indices.data(), sizeof(UINT) * indices.size());
        mesh->indexCount = static_cast<UINT>(indices.size());
        mesh->stride = sizeof(Vertex);

        // Test: OBJ 파일 로드 시간 측정.
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        std::cout << duration << "ms" << std::endl;
        
        return mesh;
    }
}
