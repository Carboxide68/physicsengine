
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf/tiny_gltf.h>
namespace tinygltf_impl {
    using namespace tinygltf;
    int load_model(TinyGLTF &loader, Model *model, std::string model_name) {
        std::string err;
        std::string warn;

        bool ret = loader.LoadASCIIFromFile(model, &err, &warn, model_name);

        if (!warn.empty()) {
          printf("Warn: %s\n", warn.c_str());
        }

        if (!err.empty()) {
          printf("Err: %s\n", err.c_str());
        }

        if (!ret) {
          printf("Failed to parse glTF\n");
          return -1;
        }
        return 0;
    }
    
    std::vector<float> MeshToFloats(Model& model, Mesh& mesh) {
        
        std::vector<float> floats;
        BufferView index_buffer = model.bufferViews[model.accessors[mesh.primitives[0].indices].bufferView];
        uint16_t *indices = (uint16_t*)((size_t)model.buffers[index_buffer.buffer].data.data() + (size_t)index_buffer.byteOffset);

        BufferView normal_buffer = model.bufferViews[model.accessors[mesh.primitives[0].attributes["NORMAL"]].bufferView];
        float *normals = (float*)((size_t)model.buffers[normal_buffer.buffer].data.data() + (size_t)normal_buffer.byteOffset);

        BufferView position_buffer = model.bufferViews[model.accessors[mesh.primitives[0].attributes["POSITION"]].bufferView];
        float *positions = (float*)((size_t)model.buffers[position_buffer.buffer].data.data() + (size_t)position_buffer.byteOffset);

        for (uint16_t *i = indices; (size_t)i < (size_t)indices + (size_t)index_buffer.byteLength; i++) {
            floats.push_back(positions[(*i)*3]);
            floats.push_back(positions[(*i)*3+1]);
            floats.push_back(positions[(*i)*3+2]);

            floats.push_back(normals[(*i)*3]);
            floats.push_back(normals[(*i)*3+1]);
            floats.push_back(normals[(*i)*3+2]);
        }
        return floats;
    }
}
