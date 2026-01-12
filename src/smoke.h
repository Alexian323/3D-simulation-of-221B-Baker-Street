#pragma once
// ============================================================================
// Le fumigène - VERSION AMÉLIORÉE AVEC VOLUTES
// ============================================================================

#include <random>

struct SmokeParticle {
    glm::vec3 position;
    glm::vec3 velocity;
    float life;        // 0.0 = mort, 1.0 = naissance
    float size;        // Taille de la particule
    float rotation;    // Rotation Z pour variation
    float age;         // Temps écoulé depuis la création
    float swirl;       // Phase pour les volutes
};

class SmokeEmitter {
public:
    std::vector<SmokeParticle> particles;
    glm::vec3 emitterPosition; // Position du bout du cigare
    int maxParticles = 150;
    float emissionRate = 0.03f; // Particules émises très fréquemment (0.05)
    float particleLifetime = 2.5f; // Durée de vie modérée (3.5f secondes)
    
    GLuint vao = 0;
    GLuint vbo = 0;
    
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist{0.8f, 1.2f};
    std::uniform_real_distribution<float> lifeDist{0.8f, 1.2f};
    
    SmokeEmitter(glm::vec3 pos) : emitterPosition(pos) {
        rng.seed(std::random_device{}());
        particles.reserve(maxParticles);
        
        // Créer VAO/VBO pour les particules (instanced rendering)
        setupBuffers();
    }
    
    void setupBuffers() {
        // Quad simple pour billboard (-0.5 à 0.5)
        float quadVertices[] = {
            -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, // Bottom-left
             0.5f, -0.5f, 0.0f,  1.0f, 0.0f, // Bottom-right
             0.5f,  0.5f, 0.0f,  1.0f, 1.0f, // Top-right
            -0.5f,  0.5f, 0.0f,  0.0f, 1.0f  // Top-left
        };
        
        GLuint indices[] = { 0, 1, 2, 0, 2, 3 };
        
        glCreateVertexArrays(1, &vao);
        glCreateBuffers(1, &vbo);
        
        GLuint ebo;
        glCreateBuffers(1, &ebo);
        
        glNamedBufferStorage(vbo, sizeof(quadVertices), quadVertices, 0);
        glNamedBufferStorage(ebo, sizeof(indices), indices, 0);
        
        glVertexArrayVertexBuffer(vao, 0, vbo, 0, 5 * sizeof(float));
        glVertexArrayElementBuffer(vao, ebo);
        
        // Position
        glEnableVertexArrayAttrib(vao, 0);
        glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao, 0, 0);
        
        // UV
        glEnableVertexArrayAttrib(vao, 1);
        glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
        glVertexArrayAttribBinding(vao, 1, 0);
    }
    
    void emit() {
        if (particles.size() >= maxParticles) return;
        
        SmokeParticle p;
        // p.position = emitterPosition;
        float radius = 0.005f; // Diamètre d'un cigare est petit (ex: 0.01m de large)
        p.position = emitterPosition + glm::vec3(
            dist(rng) * radius,  // X : très petite dispersion (ex: +/- 0.005m)
            0.0f,                // Y : Pas de variation verticale au départ
            dist(rng) * radius   // Z : très petite dispersion (ex: +/- 0.005m)
        );
        
        // Vélocité initiale : montée verticale pure
        /*
        p.velocity = glm::vec3(
            dist(rng) * 0.01f,  
            0.000005f + dist(rng) * 0.000001f, // Y: montée DOUCE (0.05 + [+/-0.01])
            dist(rng) * 0.01f   
        );*/
        p.velocity = glm::vec3(
            dist(rng) * 0.002f, // X: RÉDUIT (ex: 0.01f -> 0.002f). Très faible drift latéral.
            0.00000005f + dist(rng) * 0.00000001f, // Y: montée DOUCE (0.05 + [+/-0.01])
            dist(rng) * 0.002f  // Z: RÉDUIT (ex: 0.01f -> 0.002f). Très faible drift latéral.
        );

        p.life = lifeDist(rng);
        p.age = 0.0f;
        p.size = 0.015f + dist(rng) * 0.005f;
        p.rotation = dist(rng) * 3.14159f;
        p.swirl = dist(rng) * 6.28318f; // Phase aléatoire pour volutes
        
        particles.push_back(p);
    }
    
    void update(float deltaTime) {
        // Émission continue de nouvelles particules
        static float emissionTimer = 0.0f;
        emissionTimer += deltaTime;
        while (emissionTimer >= emissionRate) {
            emit();
            emissionTimer -= emissionRate;
        }
        
        // Mise à jour des particules existantes
        for (auto it = particles.begin(); it != particles.end(); ) {
            it->age += deltaTime;
            it->life -= deltaTime / particleLifetime;
            it->size += deltaTime * 0.1f;
            
            if (it->life <= 0.0f) {
                it = particles.erase(it);
                continue;
            }
            
            // Mouvement en volutes (sinusoïdes déphasées)
            float t = it->age;
            float swirlRadius = t * 0.25f; // Les volutes s'élargissent avec le temps
            float swirlSpeed = 1.5f;
            
            // Déplacement latéral en spirale
            float offsetX = std::sin(t * swirlSpeed + it->swirl) * swirlRadius;
            float offsetZ = std::cos(t * swirlSpeed * 0.8f + it->swirl) * swirlRadius;
            
            // Appliquer le mouvement
            it->velocity.x = offsetX * deltaTime * 0.5f;
            it->velocity.z = offsetZ * deltaTime * 0.5f;
            it->velocity.y = 0.08f - (t * 0.01f); // Décélération très légère
            
            it->position += it->velocity * deltaTime * 5.0f;
            
            // Expansion progressive (fumée se dissipe)
            it->size += deltaTime * 0.15f;
            
            // Rotation lente
            it->rotation += deltaTime * 0.2f;
            
            ++it;
        }
    }
    
    void render(GLuint smokeProgram, const glm::mat4& view, const glm::mat4& proj, 
                const glm::vec3& cameraPos) {
        if (particles.empty()) return;
        
        glUseProgram(smokeProgram);
        
        // Matrices
        GLint locView = glGetUniformLocation(smokeProgram, "viewMatrix");
        GLint locProj = glGetUniformLocation(smokeProgram, "projMatrix");
        GLint locCamPos = glGetUniformLocation(smokeProgram, "cameraPosition");
        
        if (locView >= 0) glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        if (locProj >= 0) glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(proj));
        if (locCamPos >= 0) glUniform3fv(locCamPos, 1, glm::value_ptr(cameraPos));
        
        glBindVertexArray(vao);
        
        // Rendu de chaque particule
        for (const auto& p : particles) {
            GLint locPos = glGetUniformLocation(smokeProgram, "particlePos");
            GLint locSize = glGetUniformLocation(smokeProgram, "particleSize");
            GLint locLife = glGetUniformLocation(smokeProgram, "particleLife");
            GLint locRot = glGetUniformLocation(smokeProgram, "particleRotation");
            
            if (locPos >= 0) glUniform3fv(locPos, 1, glm::value_ptr(p.position));
            if (locSize >= 0) glUniform1f(locSize, p.size);
            if (locLife >= 0) glUniform1f(locLife, p.life);
            if (locRot >= 0) glUniform1f(locRot, p.rotation);
            
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
    }
};

// ============================================================================
// SHADERS POUR LA FUMÉE
// ============================================================================

const char* smokeVertexShader = R"(
#version 460

layout(location = 0) in vec3 position; // Position du quad
layout(location = 1) in vec2 uv;

out vec2 vUV;
out float vAlpha;

uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform vec3 cameraPosition;
uniform vec3 particlePos;     // Position de la particule
uniform float particleSize;   // Taille
uniform float particleLife;   // Vie (1.0 = naissance, 0.0 = mort)
uniform float particleRotation; // Rotation Z

void main() {
    vUV = uv;
    
    // Alpha fade : progressif et fluide
    // Fade-in rapide au début, fade-out lent à la fin
    float fadeIn = smoothstep(0.0, 0.1, particleLife);
    float fadeOut = smoothstep(0.0, 0.3, particleLife);
    // vAlpha = fadeIn * fadeOut * 0.25; // Max alpha = 0.25 (fumée visible mais subtile)
    float lifeFade = particleLife * particleLife ;
    vAlpha = lifeFade * 0.16;
    
    // Billboard : toujours face à la caméra
    vec3 forward = normalize(cameraPosition - particlePos);
    vec3 right = normalize(cross(vec3(0, 1, 0), forward));
    vec3 up = cross(forward, right);
    
    // Rotation autour de l'axe forward (billboard rotation)
    float c = cos(particleRotation);
    float s = sin(particleRotation);
    vec3 rotatedRight = right * c - up * s;
    vec3 rotatedUp = right * s + up * c;
    
    // Position finale du vertex
    vec3 worldPos = particlePos 
                  + rotatedRight * position.x * particleSize
                  + rotatedUp * position.y * particleSize;
    
    gl_Position = projMatrix * viewMatrix * vec4(worldPos, 1.0);
}
)";

const char* smokeFragmentShader = R"(
#version 460

in vec2 vUV;
in float vAlpha;
out vec4 fColor;

uniform sampler2D smokeTexture;

void main() {
        // Texture de fumée (utilise canal alpha de la texture)
        vec4 texColor = texture(smokeTexture, vUV);
        
        // Fumée GRIS-BLEU vaporeuse (couleur froide)
        // vec3 smokeColor = vec3(0.55, 0.6, 0.7); // Gris-bleu doux
        vec3 smokeColor = vec3(0.7, 0.75, 0.8);
        
        // Variation subtile de couleur selon la vie
        float t = vAlpha / 0.25; // Normaliser
        smokeColor = mix(
            vec3(0.7, 0.72, 0.75),  // Début: gris-blanc léger
            vec3(0.45, 0.5, 0.6),   // Fin: gris-bleu plus foncé
            1.0 - t
        );
        
        // Alpha combiné : texture alpha * particle life alpha
        float finalAlpha = texColor.a * vAlpha;
        
        // Discard pour optimisation (seuil très bas)
        if (finalAlpha < 0.005) discard;
        
        fColor = vec4(smokeColor, finalAlpha);
    }
)";

float clamp(float num, float low, float high) {
  if (num > high) return high;
  if (num < low) return low;
  return num;
}

// Créer la texture de fumée procédurale
GLuint createSmokeTexture() {
    const int size = 256;
    std::vector<unsigned char> data(size * size);
    
    // Génération d'une texture gaussienne TRÈS DOUCE (fumée vaporeuse)
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float dx = (x - size/2.0f) / (size/2.0f);
            float dy = (y - size/2.0f) / (size/2.0f);
            float dist = std::sqrt(dx*dx + dy*dy);
            
            // Falloff TRÈS DOUX (exp avec facteur réduit)
            float alpha = std::exp(-dist * dist * 1.5f) * 255.0f; // Réduit de 3.0 à 1.5
            data[y * size + x] = static_cast<unsigned char>(clamp(alpha, 0.0f, 255.0f));
        }
    }
    
    GLuint tex;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 4, GL_R8, size, size);
    glTextureSubImage2D(tex, 0, 0, 0, size, size, GL_RED, GL_UNSIGNED_BYTE, data.data());
    glGenerateTextureMipmap(tex);
    
    // Swizzle pour utiliser RED comme alpha
    GLint swizzle[] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
    glTextureParameteriv(tex, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
    
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    return tex;
}
