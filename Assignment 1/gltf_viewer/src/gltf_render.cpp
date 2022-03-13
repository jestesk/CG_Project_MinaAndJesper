// Basic reader and data types for the glTF scene format.
//
// Author: Fredrik Nysjo (2021)
//

#include "gltf_render.h"

namespace gltf {

void create_drawables_from_gltf_asset(DrawableList &drawables, const GLTFAsset &asset)
{
    // First clean up existing OpenGL resources
    destroy_drawables(drawables);

    // Create vertex buffer
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_COPY_WRITE_BUFFER, buffer);
    assert(asset.buffers.size() == 1);
    glBufferData(GL_COPY_WRITE_BUFFER, asset.buffers[0].byteLength, &asset.buffers[0].data[0],
                 GL_STATIC_DRAW);

    // Create one vertex array object per mesh/drawable
    drawables.resize(asset.meshes.size());
    for (unsigned i = 0; i < asset.meshes.size(); ++i) {
        assert(asset.meshes[i].primitives.size() == 1);
        drawables[i].buffer = buffer;

        glGenVertexArrays(1, &drawables[i].vao);
        glBindVertexArray(drawables[i].vao);

        // Specify vertex format
        glBindBuffer(GL_ARRAY_BUFFER, drawables[i].buffer);
        const Primitive &primitive = asset.meshes[i].primitives[0];
        for (const auto &it : primitive.attributes) {
            const Accessor &accessor = asset.accessors[it.index];
            const BufferView &bufferView = asset.bufferViews[accessor.bufferView];

            // Note: must add accessor's byte offset to buffer-view's
            int byteOffset = bufferView.byteOffset + accessor.byteOffset;

            if (it.name.compare("POSITION") == 0) {
                glEnableVertexAttribArray(POSITION);
                // Note: we often declare the position attribute as vec4 in the
                // vertex shader, even if the actual type in the buffer is
                // vec3. This is valid and will give us a homogenous coordinate
                // with the last component assigned the value 1.
                glVertexAttribPointer(POSITION, 3 /*VEC3*/, accessor.componentType, GL_FALSE,
                                      bufferView.byteStride, (GLvoid *)(intptr_t)byteOffset);
            } else if (it.name.compare("COLOR_0") == 0) {
                glEnableVertexAttribArray(COLOR_0);
                glVertexAttribPointer(COLOR_0, 4 /*VEC4*/, accessor.componentType, GL_FALSE,
                                      bufferView.byteStride, (GLvoid *)(intptr_t)byteOffset);
            } else if (it.name.compare("NORMAL") == 0) {
                glEnableVertexAttribArray(NORMAL);
                glVertexAttribPointer(NORMAL, 3 /*VEC3*/, accessor.componentType, GL_FALSE,
                                      bufferView.byteStride, (GLvoid *)(intptr_t)byteOffset);
            } else if (it.name.compare("TEXCOORD_0") == 0) {
                glEnableVertexAttribArray(TEXCOORD_0);
                glVertexAttribPointer(TEXCOORD_0, 2 /*VEC2*/, accessor.componentType, GL_FALSE,
                                      bufferView.byteStride, (GLvoid *)(intptr_t)byteOffset);
            }
            // You can add support for more named attributes here...
        }

        // Specify index format
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawables[i].buffer);
        const Accessor &accessor = asset.accessors[primitive.indices];
        const BufferView &bufferView = asset.bufferViews[accessor.bufferView];
        drawables[i].indexCount = accessor.count;
        drawables[i].indexType = accessor.componentType;
        drawables[i].indexByteOffset = bufferView.byteOffset;
    }
    glBindVertexArray(0);
}

void destroy_drawables(DrawableList &drawables)
{
    for (unsigned i = 0; i < drawables.size(); ++i) {
        glDeleteBuffers(1, &drawables[i].buffer);
        glDeleteVertexArrays(1, &drawables[i].vao);
    }
    drawables.clear();
}

void create_textures_from_gltf_asset(TextureList &textures, const GLTFAsset &asset)
{
    // First clean up existing OpenGL resources
    destroy_textures(textures);

    // Create one texture object per texture in the asset
    textures.resize(asset.textures.size());
    for (unsigned i = 0; i < asset.textures.size(); ++i) {
        const Image &image = asset.images[asset.textures[i].source];

        glGenTextures(1, &textures[i]);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        if (asset.textures[i].hasSampler) {
            const Sampler &sampler = asset.samplers[asset.textures[i].sampler];
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sampler.wrapS);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, sampler.wrapT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampler.minFilter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampler.magFilter);
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width, image.height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, &(image.data[0]));
        // We also need to create a mipmap chain in case GL_TEXTURE_MIN_FILTER
        // is set to something else than GL_NEAREST or GL_LINEAR
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void destroy_textures(TextureList &textures)
{
    if (!textures.size()) return;
    glDeleteTextures(textures.size(), &textures[0]);
    textures.clear();
}

}  // namespace gltf
