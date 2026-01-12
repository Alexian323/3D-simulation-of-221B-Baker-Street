inline static void* get_accessor_ptr(const cgltf_accessor* accessor)
{
    uint8_t* buffer = (uint8_t*) accessor->buffer_view->buffer->data;
    size_t offset = accessor->buffer_view->offset + accessor->offset;
    return buffer + offset;
}

typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLsizei index_count;
} GLPrimitive;

inline GLPrimitive load_primitive(const cgltf_primitive* primitive)
{
    GLPrimitive out = {0};

    const cgltf_accessor* index_accessor = primitive->indices;
    const void* index_data = get_accessor_ptr(index_accessor);
    GLsizei index_count = index_accessor->count;

    // We must pack the vertex attributes into an interleaved buffer
    const cgltf_accessor* pos_acc = NULL;
    const cgltf_accessor* norm_acc = NULL;
    const cgltf_accessor* uv_acc = NULL;

    for (int i=0; i < primitive->attributes_count; i++) {
        const cgltf_attribute* attr = &primitive->attributes[i];

        switch(attr->type) {
        case cgltf_attribute_type_position: pos_acc = attr->data; break;
        case cgltf_attribute_type_normal:   norm_acc = attr->data; break;
        case cgltf_attribute_type_texcoord: uv_acc = attr->data; break;
        default: break;
        }
    }

    int vertex_count = pos_acc->count;

    // Allocate interleaved vertex array
    typedef struct {
        float pos[3];
        float normal[3];
        float uv[2];
    } Vertex;

    Vertex* vertices = malloc(sizeof(Vertex) * vertex_count);

    float* pos_ptr  = (float*) get_accessor_ptr(pos_acc);
    float* norm_ptr = norm_acc ? (float*) get_accessor_ptr(norm_acc) : NULL;
    float* uv_ptr   = uv_acc ?   (float*) get_accessor_ptr(uv_acc)  : NULL;

    for (int v=0; v < vertex_count; v++) {

        vertices[v].pos[0] = pos_ptr[v*3+0];
        vertices[v].pos[1] = pos_ptr[v*3+1];
        vertices[v].pos[2] = pos_ptr[v*3+2];

        if (norm_ptr) {
            vertices[v].normal[0] = norm_ptr[v*3+0];
            vertices[v].normal[1] = norm_ptr[v*3+1];
            vertices[v].normal[2] = norm_ptr[v*3+2];
        } else {
            vertices[v].normal[0] = vertices[v].normal[1] = vertices[v].normal[2] = 0;
        }

        if (uv_ptr) {
            vertices[v].uv[0] = uv_ptr[v*2+0];
            vertices[v].uv[1] = uv_ptr[v*2+1];
        } else {
            vertices[v].uv[0] = vertices[v].uv[1] = 0;
        }
    }

    glGenVertexArrays(1, &out.vao);
    glGenBuffers(1, &out.vbo);
    glGenBuffers(1, &out.ebo);

    glBindVertexArray(out.vao);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, out.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertex_count, vertices, GL_STATIC_DRAW);

    // EBO
    GLenum index_type;
    switch (index_accessor->component_type) {
        case cgltf_component_type_r_16u: index_type = GL_UNSIGNED_SHORT; break;
        case cgltf_component_type_r_32u: index_type = GL_UNSIGNED_INT;   break;
        default: /* handle others */ index_type = GL_UNSIGNED_SHORT;
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 index_accessor->count * index_accessor->stride,
                 index_data,
                 GL_STATIC_DRAW);

    // Vertex attrib 0 = position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    // Vertex attrib 1 = normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(float)));

    // Vertex attrib 2 = UV
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6*sizeof(float)));

    glBindVertexArray(0);

    out.index_count = index_count;
    return out;
}

inline void draw_primitive(const GLPrimitive* prim)
{
    glBindVertexArray(prim->vao);
    glDrawElements(GL_TRIANGLES, prim->index_count, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
}

/*
for (int m = 0; m < data->meshes_count; m++) {
    const cgltf_mesh* mesh = &data->meshes[m];

    for (int p = 0; p < mesh->primitives_count; p++) {
        draw_primitive(&mesh_primitives[m][p]);  
    }
}

*/