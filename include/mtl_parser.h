#ifndef _MATERIAL_PARSER_H_
#define _MATERIAL_PARSER_H_

#define DEBUG

#define REAL float
#define INTEGER unsigned int

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

struct Material {
  std::string name_;
  std::array<REAL, 3> ambient_color;
  std::array<REAL, 3> diffuse_color;
  std::array<REAL, 3> specular_color;
  std::array<REAL, 3> emmesive_color;
  REAL specular_exponent;
  REAL opaque;  // used as alpha value in ARGB format.
  std::array<REAL, 3> transmission_filter_color;
  REAL optical_density;
  std::uint32_t illumination_model;
  std::string ambient_map;
  std::string diffuse_map;
  std::string specular_map;
  std::string specular_highlight_map;
  std::string alpha_map;
  std::string bump_map;
  std::string displacement_map;
  std::string roughness_map;
  std::string metallic_map;
  std::string sheen_map;
  std::string emmissive_map;
  std::string normal_map;
};

class MTLParser {
 public:
  MTLParser() = default;
  MTLParser(const MTLParser&) = delete;
  MTLParser& operator=(const MTLParser&) = delete;
  ~MTLParser() = default;

  std::unordered_map<std::string, Material> material_map() const {
    return material_map_;
  }

  Material GetMaterial(const std::string& name) {
    auto itr = material_map_.find(name);
    if (itr != material_map_.end()) {
      return itr->second;
    }
    // error occur : there is no such material
    return Material();
  }

  int Parse(const std::string& path) {
    if (input_stream_.is_open()) {
#ifdef DEBUG
      std::cerr << "[MTLParser] Error: Stream is already open.\n";
#endif
      return 1;
    }
    if (path.substr(path.size() - 4, 4) != ".mtl") {
#ifdef DEBUG
      std::cerr << "[MTLParser] Error: Not a .mtl file.\n";
#endif
      return 1;
    }
    input_stream_.open(path);
    if (!input_stream_.is_open()) {
#ifdef DEBUG
      std::cerr << "[MTLParser] Error: Failed to open file '" << path << "'.\n";
#endif
      return 1;
    }
    std::string line;
    std::string kwd;
    std::unordered_map<std::string, Material>::iterator last_itr;
    while (std::getline(input_stream_, line)) {
      if (line.empty()) {
        continue;
      }
      Trim(line);
      kwd = ReadKeyword(line);
      // parse
      if (kwd.empty()) {
        continue;
      } else if (kwd == "#") {
        continue;
      } else if (kwd == "newmtl") {
        Material new_material;
        if (line.empty()) {
#ifdef DEBUG
          std::cerr << "[MTLParser] Error: 'newmtl' with no material name.\n";
#endif
          return 1;
        }
        new_material.name_ = line;
        material_map_.insert({line, new_material});
        last_itr = material_map_.find(line);
      } else if (kwd == "Ka" || kwd == "Kd" || kwd == "Ks" || kwd == "Ke") {
        if (material_map_.empty()) {
#ifdef DEBUG
          std::cerr << "[MTLParser] Error: 'newmtl' with no material name.\n";
#endif
          return 1;
        }
        std::vector<REAL> components = ReadComponents<REAL>(line);
        if (components.size() != 3) {
#ifdef DEBUG
          std::cerr << "[MTLParser] Error: '" << kwd
                    << "' expects 3 components.\n";
#endif
          return 1;
        }
        std::array<REAL, 3> vector3;
        for (int i = 0; i < 3; i++) {
          vector3[i] = components[i];
        }
        if (kwd == "Ka") {
          last_itr->second.ambient_color = vector3;
        } else if (kwd == "Kd") {
          last_itr->second.diffuse_color = vector3;
        } else if (kwd == "Ks") {
          last_itr->second.specular_color = vector3;
        } else {
          last_itr->second.emmesive_color = vector3;
        }
      } else if (kwd == "Ns") {
        std::vector<REAL> components = ReadComponents<REAL>(line);
        if (components.size() != 1) {
#ifdef DEBUG
          std::cerr << "[MTLParser] Error: '" << kwd
                    << "' expects 1 component.\n";
#endif
          return 1;
        }
        last_itr->second.specular_exponent = components[0];
      } else if (kwd == "d" || kwd == "Tr") {
        std::vector<REAL> components = ReadComponents<REAL>(line);
        if (components.size() != 1) {
#ifdef DEBUG
          std::cerr << "[MTLParser] Error: '" << kwd
                    << "' expects 1 component.\n";
#endif
          return 1;
        }
        if (kwd == "d") {
          last_itr->second.opaque = components[0];
        } else {
          last_itr->second.opaque = 1.f - components[0];
        }
      } else if (kwd == "Tf") {
        std::vector<REAL> components = ReadComponents<REAL>(line);
        if (components.size() != 3) {
#ifdef DEBUG
          std::cerr << "[MTLParser] Error: '" << kwd
                    << "' expects 3 component.\n";
#endif
          return 1;
        }
        std::array<REAL, 3> vector3;
        for (int i = 0; i < 3; i++) {
          vector3[i] = components[i];
        }
        last_itr->second.transmission_filter_color = vector3;
      } else if (kwd == "Ni") {
        std::vector<REAL> components = ReadComponents<REAL>(line);
        if (components.size() != 1) {
#ifdef DEBUG
          std::cerr << "[MTLParser] Error: '" << kwd
                    << "' expects 1 component.\n";
#endif
          return 1;
        }
        last_itr->second.optical_density = components[0];
      } else if (kwd == "illum") {
        std::vector<INTEGER> components = ReadComponents<INTEGER>(line);
        if (components.size() != 1) {
#ifdef DEBUG
          std::cerr << "[MTLParser] Error: '" << kwd
                    << "' expects 1 component.\n";
#endif
          return 1;
        }
        last_itr->second.illumination_model = components[0];
      } else if (kwd == "map_Ka") {
        std::vector<std::string> components = ReadComponents<std::string>(line);
        if (components.size() != 1) {
#ifdef DEBUG
          std::cerr << "[MTLParser] Error: Texture map '" << kwd
                    << "' expects 1 filename.\n";
#endif
          return 1;
        }
        last_itr->second.ambient_map = components[0];
      } else if (kwd == "map_Kd") {
        std::vector<std::string> components = ReadComponents<std::string>(line);
        if (components.size() != 1) {
#ifdef DEBUG
          std::cerr << "[MTLParser] Error: Texture map '" << kwd
                    << "' expects 1 filename.\n";
#endif
          return 1;
        }
        last_itr->second.diffuse_map = components[0];
      } else if (kwd == "map_Ks") {
        std::vector<std::string> components = ReadComponents<std::string>(line);
        if (components.size() != 1) {
#ifdef DEBUG
          std::cerr << "[MTLParser] Error: Texture map '" << kwd
                    << "' expects 1 filename.\n";
#endif
          return 1;
        }
        last_itr->second.specular_map = components[0];
      } else if (kwd == "map_Ns") {
        std::vector<std::string> components = ReadComponents<std::string>(line);
        if (components.size() != 1) {
#ifdef DEBUG
          std::cerr << "[MTLParser] Error: Texture map '" << kwd
                    << "' expects 1 filename.\n";
#endif
          return 1;
        }
        last_itr->second.specular_highlight_map = components[0];
      } else if (kwd == "map_d") {
        std::vector<std::string> components = ReadComponents<std::string>(line);
        if (components.size() != 1) {
#ifdef DEBUG
          std::cerr << "[MTLParser] Error: Texture map '" << kwd
                    << "' expects 1 filename.\n";
#endif
          return 1;
        }
        last_itr->second.alpha_map = components[0];
      } else if (kwd == "map_Bump" || kwd == "bump") {
        std::vector<std::string> components = ReadComponents<std::string>(line);
        if (components.size() != 1) {
#ifdef DEBUG
          std::cerr << "[MTLParser] Error: Texture map '" << kwd
                    << "' expects 1 filename.\n";
#endif
          return 1;
        }
        last_itr->second.bump_map = components[0];
      } else if (kwd == "disp") {
        std::vector<std::string> components = ReadComponents<std::string>(line);
        if (components.size() != 1) {
#ifdef DEBUG
          std::cerr << "[MTLParser] Error: Texture map '" << kwd
                    << "' expects 1 filename.\n";
#endif
          return 1;
        }
        last_itr->second.displacement_map = components[0];
      } else if (kwd == "Pr" || kwd == "map_Pr") {
        std::vector<std::string> components = ReadComponents<std::string>(line);
        if (components.size() != 1) {
#ifdef DEBUG
          std::cerr << "[MTLParser] Error: Texture map '" << kwd
                    << "' expects 1 filename.\n";
#endif
          return 1;
        }
        last_itr->second.roughness_map = components[0];
      } else if (kwd == "Pm" || kwd == "map_Pm") {
        std::vector<std::string> components = ReadComponents<std::string>(line);
        if (components.size() != 1) {
#ifdef DEBUG
          std::cerr << "[MTLParser] Error: Texture map '" << kwd
                    << "' expects 1 filename.\n";
#endif
          return 1;
        }
        last_itr->second.metallic_map = components[0];
      } else {
#ifdef DEBUG
        std::cerr << "[MTLParser] Error: Unknown keyword '" << kwd << "'.\n";
#endif
        return 1;
      }
    }
    return 0;
  }

  int Clear() {
    material_map_.clear();
    input_stream_.close();
    return 0;
  }

 private:
  std::string& Trim(std::string& s) {
    if (s.empty()) {
      return s;
    }
    size_t first = s.find_first_not_of(" ");
    size_t last = s.find_last_not_of(" ");
    if (first == last) {
      return s;
    }
    return s = s.substr(first, last - first + 1);
  }

  std::string ReadKeyword(std::string& s) {
    size_t found;
    if ((found = s.find_first_of(" ")) == std::string::npos) {
      return s;
    }
    std::string kwd = s.substr(0, found);
    s = s.substr(found + 1);
    return kwd;
  }

  template <class T>
  std::vector<T> ReadComponents(std::string s) {
    std::istringstream iss(s);
    std::vector<T> components;
    T component;
    while (iss >> component) {
      components.push_back(component);
    }
    return components;
  }

  std::unordered_map<std::string, Material> material_map_;
  std::ifstream input_stream_;
};

#endif  // _MATERIAL_PARSER_H_