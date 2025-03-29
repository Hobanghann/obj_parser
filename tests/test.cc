#include <iostream>

#include "include/mtl_parser.h"
#include "include/obj_parser.h"

int main() {
  OBJParser parser;
  if (parser.Parse(
          "C:/Users/zghdl/Desktop/local_repo/obj_parser/tests/Mug.obj") != 0) {
    std::cerr << "Failed to parse OBJ file.\n";
    return 1;
  }

  std::cout << "OBJ Parsed Successfully!\n";
  std::cout << "Vertex Count: " << parser.vertex_buffer().size() << "\n";
  std::cout << "SubObjects: " << parser.sub_objects().size() << "\n";

  for (const auto& sub : parser.sub_objects()) {
    std::cout << "  SubObject: " << sub.sub_object_name << "\n";
    for (const auto& mesh : sub.mesh_groups) {
      std::cout << "    MeshGroup: " << mesh.mesh_group_name << "\n";
      for (const auto& group : mesh.index_groups) {
        std::cout << "      IndexGroup - MTL: " << group.mtl_name
                  << ", Smooth: " << (group.is_smooth_shading ? "Yes" : "No")
                  << ", Indices: " << group.index_buffer_.size() << "\n";
      }
    }
  }

  MTLParser m_parser;
  int result = m_parser.Parse(
      "C:/Users/zghdl/Desktop/local_repo/obj_parser/tests/Mug.mtl");
  if (result != 0) {
    std::cerr << "Failed to parse MTL file.\n";
    return 1;
  }

  auto materials = m_parser.material_map();
  std::cout << "Parsed " << materials.size() << " materials.\n";

  for (const auto& pair : materials) {
    std::cout << "Material: " << pair.first << "\n";
    std::cout << "  Ambient: " << pair.second.ambient_color[0] << ", "
              << pair.second.ambient_color[1] << ", "
              << pair.second.ambient_color[2] << "\n";
    std::cout << "  Diffuse: " << pair.second.diffuse_color[0] << ", "
              << pair.second.diffuse_color[1] << ", "
              << pair.second.diffuse_color[2] << "\n";
    std::cout << "  Specular: " << pair.second.specular_color[0] << ", "
              << pair.second.specular_color[1] << ", "
              << pair.second.specular_color[2] << "\n";
    std::cout << "  Specular Exponent: " << pair.second.specular_exponent
              << "\n";
    std::cout << "  Alpha (opaque): " << pair.second.opaque << "\n";
    std::cout << "  Illumination Model: " << pair.second.illumination_model
              << "\n";
    std::cout << "  Diffuse Map: " << pair.second.diffuse_map << "\n";
    std::cout << "\n";
  }

  return 0;
}