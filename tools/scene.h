#pragma once

#include "material.h"
#include "vector.h"
#include "object.h"
#include "light.h"

#include <vector>
#include <unordered_map>
#include <string>
#include <filesystem>

#include <util.h>
#include <fstream>
#include <iostream>
#include <sstream>

class Scene {
public:
    Scene(const std::vector<Object>& objects, const std::vector<SphereObject>& sphere_objects,
          const std::vector<Light>& lights, std::unordered_map<std::string, Material>& materials)
        : objects_(objects), sphere_objects_(sphere_objects), lights_(lights) {
        materials_ = std::move(materials);
    }

    const std::vector<Object>& GetObjects() const {
        return objects_;
    }
    const std::vector<SphereObject>& GetSphereObjects() const {
        return sphere_objects_;
    }
    const std::vector<Light>& GetLights() const {
        return lights_;
    }
    const std::unordered_map<std::string, Material>& GetMaterials() const {
        return materials_;
    }

private:
    std::vector<Object> objects_;
    std::vector<SphereObject> sphere_objects_;
    std::vector<Light> lights_;
    std::unordered_map<std::string, Material> materials_;
};

std::vector<std::string> ParseString(const std::string& line) {
    std::vector<std::string> output;
    std::istringstream ss(line);
    std::string part;
    while (ss) {
        ss >> part;
        output.push_back(std::move(part));
    }
    output.pop_back();
    return output;
}

Vector ReadVector(const std::vector<std::string>& parsed) {
    return Vector({std::stod(parsed[1]), std::stod(parsed[2]), std::stod(parsed[3])});
}

SphereObject ReadSphereObject(const std::vector<std::string>& parsed) {
    Sphere sphere(Vector(std::stod(parsed[1]), std::stod(parsed[2]), std::stod(parsed[3])),
                  std::stod(parsed[4]));
    return SphereObject{nullptr, sphere};
}

Light ReadLightObject(const std::vector<std::string>& parsed) {
    Light light(Vector(std::stod(parsed[1]), std::stod(parsed[2]), std::stod(parsed[3])),
                Vector(std::stod(parsed[4]), std::stod(parsed[5]), std::stod(parsed[6])));
    return light;
}

// parses x//y
auto ParsePair(const std::string& line) {
    ssize_t first_number, second_number;
    std::sscanf(line.c_str(), "%zd//%zd", &first_number, &second_number);
    return std::make_pair(first_number, second_number);
}

// parses x/y/z
auto ParseTriple(const std::string& line) {
    ssize_t first_number, second_number, third_number;
    std::sscanf(line.c_str(), "%zd/%zd/%zd", &first_number, &second_number, &third_number);
    return std::make_tuple(first_number, second_number, third_number);
}

decltype(auto) GetFromContainer(ssize_t idx, const auto& container) {
    if (idx < 0) {
        idx += std::ssize(container);
    } else {
        --idx;
    }
    return container[idx];
}

std::unordered_map<std::string, Material> ReadMaterials(const std::filesystem::path& path) {
    std::ifstream input(path);
    std::string line;

    std::unordered_map<std::string, Material> materials;

    Material material;
    bool currently_in_material{false};

    while (getline(input, line)) {
        auto parsed_line = ParseString(line);
        if (parsed_line[0].starts_with("#")) {
            continue;
        }
        if (parsed_line[0] == "newmtl") {
            if (currently_in_material) {
                materials[material.name] = material;
            }
            material = {};
            material.name = std::move(parsed_line[1]);
            currently_in_material = true;
        } else {
            if (parsed_line[0] == "Ks") {
                material.specular_color = ReadVector(parsed_line);
            }

            if (parsed_line[0] == "Ka") {
                material.ambient_color = ReadVector(parsed_line);
            }

            if (parsed_line[0] == "Kd") {
                material.diffuse_color = ReadVector(parsed_line);
            }

            if (parsed_line[0] == "Ke") {
                material.intensity = ReadVector(parsed_line);
            }

            if (parsed_line[0] == "Ns") {
                material.specular_exponent = std::stod(parsed_line[1]);
            }

            if (parsed_line[0] == "Ni") {
                material.refraction_index = std::stod(parsed_line[1]);
            }

            if (parsed_line[0] == "al") {
                material.albedo = ReadVector(parsed_line);
            }
        }
    }

    materials[material.name] = material;

    return materials;
};

Scene ReadScene(const std::filesystem::path& path) {
    std::ifstream input(path);
    std::string line;

    std::vector<Object> objects;
    std::vector<Vector> vertexes;
    std::vector<Vector> normals;
    std::unordered_map<std::string, Material> materials;
    std::vector<SphereObject> sphere_objects;
    std::vector<Light> light_objects;

    Material* current_material = nullptr;

    while (getline(input, line)) {
        auto parsed = ParseString(line);

        if (parsed.empty() || parsed[0].starts_with("#")) {
            continue;
        }

        if (parsed[0] == "v") {
            vertexes.push_back(ReadVector(parsed));
            continue;
        }

        if (parsed[0] == "vn") {
            normals.push_back(ReadVector(parsed));
            continue;
        }

        if (parsed[0] == "S") {
            auto sphere = ReadSphereObject(parsed);
            sphere.material = current_material;
            sphere_objects.push_back(sphere);
            continue;
        }

        if (parsed[0] == "P") {
            light_objects.push_back(ReadLightObject(parsed));
            continue;
        }

        if (parsed[0] == "f") {
            std::vector<Vector> polygon_points;
            std::vector<Vector> optional_normals;

            for (size_t i = 1; i < parsed.size(); ++i) {
                const auto& string_point = parsed[i];

                if (string_point.find("//") != std::string::npos) {
                    // case: x//y
                    auto [x, y] = ParsePair(string_point);

                    polygon_points.push_back(GetFromContainer(x, vertexes));
                    optional_normals.push_back(GetFromContainer(y, normals));
                } else if (string_point.find("/") != std::string::npos) {
                    // case: x/y/z
                    auto [x, y, z] = ParseTriple(string_point);

                    polygon_points.push_back(GetFromContainer(x, vertexes));
                    optional_normals.push_back(GetFromContainer(z, normals));
                } else {
                    auto idx = static_cast<ssize_t>(std::stoi(parsed[i]));

                    polygon_points.push_back(GetFromContainer(idx, vertexes));
                    optional_normals.push_back({0, 0, 0});
                }
            }

            // creates triangle of form (0, i, i+1) from polygon
            for (size_t idx = 1; idx < polygon_points.size() - 1; ++idx) {
                Triangle triangle(polygon_points[0], polygon_points[idx], polygon_points[idx + 1]);

                std::array<Vector, 3> opt_normals = {optional_normals[0], optional_normals[idx],
                                                     optional_normals[idx + 1]};

                Object object(current_material, triangle, opt_normals);
                objects.push_back(object);
            }

            continue;
        }

        if (parsed[0] == "mtllib") {
            auto mtl_path = path.parent_path() / std::filesystem::path(parsed[1]);
            materials = ReadMaterials(mtl_path);
        }

        if (parsed[0] == "usemtl") {
            current_material = &materials[parsed[1]];
        }
    }

    return Scene(objects, sphere_objects, light_objects, materials);
};
