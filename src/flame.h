// ============================================================================
// SHADERS POUR LA FLAMME (À ajouter après les shaders de fumée)
// ============================================================================

const char* flameVertexShader = R"(
#version 460

layout(location=0) in vec3 position;
layout(location=1) in vec2 texCoord;

out vec2 TexCoord;

uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform vec3 flamePosition;
uniform float flameSize;
uniform float time;

void main() {
        // Extraire uniquement les axes Right et Up (ignorer translation)
        vec3 cameraRight = normalize(vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]));
        vec3 cameraUp = normalize(vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]));
        
        // Construire position du vertex en espace monde
        // float flameWobble = sin(time * 6.0 + position.y * 10.0) * 0.015 * position.y;

        float wobbleFactor = texCoord.y; 
        float baseWobble = sin(time * 6.0 + position.y * 8.0) * 0.015;
        float flameWobble = baseWobble * wobbleFactor * 2;

        vec3 billboardLocalPos = vec3(position.x + flameWobble, position.y, position.z);
        vec3 worldPos = flamePosition + cameraRight * billboardLocalPos.x * flameSize + cameraUp * billboardLocalPos.y * flameSize;    
        gl_Position = projMatrix * viewMatrix * vec4(worldPos, 1.0);
        TexCoord = texCoord;
    }
)";

const char* flameFragmentShader = R"(
#version 460

in vec2 TexCoord;
out vec4 fragColor;

uniform sampler2D flameTexture;
uniform float time;

void main() {
    vec4 texColor = texture(flameTexture, TexCoord);
    
    // Animation de scintillement subtile
    // float flicker = 0.95 + 0.01 * sin(time * 12.0 + TexCoord.y * 3.14159);
    float flicker = 0.95 + 0.01 * sin(time * 12.0 + TexCoord.y * 5);
    
    // Couleur légèrement plus chaude
    vec3 warmColor = texColor.rgb * vec3(1.05, 1.0, 0.9) * flicker;
    
    fragColor = vec4(warmColor, texColor.a);
    
    // Rejeter pixels quasi-transparents
    if (fragColor.a < 0.05) discard;
}
)";

// ============================================================================
// FONCTION D'INITIALISATION DU BILLBOARD DE FLAMME
// ============================================================================

void initFlameQuad(GLuint& vao, GLuint& vbo) {
    // Quad simple avec ratio 2:3 (comme l'image 1024x1536)
    float quadVertices[] = {
        // Position (X, Y, Z)    // TexCoord (U, V)
        -0.5f, -0.75f, 0.0f,     0.0f, 0.0f,  // Bottom-left
         0.5f, -0.75f, 0.0f,     1.0f, 0.0f,  // Bottom-right
         0.5f,  0.75f, 0.0f,     1.0f, 1.0f,  // Top-right
         
        -0.5f, -0.75f, 0.0f,     0.0f, 0.0f,  // Bottom-left
         0.5f,  0.75f, 0.0f,     1.0f, 1.0f,  // Top-right
        -0.5f,  0.75f, 0.0f,     0.0f, 1.0f   // Top-left
    };
    
    glCreateVertexArrays(1, &vao);
    glCreateBuffers(1, &vbo);
    
    glNamedBufferStorage(vbo, sizeof(quadVertices), quadVertices, 0);
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, 5 * sizeof(float));
    
    // Position (location = 0)
    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);
    
    // TexCoord (location = 1)
    glEnableVertexArrayAttrib(vao, 1);
    glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glVertexArrayAttribBinding(vao, 1, 0);
}
