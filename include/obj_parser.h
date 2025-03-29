#ifndef _OBJ_PARSER_H_
#define _OBJ_PARSER_H_

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "mtl_parser.h"

class OBJParser {
  struct Vertex {
    bool operator==(const Vertex& rhs) const {
      return position == rhs.position &&
             (texture_coordinate == rhs.texture_coordinate) &&
             (normal == rhs.normal);
    }
    std::array<REAL, 4> position;
    std::array<REAL, 3> texture_coordinate;
    std::array<REAL, 3> normal;
  };

  struct IndexGroup {
    std::string mtl_name;
    bool is_smooth_shading = true;
    bool is_smooth_shading_empty = true;
    std::vector<INTEGER> index_buffer_;
  };

  struct MeshGroup {
    std::string mesh_group_name;
    std::vector<IndexGroup> index_groups;
  };

  struct SubObject {
    std::string sub_object_name;
    std::vector<MeshGroup> mesh_groups;
  };

 public:
  OBJParser() = default;
  OBJParser(const OBJParser&) = delete;
  OBJParser& operator=(const OBJParser&) = delete;
  ~OBJParser() = default;

  std::vector<Vertex> vertex_buffer() const { return vertex_buffer_; }
  std::vector<SubObject> sub_objects() const { return sub_objects_; }
  std::vector<std::size_t> line_indices() const { return line_indices_; }

  int Parse(const std::string& path) {
    if (input_stream_.is_open()) {
#ifdef DEBUG
      std::cerr << "[OBJParser] Error: Stream is already open.\n";
#endif
      return 1;
    }
    if (path.substr(path.size() - 4) != ".obj") {
#ifdef DEBUG
      std::cerr << "[OBJParser] Error: File is not an .obj file.\n";
#endif
      return 1;
    }
    input_stream_.open(path);
    if (!input_stream_.is_open()) {
#ifdef DEBUG
      std::cerr << "[OBJParser] Error: Failed to open file: " << path << "\n";
#endif
      return 1;
    }
    Clear();
    std::string line;
    std::string kwd;
    std::size_t found;
    while (std::getline(input_stream_, line)) {
      Trim(line);
      if (line.empty()) {
        continue;
      }
      kwd = ReadKeyword(line);
      // parse
      if (kwd == "o") {
        if (line.empty()) {
#ifdef DEBUG
          std::cerr << "[OBJParser] Error: 'o' keyword with empty name.\n";
#endif
          return 1;
        }
        SubObject new_sub;
        new_sub.sub_object_name = line;
        sub_objects_.push_back(new_sub);
      } else if (kwd == "mtllib") {
        if (line.empty()) {
#ifdef DEBUG
          std::cerr << "[OBJParser] Error: 'matlib' keyword with empty name.\n";
#endif
          return 1;
        }
        mtl_name_ = line;
      } else if (kwd == "g") {
        if (line.empty()) {
#ifdef DEBUG
          std::cerr << "[OBJParser] Error: This parser ignore 'g' keyword.\n";
#endif
          return 1;
        }
        if (sub_objects_.empty()) {
          SubObject new_sub;
          new_sub.sub_object_name = "Unnamed";
        }
        MeshGroup new_group;
        new_group.mesh_group_name = line;
        sub_objects_.back().mesh_groups.push_back(new_group);
      } else if (kwd == "usemtl") {
        if (line.empty()) {
#ifdef DEBUG
          std::cerr << "[OBJParser] Error: 'usemtl' keyword with empty name.\n";
#endif
          return 1;
        }
        if (sub_objects_.empty()) {
          SubObject new_sub;
          new_sub.sub_object_name = "Unnamed";
          sub_objects_.push_back(new_sub);
        }
        if (sub_objects_.back().mesh_groups.empty()) {
          MeshGroup new_mesh;
          new_mesh.mesh_group_name = "Unnamed";
          sub_objects_.back().mesh_groups.push_back(new_mesh);
        }
        material_name_ = line;
        if (sub_objects_.back().mesh_groups.back().index_groups.empty() ||
            !sub_objects_.back()
                 .mesh_groups.back()
                 .index_groups.back()
                 .mtl_name.empty()) {
          IndexGroup new_index;
          new_index.mtl_name = material_name_;
          new_index.is_smooth_shading = is_smooth_shading_mode_;
          sub_objects_.back().mesh_groups.back().index_groups.push_back(
              new_index);
        }
        if (sub_objects_.back()
                .mesh_groups.back()
                .index_groups.back()
                .mtl_name.empty()) {
          sub_objects_.back().mesh_groups.back().index_groups.back().mtl_name =
              material_name_;
        }

      } else if (kwd == "v") {
        std::array<REAL, 4> vector4;
        std::vector<REAL> g_components = ReadComponents<REAL>(line);
        if (g_components.size() < 3 || g_components.size() > 4) {
#ifdef DEBUG
          std::cerr << "[OBJParser] Error: 'v' expects 3 or 4 components.\n";
#endif
          return 1;
        }
        g_components.push_back(1.f);
        for (int i = 0; i < 4; i++) {
          vector4[i] = g_components[i];
        }
        positions_.push_back(vector4);
      } else if (kwd == "vt") {
        std::array<REAL, 3> vector3;
        std::vector<REAL> t_components = ReadComponents<REAL>(line);
        if (t_components.size() < 2 || t_components.size() > 3) {
#ifdef DEBUG
          std::cerr << "[OBJParser] Error: 'vt' expects 2 or 3 components.\n";
#endif
          return 1;
        }
        t_components.push_back(0.f);
        for (int i = 0; i < 3; i++) {
          vector3[i] = t_components[i];
        }
        texture_coordinates_.push_back(vector3);
      } else if (kwd == "vn") {
        std::array<REAL, 3> vector3;
        std::vector<REAL> t_components = ReadComponents<REAL>(line);
        if (t_components.size() != 3) {
#ifdef DEBUG
          std::cerr << "[OBJParser] Error: 'vn' expects 3 components.\n";
#endif
          return 1;
        }
        for (int i = 0; i < 3; i++) {
          vector3[i] = t_components[i];
        }
        normals_.push_back(vector3);
      } else if (kwd == "s") {
        if (sub_objects_.empty()) {
          SubObject new_sub;
          new_sub.sub_object_name = "Unnamed";
        }
        if (sub_objects_.back().mesh_groups.empty()) {
          MeshGroup new_mesh;
          new_mesh.mesh_group_name = "Unnamed";
        }
        if (line == "1") {
          is_smooth_shading_mode_ = true;
        } else if (line == "off") {
          is_smooth_shading_mode_ = false;
        } else {
#ifdef DEBUG
          std::cerr << "[OBJParser] Error: 's' keyword with empty option.\n";
#endif
          return 1;
        }
        if (sub_objects_.back().mesh_groups.back().index_groups.empty() ||
            !sub_objects_.back()
                 .mesh_groups.back()
                 .index_groups.back()
                 .is_smooth_shading_empty) {
          IndexGroup new_index;
          new_index.is_smooth_shading = is_smooth_shading_mode_;
          new_index.is_smooth_shading_empty = false;
          new_index.mtl_name = material_name_;
          sub_objects_.back().mesh_groups.back().index_groups.push_back(
              new_index);
        }
        if (sub_objects_.back()
                .mesh_groups.back()
                .index_groups.back()
                .is_smooth_shading_empty) {
          sub_objects_.back()
              .mesh_groups.back()
              .index_groups.back()
              .is_smooth_shading_empty = false;
          sub_objects_.back()
              .mesh_groups.back()
              .index_groups.back()
              .is_smooth_shading = is_smooth_shading_mode_;
        }
      } else if (kwd == "f") {
        std::istringstream iss(line);
        std::string indices_buf;
        while (iss >> indices_buf) {
          std::size_t first_slash = indices_buf.find("/");
          std::size_t second_slash = indices_buf.find("/", first_slash + 1);
          // v
          if (first_slash == std::string::npos &&
              second_slash == std::string::npos) {
            indices_buf.append("/0/0");
          }
          // v/vt
          else if (first_slash != std::string::npos &&
                   second_slash == std::string::npos) {
            indices_buf.append("/0");
          }
          // v//vn
          else if ((first_slash != std::string::npos &&
                    second_slash != std::string::npos) &&
                   (second_slash - first_slash) == 1) {
            indices_buf.insert(second_slash, "0");
          }
          // v/vt/vn
          else if ((first_slash != std::string::npos &&
                    second_slash != std::string::npos) &&
                   (second_slash - first_slash) > 1) {
            // do nothing
          } else {
#ifdef DEBUG
            std::cerr << "[OBJParser] Error: Malformed face index format.\n";
#endif
            return 1;
          }
          std::replace(indices_buf.begin(), indices_buf.end(), '/', ' ');
          std::istringstream buf_iss(indices_buf);
          std::size_t g_index, t_index, n_index;
          buf_iss >> g_index >> t_index >> n_index;
          AddVertex(g_index, t_index, n_index);
        }
      } else if (kwd == "l") {
        if (line.empty()) {
#ifdef DEBUG
          std::cerr << "[OBJParser] Error: 'l' keyword with empty indices.\n";
#endif
          return 1;
        }
        std::istringstream iss(line);
        std::size_t index;
        while (iss >> index) {
          if (index < 0 || index >= positions_.size()) {
#ifdef DEBUG
            std::cerr << "[OBJParser] Error: Line index out of range.\n";
#endif
            return 1;
          }
          line_indices_.push_back(index);
        }
      } else {
#ifdef DEBUG
        std::cerr << "[OBJParser] Error: Unknown keyword '" << kwd << "'.\n";
#endif
        return 1;
      }
    }
    input_stream_.close();
    return 0;
  }

  int Clear() {
    object_name_.clear();
    mtl_name_.clear();
    positions_.clear();
    texture_coordinates_.clear();
    normals_.clear();
    vertex_map_.clear();
    vertex_buffer_.clear();
    sub_objects_.clear();
    line_indices_.clear();
    return 0;
  }

 private:
  const std::array<REAL, 3> max_vector3 = {std::numeric_limits<REAL>::max(),
                                           std::numeric_limits<REAL>::max(),
                                           std::numeric_limits<REAL>::max()};

  const std::array<REAL, 4> max_vector4 = {
      std::numeric_limits<REAL>::max(), std::numeric_limits<REAL>::max(),
      std::numeric_limits<REAL>::max(), std::numeric_limits<REAL>::max()};

  struct HashFunction {
    std::size_t operator()(Vertex key) const {
      std::hash<REAL> v_hash;
      std::size_t g_hash = v_hash(key.position[0]) ^ v_hash(key.position[1]) ^
                           v_hash(key.position[2]) ^ v_hash(key.position[3]);
      std::size_t t_hash = v_hash(key.texture_coordinate[0]) ^
                           v_hash(key.texture_coordinate[1]) ^
                           v_hash(key.texture_coordinate[2]);
      std::size_t n_hash =
          v_hash(key.normal[0]) ^ v_hash(key.normal[1]) ^ v_hash(key.normal[2]);

      return g_hash ^ (t_hash << 1) ^ (n_hash << 2);
    }
  };

  std::string& Trim(std::string& s) {
    if (s.empty()) {
      return s;
    }
    size_t first = s.find_first_not_of(" ");
    size_t last = s.find_last_not_of(" ");
    if (first == std::string::npos || last == std::string::npos) {
      s.clear();
      return s;
    }
    s = s.substr(first, last - first + 1);
    size_t comment = s.find('#');
    if (comment != std::string::npos) s = s.substr(0, comment);
    return s;
  }

  std::string ReadKeyword(std::string& s) {
    size_t found;
    if ((found = s.find(" ")) == std::string::npos) {
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

  void AddVertex(std::size_t g_index, std::size_t t_index,
                 std::size_t n_index) {
    Vertex new_vertex;
    new_vertex.position = positions_[g_index - 1];
    new_vertex.texture_coordinate =
        t_index != 0 ? texture_coordinates_[t_index - 1] : max_vector3;
    new_vertex.normal = n_index != 0 ? normals_[n_index - 1] : max_vector3;
    auto found = vertex_map_.find(new_vertex);
    if (found != vertex_map_.end()) {
      sub_objects_.back()
          .mesh_groups.back()
          .index_groups.back()
          .index_buffer_.push_back(found->second);
    } else {
      sub_objects_.back()
          .mesh_groups.back()
          .index_groups.back()
          .index_buffer_.push_back(vertex_buffer_.size());
      vertex_map_.insert({new_vertex, vertex_buffer_.size()});
      vertex_buffer_.push_back(new_vertex);
    }
  }

  std::string object_name_;
  std::string mtl_name_;
  std::vector<std::array<REAL, 4>> positions_;
  std::vector<std::array<REAL, 3>> texture_coordinates_;
  std::vector<std::array<REAL, 3>> normals_;
  std::unordered_map<Vertex, std::size_t, HashFunction> vertex_map_;

  std::vector<Vertex> vertex_buffer_;
  std::vector<SubObject> sub_objects_;
  std::vector<std::size_t> line_indices_;
  std::ifstream input_stream_;

  std::string material_name_;
  bool is_smooth_shading_mode_ = true;
};
#endif  // _OBJ_PARESR_H_