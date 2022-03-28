#pragma once

#include "common.h"
#include <GL/glew.h>

class Buffer {
public:

    Buffer(const Buffer& other) = delete;

    ~Buffer() {
        glDeleteBuffers(1, &m_id);
    }

    /**
     * @brief Creates a shared ptr to a new buffer
     *
     * @param size The size of the buffer
     * @param access_profile
     *
     * @return 
     */
    static Ref<Buffer> Create(size_t size, GLenum access_profile) {
        return Ref<Buffer>(new Buffer(size, access_profile));
    }

    /**
     * @brief Bind the buffer to a certain target
     *
     * @param target An enum to a specific target
     */
    inline void bind(GLenum target) {
        glBindBuffer(target, m_id);
    }
    
    static inline void unbind(GLenum target) {
        glBindBuffer(target, 0);
    }

    /**
     * @brief Get the contents of a GPU buffer
     *
     * @param data A pointer to a buffer with the size of at least ${size}
     * @param offset The offset into the GPU buffer
     * @param size The amount of bytes to read in the GPU buffer
     */
    inline void getContents(void *data, size_t offset, size_t size) {
        if (size + offset > m_size) return;
        glBindBuffer(GL_COPY_READ_BUFFER, m_id);
        glGetBufferSubData(GL_COPY_READ_BUFFER, offset, size, data);
        glBindBuffer(GL_COPY_READ_BUFFER, 0);
    }
    
    /**
     * @brief Populate a part of a buffer with data
     *
     * @param data The data to populate the buffer with
     * @param offset Offset into the buffer to begin
     * @param size The amount of data to write in bytes
     */
    inline void subData(void *data, size_t offset, size_t size) {
        if (size + offset > m_size) {
            printf("Can't sub data outside of the buffer!\n");
            return;
        }
        glBindBuffer(GL_COPY_WRITE_BUFFER, m_id);
        glBufferSubData(GL_COPY_WRITE_BUFFER, offset, size, data);
        glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
    }

    /**
     * @brief Get the handle to the underlying GL object
     */
    inline uint getHandle() const {
        return m_id;
    }

private:

    Buffer(size_t size, GLenum access_profile) {
        m_size = size;
        glGenBuffers(1, &m_id);
        glBindBuffer(GL_COPY_WRITE_BUFFER, m_id);
        glBufferData(GL_COPY_WRITE_BUFFER, m_size, 0, access_profile);
        glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
    }

    uint m_id;
    size_t m_size;

};
