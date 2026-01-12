#include <SDL3/SDL.h>
#include <geGL/geGL.h>
#include <geGL/StaticCalls.h>
#include <cmath>
#include <map>
#include <vector>

#include <iostream>
#include <direct.h>   // _getcwd
#include <cstdio>     // perror
#include <limits.h>   // _MAX_PATH

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>   // translate, rotate, scale
#include <glm/gtc/type_ptr.hpp>           // value_ptr
#include <glm/gtx/string_cast.hpp> 

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace ge::gl;

#include "objLoader.h"
#include "matrix.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
/*
cgltf_options options = {0};
cgltf_data* data = NULL;
cgltf_result result = cgltf_parse_file(&options, "scene.gltf", &data);
if (result == cgltf_result_success)
{
	// TODO make awesome stuff 
	cgltf_free(data);
} */

//////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           La Pièce 
//////////////////////////////////////////////////////////////////////////////////////////////////////

// Dimensions de la pièce victorienne
const float ROOM_WIDTH = 6.0f;
const float ROOM_HEIGHT = 3.5f;
const float ROOM_DEPTH = 8.0f;

struct Vertex {
    float position[3]; // loc 0
    float normal[3];   // loc 1
    float uv[2];       // loc 2 (Coordonnées de texture)
};

// Flamme Billboard
GLuint flameProgram; // Programme de shader pour la flamme
GLuint flameQuadVao; // VAO pour le quad du billboard

GLuint createShader(GLenum type, std::string const& src) {
    auto shader = glCreateShader(type);
    char const* srcs[] = { src.c_str() };
    glShaderSource(shader, 1, srcs, 0);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char buf[10000];
        glGetShaderInfoLog(shader, 10000, 0, buf);
        std::cerr << "ERROR: " << buf << std::endl;
    }
    return shader;
}

GLuint createProgram(std::vector<GLuint> const& shaders) {
    auto prg = glCreateProgram();
    for (auto const& x : shaders)
        glAttachShader(prg, x);
    glLinkProgram(prg);

    GLint status;
    glGetProgramiv(prg, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        char buf[10000];
        glGetProgramInfoLog(prg, 10000, 0, buf);
        std::cerr << "ERROR: " << buf << std::endl;
    }
    return prg;
}

void generateRoomGeometry(std::vector<float>& vertices, std::vector<uint32_t>* indices, bool isRoom) {
    float hw = ROOM_WIDTH / 2.0f;
    float hh = ROOM_HEIGHT;
    float hd = ROOM_DEPTH / 2.0f;

    uint32_t idx = 0;
    vertices.clear(); // On vide les vecteurs au cas où
    if (indices) indices->clear();

    // Tool function
    auto addQuad = [&](float x1, float y1, float z1, float x2, float y2, float z2,
                       float x3, float y3, float z3, float x4, float y4, float z4,
                       float nx, float ny, float nz, float uMax, float vMax, float materialID) {
        // Données des sommets
        float q[] = {
            x1, y1, z1, nx, ny, nz, 0.0f, 0.0f, materialID,
            x2, y2, z2, nx, ny, nz, uMax, 0.0f, materialID,
            x3, y3, z3, nx, ny, nz, uMax, vMax, materialID,
            x4, y4, z4, nx, ny, nz, 0.0f, vMax, materialID
        };
        for(int i=0; i<36; ++i) vertices.push_back(q[i]);
        if (indices) {
            indices->push_back(idx); indices->push_back(idx + 1); indices->push_back(idx + 2);
            indices->push_back(idx); indices->push_back(idx + 2); indices->push_back(idx + 3);
            idx += 4;
        }
    };

    // Facteur d'échelle du papier peint 
    float scale = 0.5f; 
    
    // =======================================================
    // --- Définition des dimensions de la fenêtre (2.0m x 2.0m, centrée) ---
    // =======================================================
    float winWidth  = 2.0f;
    float winHeight = 2.0f;  
    float winYCenter = 1.75f;                   
    float winYMin   = winYCenter - winHeight * 0.5f; // 0.75m
    float winYMax   = winYCenter + winHeight * 0.5f; // 2.75m
    
    float winZMin   = 0.0f - winWidth * 0.5f;   // -1.0m
    float winZMax   = 0.0f + winWidth * 0.5f;   // 1.0m
    float windowX = hw;                         // Mur droit (X=hw)
    float offset = 0.005f;                      

    if (isRoom) {
        // =======================================================
        // --- Murs et Fenêtre ---
        // =======================================================
        // Mur du fond (Mur z=-hd)
        addQuad(-hw, 0, -hd, hw, 0, -hd, hw, hh, -hd, -hw, hh, -hd, 0, 0, 1, ROOM_WIDTH * scale, ROOM_HEIGHT * scale, 0);

        // Mur gauche (Mur x=-hw)
        addQuad(-hw, 0, hd, -hw, 0, -hd, -hw, hh, -hd, -hw, hh, hd, 1, 0, 0, ROOM_DEPTH * scale, ROOM_HEIGHT * scale, 0);

        // Mur avant (Mur z=hd)
        addQuad(hw, 0, hd, -hw, 0, hd, -hw, hh, hd, hw, hh, hd,  0, 0, -1, ROOM_WIDTH * scale, ROOM_HEIGHT * scale, 0); 
        
        // ----------------------------------------------------------------------------------
        // MUR DROIT (X = hw) AVEC OUVERTURE DE FENÊTRE
        // ----------------------------------------------------------------------------------
        float NX = -1.0f; // Normale pointant vers l'intérieur
        
        // 1. Partie Basse (Sous la fenêtre)
        addQuad(windowX, 0, -hd, windowX, 0, hd, windowX, winYMin, hd, windowX, winYMin, -hd, NX, 0, 0, ROOM_DEPTH * scale, winYMin * scale, 0);
        
        // 2. Partie Haute (Au-dessus de la fenêtre)
        addQuad(windowX, winYMax, -hd, windowX, winYMax, hd, windowX, hh, hd, windowX, hh, -hd, NX, 0, 0, ROOM_DEPTH * scale, (hh - winYMax) * scale, 0);

        // 3. Partie Arrière (Entre z=-hd et z=winZMin, de y=winYMin à y=winYMax)
        addQuad(windowX, winYMin, -hd, windowX, winYMin, winZMin, windowX, winYMax, winZMin, windowX, winYMax, -hd, NX, 0, 0, (winZMin - (-hd)) * scale, winHeight * scale, 0);

        // 4. Partie Avant (Entre z=winZMax et z=hd, de y=winYMin à y=winYMax)
        addQuad(windowX, winYMin, winZMax, windowX, winYMin, hd, windowX, winYMax, hd, windowX, winYMax, winZMax, NX, 0, 0, (hd - winZMax) * scale, winHeight * scale, 0);

        // ----------------------------------------------------------------------------------
        // VITRE DE LA FENÊTRE (MATERIAL ID 6)
        // ----------------------------------------------------------------------------------
        // float x_vitre = windowX - offset; // Légèrement à l'intérieur du mur
        // float NV = 1.0f; // Normale (1, 0, 0)
        // addQuad( x_vitre, winYMin, winZMax, x_vitre, winYMin, winZMin, x_vitre, winYMax, winZMin, x_vitre, winYMax, winZMax, NV, 0, 0, 1.0f, 1.0f, 6.0f); 

        // ----------------------------------------------------------------------------------
        // RAYONS DE SOLEIL (GOD RAYS) - GEOMETRIE (MATERIAL ID 7)
        // ----------------------------------------------------------------------------------
        // REMOVED
        
        // =======================================================
        // --- Sol, Plafond, Plinthes, Corniches et Porte ---
        // =======================================================

        // Sol
        addQuad(-hw, 0,  hd,  hw, 0,  hd, hw, 0, -hd,  -hw, 0, -hd,  0, 1, 0, ROOM_WIDTH, ROOM_DEPTH, 2 );            

        // plafond
        addQuad(-hw, hh, -hd,  hw, hh, -hd,  hw, hh,  hd,  -hw, hh,  hd, 0, -1, 0,  ROOM_WIDTH, ROOM_DEPTH, 0 );

        float ph = 0.15f; 

        // Plinthe Mur du fond
        addQuad(-hw, 0, -hd+0.01f, hw, 0, -hd+0.01f, hw, ph, -hd+0.01f, -hw, ph, -hd+0.01f, 0,0,1, 1,0.2, 3);

        // Plinthe Mur gauche
        addQuad(-hw+0.01f, 0, hd, -hw+0.01f, 0, -hd, -hw+0.01f, ph, -hd, -hw+0.01f, ph, hd, 1,0,0, 1,0.2, 3);

        // Plinthe Mur droit (Plinthe coupée)
        if (ph < winYMin) {
            addQuad(hw-0.01f, 0, -hd, hw-0.01f, 0, hd, hw-0.01f, ph, hd, hw-0.01f, ph, -hd, -1,0,0, 1,0.2, 3);
        }
        
        // Plinthe Mur avant
        addQuad(hw, 0, hd-0.01f, -hw, 0, hd-0.01f, -hw, ph, hd-0.01f, hw, ph, hd-0.01f, 0,0,-1, 1,0.2, 3);

        float ch = 0.20f;
        float cy = hh - ch;

        // Corniche Fond
        addQuad(-hw, cy, -hd+0.01f, hw, cy, -hd+0.01f, hw, hh, -hd+0.01f, -hw, hh, -hd+0.01f, 0,0,1, 1,0.3, 4);

        // Corniche Gauche
        addQuad(-hw+0.01f, cy, hd, -hw+0.01f, cy, -hd, -hw+0.01f, hh, -hd, -hw+0.01f, hh, hd, 1,0,0, 1,0.3, 4);

        // Corniche Droite (coupée)
        if (cy > winYMax) {
            addQuad(hw-0.01f, cy, -hd, hw-0.01f, cy, hd, hw-0.01f, hh, hd, hw-0.01f, hh, -hd, -1,0,0, 1,0.3, 4);
        }

        // Corniche Avant
        addQuad(hw, cy, hd-0.01f, -hw, cy, hd-0.01f, -hw, hh, hd-0.01f, hw, hh, hd-0.01f, 0,0,-1, 1,0.3, 4);

        // Porte (sur le Mur gauche: x = -hw)
        float doorWidth  = 0.9f;
        float doorHeight = 2.8f;  
        float doorBottom = 0.0f;                    
        float doorCenterZ = 0.0f;                   
        float x = -hw;                              
        float z1 = doorCenterZ - doorWidth * 0.5f;
        float z2 = doorCenterZ + doorWidth * 0.5f;
        float y1 = doorBottom;
        float y2 = doorBottom + doorHeight;
        float offset_door = 0.015f; 
        float xd = x + offset_door;
        addQuad( xd, y1, z2,   xd, y1, z1,   xd, y2, z1,  xd, y2, z2,  1, 0, 0,  1.0f, 1.0f, 5 );
    }
    else {
        // ----------------------------------------------------------------------------------
        // VITRE DE LA FENÊTRE (MATERIAL ID 6)
        // ----------------------------------------------------------------------------------
        float x_vitre = windowX - offset; // Légèrement à l'intérieur du mur
        float NV = 1.0f; // Normale (1, 0, 0)
        addQuad( x_vitre, winYMin, winZMax, x_vitre, winYMin, winZMin, x_vitre, winYMax, winZMin, x_vitre, winYMax, winZMax, NV, 0, 0, 1.0f, 1.0f, 6.0f); 
        // Les rayons doivent traverser la fenêtre
    }
}

// ============================================================================
// La fumée de la pipe - VERSION AMÉLIORÉE AVEC VOLUTES
// ============================================================================
#include "smoke.h"

// ============================================================================
// La flamme de la bougie
// ============================================================================
#include "flame.h"

// ============================================================================
// Shadow Mapping
// ============================================================================
#include "shadowMapping.h"


// ============================================================================
// Draw the room & the objects
// ============================================================================
#include "scene.h"


int main(int argc, char* argv[]) {
    int winWidth  = 1920;  
    int winHeight = 1080;  
    auto window = SDL_CreateWindow("Sherlock Holmes' Room", winWidth, winHeight, SDL_WINDOW_OPENGL);
    auto context = SDL_GL_CreateContext(window);
    float globalBrightness = 3.0f; // 1.0 = normal, 0.0 = noir
    float localBrightness = 1.0f; // 1.0 = normal, 0.0 = noir

    ge::gl::init();

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback([](GLenum src, GLenum type, GLuint id, GLenum sev, GLsizei len, const GLchar* msg, const void*) { 
        std::cerr << "GL DEBUG: " << msg << " (id=" << id << ")\n";  
    }, nullptr);

    // Génération de la géométrie de la pièce
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    generateRoomGeometry(vertices, &indices, true);

    // La fenêtre doit être traitée séparément: elle ne doit pas être affichée en Passe 1 pour laisser passer la lumière
    std::vector<float> windowVertices; 
    std::vector<uint32_t> windowIndices;
    generateRoomGeometry(windowVertices, &windowIndices, false);

    // Création des buffers
    GLuint vbo;
    glCreateBuffers(1, &vbo);
    glNamedBufferData(vbo, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    GLuint ebo;
    glCreateBuffers(1, &ebo);
    glNamedBufferData(ebo, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // Configuration du VAO de la pièce
    GLuint vao;
    glCreateVertexArrays(1, &vao);
    
    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);

    glEnableVertexArrayAttrib(vao, 1);
    glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, sizeof(float)*3);
    glVertexArrayAttribBinding(vao, 1, 0);

    glEnableVertexArrayAttrib(vao, 2);
    glVertexArrayAttribFormat(vao, 2, 2, GL_FLOAT, GL_FALSE, sizeof(float)*6);
    glVertexArrayAttribBinding(vao, 2, 0);

    glEnableVertexArrayAttrib(vao, 3);
    glVertexArrayAttribFormat(vao, 3, 1, GL_FLOAT, GL_FALSE, sizeof(float)*8);
    glVertexArrayAttribBinding(vao, 3, 0);

    glVertexArrayElementBuffer(vao, ebo);
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(float)*9);

    // --- CRÉATION DU WINDOW VAO ---
    // Buffers spécifiques à la fenêtre
    GLuint windowVao, windowVbo, windowEbo;
    glCreateBuffers(1, &windowVbo);
    glNamedBufferData(windowVbo, windowVertices.size() * sizeof(float), windowVertices.data(), GL_STATIC_DRAW);
    glCreateBuffers(1, &windowEbo);
    glNamedBufferData(windowEbo, windowIndices.size() * sizeof(uint32_t), windowIndices.data(), GL_STATIC_DRAW);
    // CONFIGURATION DU WINDOW VAO
    glCreateVertexArrays(1, &windowVao);
    // On lie bien le VBO de la fenêtre
    glVertexArrayVertexBuffer(windowVao, 0, windowVbo, 0, sizeof(float) * 9);
    glVertexArrayElementBuffer(windowVao, windowEbo);
    // Les attributs doivent être configurés explicitement pour ce VAO
    for(int i = 0; i <= 3; ++i) glEnableVertexArrayAttrib(windowVao, i);
    glVertexArrayAttribFormat(windowVao, 0, 3, GL_FLOAT, GL_FALSE, 0); // Pos
    glVertexArrayAttribFormat(windowVao, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float)); // Norm
    glVertexArrayAttribFormat(windowVao, 2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float)); // UV
    glVertexArrayAttribFormat(windowVao, 3, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float)); // MatID
    for(int i = 0; i <= 3; ++i) glVertexArrayAttribBinding(windowVao, i, 0);
    
    // Shaders
    auto vsSrc = R".(
      #version 460
      layout(location=0) in vec3 position;
      layout(location=1) in vec3 normal;
      layout(location=2) in vec2 uv; 
      layout(location=3) in float materialID;

      out vec3 vNormal;
      out vec2 vUV;
      out vec3 vPosition;
      out float vMatID;

      uniform mat4 viewMatrix = mat4(1);
      uniform mat4 projMatrix = mat4(1);
      uniform mat4 model = mat4(1);

      // Shadow mapping
      out vec4 vFragPosLightSpace;
      uniform mat4 lightSpaceMatrix;      

      void main() {
          gl_Position = projMatrix * viewMatrix * model * vec4(position, 1);
          
          mat3 normalMatrix = mat3(transpose(inverse(model)));
          vec3 transformedNormal = normalize(normalMatrix * normal);
          vNormal = transformedNormal;
          
          vUV = uv;
          vPosition = vec3(model * vec4(position, 1));
          vMatID = materialID; 

          // Shadow mapping
          vFragPosLightSpace = lightSpaceMatrix * model * vec4(position, 1.0);          
      }
     ).";

    auto fsSrc = R".(
      #version 460
      in vec3 vNormal;
      in vec2 vUV;
      in vec3 vPosition;
      in float vMatID;
      out vec4 fColor;
      
      uniform vec3 sunDirection = normalize(vec3(1.0, -0.5, 0.0)); 
      uniform vec3 cameraPosition; 
      uniform sampler2D materialTex[32];
      uniform bool debugMode = false;
      uniform bool showNormals = false;
      uniform bool showUVs = false;
      
      uniform vec3 materialKd[32];       // Couleurs Kd
      uniform bool materialHasTex[32];   // Flag texture

      uniform int renderPass = 0;     // renderPass: 0=Opaque, 1=Transparent
      uniform vec3 sunPositionWorld;  // Position d'origine du rayon (centre fenêtre)
      uniform float beamWidthZ = 1.0;

      // La flamme
      in vec2 TexCoord;
      uniform sampler2D flameTexture;
      // out vec4 fragColor; // GRRRRrrr....

      // POINT LIGHT éclairage à la bougie
      struct PointLight {
            vec3 position;
            vec3 color;
            // x = Constant, y = Linear, z = Quadratic
            vec3 attenuation; 
      };
      uniform PointLight pointLight;
      uniform vec3 viewPos; // Position de la caméra (pour les reflets spéculaires)
      
      uniform float globalBrightness = 1.0;
      uniform float localBrightness = 1.0;
      uniform vec3 sunColor = vec3(1.0, 0.95, 0.8); // Lumière chaude
      uniform vec3 ambientColor = vec3(0.2, 0.2, 0.3); // Ambiance bleutée froide

      // Shadow mapping
      uniform sampler2D shadowMap;
      in vec4 vFragPosLightSpace;

      // 16 points répartis selon un disque de Poisson
      const vec2 poissonDisk[16] = vec2[]( 
        vec2( -0.94201624, -0.39906216 ), vec2( 0.94558609, -0.76890725 ), 
        vec2( -0.094184101, -0.92938870 ), vec2( 0.34495938, 0.29387760 ), 
        vec2( -0.91588581, 0.45771432 ), vec2( -0.81544232, -0.87912464 ), 
        vec2( -0.38277543, 0.27676845 ), vec2( 0.97484398, 0.75648379 ), 
        vec2( 0.44323325, -0.97511554 ), vec2( 0.53742981, -0.47373420 ), 
        vec2( -0.26496911, -0.41893023 ), vec2( 0.79197514, 0.19090188 ), 
        vec2( -0.24188840, 0.99706507 ), vec2( -0.81409955, 0.91437590 ), 
        vec2( 0.19984126, 0.78641367 ), vec2( 0.14383161, -0.14100790 ) 
      );

      // Fonction de bruit rapide pour la rotation
      float random(vec4 seed4) {
        float dot_product = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
        return fract(sin(dot_product) * 43758.5453);
      }

      float calculateShadow() {
        vec3 projCoords = vFragPosLightSpace.xyz / vFragPosLightSpace.w;
        projCoords = projCoords * 0.5 + 0.5;
        if(projCoords.z > 1.0) return 0.0;

        float bias = max(0.005 * (1.0 - dot(normalize(vNormal), normalize(-sunDirection))), 0.0005);
        float shadow = 0.0;
        
        // Rayon de flou (à ajuster selon vos goûts) : Plus le chiffre est grand, plus l'ombre est "douce"
        float spread = 1.5 / textureSize(shadowMap, 0).x;

        // Utiliser la position du fragment pour créer un angle de rotation aléatoire
        float angle = random(vec4(vPosition, 0.0)) * 6.283185; // 0 à 2PI
        float s = sin(angle);
        float c = cos(angle);
        mat2 rotation = mat2(c, s, -s, c);

        for (int i=0; i<16; i++) {
            // Faire pivoter le point du disque de Poisson
            vec2 offset = rotation * poissonDisk[i] * spread;
            float pcfDepth = texture(shadowMap, projCoords.xy + offset).r;
            if (projCoords.z - bias > pcfDepth) shadow += 1.0;
        }
        return shadow / 16.0;
      }

      void main() {
          int mid = int(vMatID + 0.5);

          // ------------------------------------------------------------------
          // CONTRÔLE DU PASSAGE DE RENDU (OPAQUE vs TRANSPARENT)
          // ------------------------------------------------------------------
          // MatID 6 = Vitre, MatID 7 = Rayons
          bool isTransparent = (mid == 6 || mid == 7);
          
          if (renderPass == 0) { // PASSAGE OPAQUE (Phase 1)
              // Jeter la vitre (6) et les rayons (7). Garde les murs (0-5) et les OBJs (8+)
              if (isTransparent) { discard; } 
          } else { // PASSAGE TRANSPARENT (Phase 2)
              // Garder uniquement la vitre (6) et les rayons (7). Jeter les murs et les OBJs
              if (!isTransparent) { discard; } 
          }
          // ------------------------------------------------------------------

          // UV DEBUG MODE
          if (showUVs) { fColor = vec4(fract(vUV), 0.0, 1.0); return; }
          
          // NORMAL DEBUG MODE
          if (showNormals) { fColor = vec4(vNormal * 0.5 + 0.5, 1.0); return; }
          
          // MATERIAL ID DEBUG MODE
          if (debugMode) {
              if (vMatID < -0.5) { 
                fColor = vec4(1.0, 0.0, 1.0, 1.0);
              } else if (vMatID < 0.5) {
                fColor = vec4(1.0, 0.0, 0.0, 1.0);
              } else if (vMatID < 3.5) {
                fColor = vec4(0.0, vMatID / 3.0, 0.0, 1.0);
              } else if (vMatID < 6.5) {
                fColor = vec4(0.0, 0.0, (vMatID - 3.0) / 3.0, 1.0);
              } else if (vMatID < 7.5) { // MatID 7 (Rayons) : Jaune
                fColor = vec4(1.0, 1.0, 0.0, 1.0); return; 
              } else {
                fColor = vec4(1.0, 1.0, 0.0, 1.0); // YELLOW for table (OBJ)
              }
              return;
          }
          
          // ===== RÉCUPÉRATION COULEUR MATÉRIAU =====
          vec3 texColor;
          if (mid >= 0 && mid < 32) {
              if (materialHasTex[mid]) { // Utiliser la texture
                  texColor = texture(materialTex[mid], vUV).rgb;
              } else { // Utiliser la couleur Kd
                  texColor = materialKd[mid];
              }
          } else {
              texColor = vec3(1.0, 0.0, 1.0); // Magenta debug
          }

          // --- LIGHTING pour les objets opaques (Murs, Sol, OBJs) ---
          float shadow = calculateShadow();
          vec3 lightDir = normalize(-sunDirection);
          vec3 ambient = ambientColor * 0.3 * globalBrightness;
          vec3 diffuse = sunColor * max(dot(normalize(vNormal), lightDir), 0.0) * 0.8;
          vec3 lighting = vec3(ambient + (1.0 - shadow) * diffuse);  // L'ombre ne coupe que la diffuse

          vec3 finalColor = texColor * lighting; 

          // ==========================================================
          // Calcul de l'éclairage de la Bougie (Point Light) - AJOUTER ICI
          // ==========================================================
          // On s'assure que le calcul n'affecte pas les rayons de soleil (MatID 7)
          if (mid != 7) { 
            // 1. Initialisation des composantes
            vec3 norm = normalize(vNormal);
            vec3 lightContribution = vec3(0.0);
            
            // 2. Calcul du vecteur Lumière et de la distance
            vec3 lightVec = pointLight.position - vPosition;
            float distance = length(lightVec);
            vec3 lightDirNorm = normalize(lightVec);

            // 3. Atténuation (Falloff)
            float attenuation = 1.0 / (
                pointLight.attenuation.x + 
                pointLight.attenuation.y * distance + 
                pointLight.attenuation.z * distance * distance
            );
            
            // 4. Composante Diffuse
            float diff = max(dot(norm, lightDirNorm), 0.0);
            vec3 diffuseLight = pointLight.color * diff * texColor;
            
            // 5. Composante Spéculaire (Optionnelle, pour les surfaces brillantes comme le bougeoir)
            // Pour simplifier, nous n'incluons que le diffuse pour la lumière de la bougie.
            
            // 6. Application de l'atténuation
            lightContribution = diffuseLight * attenuation * localBrightness;

            // 7. AJOUTER LA CONTRIBUTION DE LA BOUGIE à la couleur finale
            finalColor += lightContribution;
          }
          // ==========================================================          

          // ==========================================================
          // PROJECTION DE LUMIÈRE DES RAYONS (Uniquement sur le Sol)
          // ==========================================================
          // Supprimé: La lumière doit éclairer le sol
          // ==========================================================

          fColor = vec4(finalColor, 1.0);
        }
    ).";

const char* vsDepth = R"(
#version 460
layout(location=0) in vec3 position;
uniform mat4 lightSpaceMatrix;
uniform mat4 model;
void main() {
    gl_Position = lightSpaceMatrix * model * vec4(position, 1.0);
}
)";

const char* fsDepth = R"(
#version 460
void main() {} // Rien à faire, OpenGL écrit la profondeur tout seul
)";    

    auto vs = createShader(GL_VERTEX_SHADER, vsSrc);
    auto fs = createShader(GL_FRAGMENT_SHADER, fsSrc);
    auto prg = createProgram({ vs, fs });

    auto viewMatrixL = glGetUniformLocation(prg, "viewMatrix");
    auto projMatrixL = glGetUniformLocation(prg, "projMatrix");
    GLint locSunDir = glGetUniformLocation(prg, "sunDirection"); 
    GLint locCameraPos = glGetUniformLocation(prg, "cameraPosition"); 
    GLint locRenderPass = glGetUniformLocation(prg, "renderPass"); // Déclaration ajoutée

    //  Compiler les shaders de fumée
    auto smokeVS = createShader(GL_VERTEX_SHADER, smokeVertexShader);
    auto smokeFS = createShader(GL_FRAGMENT_SHADER, smokeFragmentShader);
    auto smokeProgram = createProgram({smokeVS, smokeFS});

    // Créer les shaders de flamme
    auto flameVS = createShader(GL_VERTEX_SHADER, flameVertexShader);
    auto flameFS = createShader(GL_FRAGMENT_SHADER, flameFragmentShader);
    flameProgram = createProgram({flameVS, flameFS});

    // Initialiser le quad de flamme
    GLuint flameVBO;
    initFlameQuad(flameQuadVao, flameVBO);
    std::cout << "Flame billboard initialized" << std::endl;

    // Camera setup
    float cameraPosition[3] = { 0, 1.7f, 0.0f };
    float angleX = 0.f;
    float angleY = 0.f;

    // Light setup
    float dimmer = 0.005f;

    float viewMatrix[16], VT[16], VR[16], VRX[16], VRY[16], projMatrix[16];
    matrixIdentity(projMatrix);

    float rotationSpeed = 0.004f;
    float moveSpeed = 0.009f;

    std::map<int, bool> keys;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Chargement texture
    auto loadTex = [&](const char* a, const char* b, const char* c){
        const char* paths[] = { a, b, c };
        return loadTexture(paths, 3);
    };

    GLuint wallTex   = loadTex("../img/papierpeint.jpg",   "./img/papierpeint.jpg",   "../../img/papierpeint.jpg");
    GLuint floorTex  = loadTex("../img/parquetbois.jpg",   "./img/parquetbois.jpg",   "../../img/parquetbois.jpg");
    // GLuint ceilTex   = loadTex("../img/plafondpeint.jpg",  "./img/plafondpeint.jpg",  "../../img/plafondpeint.jpg");
    // GLuint ceilTex   = loadTex("../img/blanc.jpg",  "./img/blanc.jpg",  "../../img/blanc.jpg");
    GLuint plinthTex = loadTex("../img/plinthe.jpg",       "./img/plinthe.jpg",       "../../img/plinthe.jpg");
    GLuint stucTex   = loadTex("../img/stuc.jpg",          "./img/stuc.jpg",          "../../img/stuc.jpg");
    GLuint doorTex   = loadTex("../img/portev.png",        "./img/portev.png",        "../../img/portev.png");
    // GLuint glassTex  = loadTex("../img/glass.png",         "./img/glass.png",         "../../img/glass.png");
    GLuint windowTex  = loadTex("../img/window1024.png",   "./img/window1024.png",    "../../img/window1024.png");
    GLuint flameTex  = loadTex("../img/flame.png",   "./img/flame.png",    "../../img/flame.png");

    glUseProgram(prg);

    GLint materialTextures[32];
    for(int i = 0; i < 32; i++) materialTextures[i] = i;
    
    GLint locMaterialTex = glGetUniformLocation(prg, "materialTex");
    if(locMaterialTex >= 0) {
        glProgramUniform1iv(prg, locMaterialTex, 32, materialTextures);
        std::cout << "Initialized materialTex uniform array" << std::endl;
    }

    // Load table
    const int baseUnit = 9;
    int matCount = 0;
    OBJMesh table;
    char *objPath = "../obj/old_table.obj";
    if (!loadOBJ(objPath, table, baseUnit)) {
        std::cerr << "Failed to load "<< objPath << std::endl;
    } else {
        std::cout << "Table loaded: " << table.count << " vertices, " 
                  << table.materialTextures.size() << " materials" << std::endl;
        for(int i = 0; i < (int)table.materialTextures.size(); i++) {
            std::cout << "  Material " << i << " (" << table.materialNames[i] << "): texID=" << table.materialTextures[i] << std::endl;
        }
    }
    matCount += table.materialTextures.size();

    // Chargement du Cadre
    OBJMesh frame;
    char *objPathFrame = "../obj/SM_frame_01.obj";
    if (!loadOBJ(objPathFrame, frame, baseUnit+matCount)) {
        std::cerr << "Failed to load "<< objPathFrame << std::endl;
    } else {
        std::cout << "Frame loaded: " << frame.count << " vertices, " << frame.materialTextures.size() << " materials" << std::endl;
        for(int i = 0; i < (int)frame.materialTextures.size(); i++) {
            std::cout << "  Material " << i << " (" << frame.materialNames[i] << "): texID=" << frame.materialTextures[i] << std::endl;
        }
    }
    matCount += frame.materialTextures.size();

    // Chargement du Cendrier
    OBJMesh ashtray;
    char *objPathAshtray = "../obj/objCigarrete.obj";
    // char *objPathAshtray = "../obj/Ashtray.obj";
    if (!loadOBJ(objPathAshtray, ashtray, baseUnit+matCount)) {
        std::cerr << "Failed to load "<< objPathAshtray << std::endl;
    } else {
        std::cout << "Ashtray loaded: " << ashtray.count << " vertices, " << ashtray.materialTextures.size() << " materials" << std::endl;
        for(int i = 0; i < (int)ashtray.materialTextures.size(); i++) {
            std::cout << "  Material " << i << " (" << ashtray.materialNames[i] << "): texID=" << ashtray.materialTextures[i] << std::endl;
        }
    }
    matCount += ashtray.materialTextures.size();

    // Chargement de la pipe
    OBJMesh pipe;
    char *objPathPipe = "../obj/Pipe.obj";
    if (!loadOBJ(objPathPipe, pipe, baseUnit+matCount)) {
        std::cerr << "Failed to load "<< objPathPipe << std::endl;
    } else {
        std::cout << "Pipe loaded: " << pipe.count << " vertices, " << pipe.materialTextures.size() << " materials" << std::endl;
        for(int i = 0; i < (int)pipe.materialTextures.size(); i++) {
            std::cout << "  Material " << i << " (" << pipe.materialNames[i] << "): texID=" << pipe.materialTextures[i] << std::endl;
        }
    }
    matCount += pipe.materialTextures.size();
    
    // Chargement du canapé
    OBJMesh couch;
    char *opjPathCouch = "../obj/couch1.obj";
    if (!loadOBJ(opjPathCouch, couch, baseUnit+matCount)) {
        std::cerr << "Failed to load "<< opjPathCouch << std::endl;
    } else {
        std::cout << "Couch loaded: " << couch.count << " vertices, " << couch.materialTextures.size() << " materials" << std::endl;
        for(int i = 0; i < (int)couch.materialTextures.size(); i++) {
            std::cout << "  Material " << i << " (" << couch.materialNames[i] << "): texID=" << couch.materialTextures[i] << std::endl;
        }
    }
    matCount += couch.materialTextures.size();

    // Chargement de la cheminée
    OBJMesh fireplace;
    char *objPathFireplace = "../obj/fireplace.obj";
    if (!loadOBJ(objPathFireplace, fireplace, baseUnit+matCount)) {
        std::cerr << "Failed to load "<< objPathFireplace << std::endl;
    } else {
        std::cout << "Fireplace loaded: " << fireplace.count << " vertices, " << fireplace.materialTextures.size() << " materials" << std::endl;
        for(int i = 0; i < (int)fireplace.materialTextures.size(); i++) {
            std::cout << "  Material " << i << " (" << fireplace.materialNames[i] << "): texID=" << fireplace.materialTextures[i] << std::endl;
        }
    }
    matCount += fireplace.materialTextures.size();

    // Chargement de la bougie
    OBJMesh candle;
    char *objPathCandle = "../obj/candle.obj";
    if (!loadOBJ(objPathCandle, candle, baseUnit+matCount)) {
        std::cerr << "Failed to load "<< objPathCandle << std::endl;
    } else {
        std::cout << "Candle loaded: " << candle.count << " vertices, " << candle.materialTextures.size() << " materials" << std::endl;
        for(int i = 0; i < (int)candle.materialTextures.size(); i++) {
            std::cout << "  Material " << i << " (" << candle.materialNames[i] << "): texID=" << candle.materialTextures[i] << std::endl;
        }
    }
    matCount += candle.materialTextures.size();

    GLuint smokeTex = createSmokeTexture();
    // ====> DEBUG
    std::cout << "Smoke texture created with ID: " << smokeTex << std::endl;
    glBindTextureUnit(0, smokeTex);
    GLint locSmokeTex = glGetUniformLocation(smokeProgram, "smokeTexture");
    if (locSmokeTex >= 0) {
        glUniform1i(locSmokeTex, 0);
        std::cout << "Smoke texture bound to unit 0" << std::endl;
    } else {
        std::cerr << "ERROR: smokeTexture uniform not found!" << std::endl;
    }
    // <==== DEBUG

    // Les objets
    SimpleObj sTable = {table.vao, (int)table.count};
    SimpleObj sFrame = {frame.vao, (int)frame.count};
    SimpleObj sAshtray = {ashtray.vao, (int)ashtray.count};
    SimpleObj sPipe = {pipe.vao, (int)pipe.count};
    SimpleObj sCouch = {couch.vao, (int)couch.count};
    SimpleObj sFireplace = {fireplace.vao, (int)fireplace.count};
    SimpleObj sCandle = {candle.vao, (int)candle.count};

    // Créer l'émetteur (positionné au bout du cigare)
    glm::vec3 cigarTipPosition = glm::vec3(0.025f, 1.06f, -2.0f); 
    SmokeEmitter smokeEmitter(cigarTipPosition);

    bool running = true;
    bool printedOnce = false;
    GLint locSunPosWorld = glGetUniformLocation(prg, "sunPositionWorld");

    // ====================================================================
    // Initialisation du Shadow mapping
    // ====================================================================
    ShadowMapping shadow;
    shadow.init();
    // Compilation des shaders de profondeur
    auto vsD = std::make_shared<Shader>(GL_VERTEX_SHADER, vsDepth);
    auto fsD = std::make_shared<Shader>(GL_FRAGMENT_SHADER, fsDepth);
    auto depthProgram = std::make_shared<Program>(vsD, fsD);
    // Récupérer les locations pour pouvoir les utiliser plus tard
    GLint locDepthLightSpace = glGetUniformLocation(depthProgram->getId(), "lightSpaceMatrix");
    GLint locDepthModel = glGetUniformLocation(depthProgram->getId(), "model");

    // ====================================================================
    // Boucle d'affichage / redering
    // ====================================================================
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_KEY_UP) keys[event.key.key] = false;
            if (event.type == SDL_EVENT_KEY_DOWN) keys[event.key.key] = true;
        }//while

        if (keys[SDLK_ESCAPE]) running = false;

        // Camera controls
        if (keys[SDLK_LEFT]) angleY += rotationSpeed;
        if (keys[SDLK_RIGHT]) angleY -= rotationSpeed;
        if (keys[SDLK_UP]) angleX += rotationSpeed;
        if (keys[SDLK_DOWN]) angleX -= rotationSpeed;

        if (angleX > 1.5f) angleX = 1.5f;
        if (angleX < -1.5f) angleX = -1.5f;

        rotateX(VRX, angleX);
        rotateY(VRY, angleY);
        matrixMultiplication(VR, VRX, VRY);
    
        float forwardBackward = 0.0f;
        float leftRight = 0.0f;

        if (keys[SDLK_W] || keys[SDLK_Z]) forwardBackward -= moveSpeed;
        if (keys[SDLK_S]) forwardBackward += moveSpeed;
        if (keys[SDLK_A] || keys[SDLK_Q]) leftRight -= moveSpeed;
        if (keys[SDLK_D] || keys[SDLK_E]) leftRight += moveSpeed;
        if (keys[SDLK_X]) cameraPosition[1] += moveSpeed; // Monter (X)
        if (keys[SDLK_C]) cameraPosition[1] -= moveSpeed; // Descendre (C)

        cameraPosition[0] += VR[2] * forwardBackward;
        cameraPosition[1] += VR[6] * forwardBackward;
        cameraPosition[2] += VR[10] * forwardBackward;

        cameraPosition[0] += VR[0] * leftRight;
        cameraPosition[1] += VR[4] * leftRight;
        cameraPosition[2] += VR[8] * leftRight;
        
        if (keys[SDLK_SPACE]) cameraPosition[1] += moveSpeed;
        if (keys[SDLK_LSHIFT]) cameraPosition[1] -= moveSpeed;

        if (keys[SDLK_R]) {
            cameraPosition[0] = 0.0f;
            cameraPosition[1] = 1.7f;
            cameraPosition[2] = 0.0f;
            angleX = 0.0f; 
            angleY = 0.0f;
        }

        translate(VT, -cameraPosition[0], -cameraPosition[1], -cameraPosition[2]);
        matrixMultiplication(viewMatrix, VR, VT);
        perspective(projMatrix, 1024.0f / 768.0f, 90.0f / 180.0f * 3.1415926f, 0.1f, 100.0f);

        // Light control
        if (keys[SDLK_O]) globalBrightness -= dimmer; 
        if (keys[SDLK_P]) globalBrightness += dimmer; 
        if (keys[SDLK_K]) localBrightness -= dimmer; 
        if (keys[SDLK_L]) localBrightness += dimmer;
        if (globalBrightness < 0.0f) {globalBrightness = 0.0f; std::cout << "globalBrightness = 0.0f" << std::endl;} // Noir complet
        if (globalBrightness > 4.0f) globalBrightness = 4.0f; // Cap à 400%
        if (localBrightness < 0.0f) {localBrightness = 0.0f; std::cout << "localBrightness = 0.0f" << std::endl;} // Noir complet
        if (localBrightness > 4.0f) localBrightness = 4.0f; // Cap à 400%

        // Render
        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Bind textures
        glBindTextureUnit(0, wallTex);
        // glBindTextureUnit(1, floorTex);
        glBindTextureUnit(2, floorTex);
        glBindTextureUnit(3, plinthTex);
        glBindTextureUnit(4, stucTex);
        glBindTextureUnit(5, doorTex);
        glBindTextureUnit(6, windowTex);
        
        int currentOffset = 0; // Utiliser un offset cumulé
        for(int i = 0; i < (int)table.materialTextures.size(); i++) 
            glBindTextureUnit(baseUnit + currentOffset + i, table.materialTextures[i]);
        currentOffset += (int) table.materialTextures.size(); // currentOffset = Taille Table
        
        for(int i = 0; i < (int)frame.materialTextures.size(); i++) 
            glBindTextureUnit(baseUnit + currentOffset + i, frame.materialTextures[i]);
        currentOffset += (int) frame.materialTextures.size(); // currentOffset = Taille Table + Taille Frame
        
        for(int i = 0; i < (int)ashtray.materialTextures.size(); i++) 
            glBindTextureUnit(baseUnit + currentOffset + i, ashtray.materialTextures[i]); 
        currentOffset += (int) ashtray.materialTextures.size(); // currentOffset = Taille Table + Taille Frame + Taille Ashtray
        
        for(int i = 0; i < (int)pipe.materialTextures.size(); i++) 
            glBindTextureUnit(baseUnit + currentOffset + i, pipe.materialTextures[i]);
        currentOffset += (int) pipe.materialTextures.size();

        for(int i = 0; i < (int)couch.materialTextures.size(); i++) 
            glBindTextureUnit(baseUnit + currentOffset + i, couch.materialTextures[i]);
        currentOffset += (int) couch.materialTextures.size();

        for(int i = 0; i < (int)fireplace.materialTextures.size(); i++) 
            glBindTextureUnit(baseUnit + currentOffset + i, fireplace.materialTextures[i]);
        currentOffset += (int) fireplace.materialTextures.size();
        
        for(int i = 0; i < (int)candle.materialTextures.size(); i++) 
            glBindTextureUnit(baseUnit + currentOffset + i, candle.materialTextures[i]);
        currentOffset += (int) candle.materialTextures.size();

        if(!printedOnce) {
            std::cout << "\nESC = Quit" << std::endl;
            std::cout << "\n=== DEBUG KEYS ===" << std::endl;
            std::cout << "N = MaterialID debug (table should be YELLOW)" << std::endl;
            std::cout << "M = Normal debug" << std::endl;
            std::cout << "U = UV debug" << std::endl;
            std::cout << "Normal = Actual render" << std::endl;
            printedOnce = true;
        }

        glUseProgram(prg);
        glProgramUniformMatrix4fv(prg, viewMatrixL, 1, GL_FALSE, viewMatrix);
        glProgramUniformMatrix4fv(prg, projMatrixL, 1, GL_FALSE, projMatrix);

        // Ajuster l'éclairage
        GLint gBrightness = glGetUniformLocation(prg, "globalBrightness");
        if (gBrightness >= 0) glUniform1f(gBrightness, globalBrightness);   
        GLint lBrightness = glGetUniformLocation(prg, "localBrightness");
        if (lBrightness >= 0) glUniform1f(lBrightness, localBrightness);   

        // ====================================================================
        // DÉBUT DE LA LOGIQUE D'ÉCLAIRAGE ET D'ANIMATION DE LA FLAMME (AJOUTÉ)
        // ====================================================================
        // Position de la flamme/lumière au-dessus de la bougie (constante)
        glm::vec3 flamePos = glm::vec3(-0.44f, 1.515f, -1.9f);
        float currentTime = SDL_GetTicks() / 1000.0f;
        const float BASE_SIZE = 0.12f;
        // La flamme varie en taille
        float sizeWobble = ( sin(currentTime * 1.5) * 0.5 + sin(currentTime * 3.7) * 0.2  ) * 0.10f;
        float currentFlameSize = BASE_SIZE * (1.0f + sizeWobble);

        // Calcul du Scintillement de l'Intensité Lumineuse 
        float intensityFlicker = 1.0f + 0.15f * sin(currentTime * 5.0f); // Variation entre 0.85 et 1.15
        float finalIntensity = intensityFlicker; // Intensité finale de la lumière ponctuelle
        glm::vec3 lightColor = glm::vec3(1.0f, 0.6f, 0.1f); // Orange-Jaune très chaud
        glm::vec3 finalLightColor = lightColor * finalIntensity * 0.6f; // Couleur finale incluant l'intensité du scintillement 

        // Paramètres d'atténuation (constants pour cette source)
        const float CONSTANT = 1.0f;
        // const float LINEAR = 7.0f;  
        // const float QUADRATIC = 18.0f;  // Trop agressif
        const float LINEAR = 2.0f;  
        const float QUADRATIC = 2.0f;
        // ====================================================================
        // FIN DE LA LOGIQUE D'ÉCLAIRAGE ET D'ANIMATION
        // ====================================================================

        // Render
        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Définition de l'origine de la lumière (Centre de la fenêtre)
        glm::vec3 sunPosOrigin = glm::vec3(10.0f, 2.0f, 3.5f); 
        // La direction doit pointer DEPUIS le soleil VERS le centre de la pièce
        glm::vec3 sunDir = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - sunPosOrigin);
        // glm::vec3 sunDir = glm::normalize(glm::vec3(-1.0f, 1.5f, -0.2f)); // Direction ajustée pour la projection au sol
        if(locSunDir >= 0) glProgramUniform3fv(prg, locSunDir, 1, glm::value_ptr(sunDir));
        if(locCameraPos >= 0) glProgramUniform3f(prg, locCameraPos, cameraPosition[0], cameraPosition[1], cameraPosition[2]);

        if(locSunPosWorld >= 0) glProgramUniform3fv(prg, locSunPosWorld, 1, glm::value_ptr(sunPosOrigin)); 

        // 1. Calcul de la matrice de lumière
        glm::mat4 lightSpaceMatrix = shadow.getLightSpaceMatrix(sunPosOrigin, sunDir);
        static int frameCount = 0;
        // if (frameCount++ % 500 == 0) std::cout << "LightSpaceMatrix: " << glm::to_string(lightSpaceMatrix) << std::endl; // Affiche toutes les 500 frames pour ne pas flood la console

        // --- PASSE 1 : REMPLIR LA SHADOW MAP ---
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glViewport(0, 0, shadow.resolution, shadow.resolution);
        glBindFramebuffer(GL_FRAMEBUFFER, shadow.fbo);
        glClear(GL_DEPTH_BUFFER_BIT);
glDisable(GL_CULL_FACE);
        glUseProgram(depthProgram->getId());
        GLint locDepthModel = glGetUniformLocation(depthProgram->getId(), "model");
        glUniformMatrix4fv(glGetUniformLocation(depthProgram->getId(), "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix)); 
        glm::mat4 modelRoom = drawScene(depthProgram->getId(), locDepthModel, vao, indices.size(), sTable, sFrame, sAshtray, sPipe, sCouch, sFireplace, sCandle);
        if(glGetError() != GL_NO_ERROR) {std::cout << "Erreur après drawScene!" << std::endl; break;}

        /* glm::mat4 modelWindow = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(depthProgram->getId(), "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        glBindVertexArray(windowVao);
        glDrawElements(GL_TRIANGLES, windowIndices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);        
        */

        glUseProgram(prg);
        glDepthMask(GL_TRUE); 
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        // ====================================================================
        // ENVOI DES UNIFORMS DE LA POINT LIGHT 
        // ====================================================================
        GLint locLightPos = glGetUniformLocation(prg, "pointLight.position");
        GLint locLightColor = glGetUniformLocation(prg, "pointLight.color");
        GLint locLightAttenuation = glGetUniformLocation(prg, "pointLight.attenuation"); 
        GLint locViewPos = glGetUniformLocation(prg, "viewPos");

        // 1. Position de la lumière (calculée plus haut)
        if (locLightPos >= 0) glUniform3fv(locLightPos, 1, glm::value_ptr(flamePos));
        // 2. Couleur scintillante (calculée plus haut)
        if (locLightColor >= 0) glUniform3fv(locLightColor, 1, glm::value_ptr(finalLightColor));        
        // 3. Paramètres d'atténuation (calculés plus haut)
        if (locLightAttenuation >= 0) glUniform3f(locLightAttenuation, CONSTANT, LINEAR, QUADRATIC);
        // 4. Position de la caméra (pour les spéculaires dans le shader)
        if(locViewPos >= 0) glUniform3f(locViewPos, cameraPosition[0], cameraPosition[1], cameraPosition[2]);
        // ====================================================================
        // FIN DE L'ENVOI DES UNIFORMS DE LA POINT LIGHT
        // ====================================================================

        // Debug uniforms
        GLint locDebug = glGetUniformLocation(prg, "debugMode");
        if(locDebug >= 0) glUniform1i(locDebug, keys[SDLK_N] ? 1 : 0);
        
        GLint locShowNormals = glGetUniformLocation(prg, "showNormals");
        if(locShowNormals >= 0) glUniform1i(locShowNormals, keys[SDLK_M] ? 1 : 0);
        
        GLint locShowUVs = glGetUniformLocation(prg, "showUVs");
        if(locShowUVs >= 0) glUniform1i(locShowUVs, keys[SDLK_U] ? 1 : 0);

        // 1. Initialiser les matériaux de la room (0-7)
        for(int i = 0; i < 8; i++) {
            std::string kdName = "materialKd[" + std::to_string(i) + "]";
            std::string hasTexName = "materialHasTex[" + std::to_string(i) + "]";
            
            GLint locKd = glGetUniformLocation(prg, kdName.c_str());
            GLint locHasTex = glGetUniformLocation(prg, hasTexName.c_str());
            
            if(locKd >= 0) glProgramUniform3f(prg, locKd, 0.8f, 0.8f, 0.8f); // Gris par défaut
            if(locHasTex >= 0) glProgramUniform1i(prg, locHasTex, 1); // Tous ont des textures
        }//for
        currentOffset = 0;        

        // 2. Matériaux de la table
        for(int i = 0; i < (int)table.materialProps.size(); i++) {
            int idx = baseUnit + currentOffset + i;
            std::string kdName = "materialKd[" + std::to_string(idx) + "]";
            std::string hasTexName = "materialHasTex[" + std::to_string(idx) + "]";
            
            GLint locKd = glGetUniformLocation(prg, kdName.c_str());
            GLint locHasTex = glGetUniformLocation(prg, hasTexName.c_str());
            
            if(locKd >= 0) glProgramUniform3fv(prg, locKd, 1, table.materialProps[i].Kd);
            if(locHasTex >= 0) glProgramUniform1i(prg, locHasTex, table.materialProps[i].hasTexture ? 1 : 0);
        }//for
        currentOffset += (int)table.materialProps.size();

        // 3. Matériaux du frame
        int frameOffset = baseUnit + table.materialProps.size();
        for(int i = 0; i < (int)frame.materialProps.size(); i++) {
            int idx = baseUnit + currentOffset + i;
            std::string kdName = "materialKd[" + std::to_string(idx) + "]";
            std::string hasTexName = "materialHasTex[" + std::to_string(idx) + "]";
            
            GLint locKd = glGetUniformLocation(prg, kdName.c_str());
            GLint locHasTex = glGetUniformLocation(prg, hasTexName.c_str());
            
            if(locKd >= 0) glProgramUniform3fv(prg, locKd, 1, frame.materialProps[i].Kd);
            if(locHasTex >= 0) glProgramUniform1i(prg, locHasTex, frame.materialProps[i].hasTexture ? 1 : 0);
        }//for
        currentOffset += (int)frame.materialProps.size();

        // 4. Matériaux du cendrier (ashtray)
        int ashtrayOffset = frameOffset + frame.materialProps.size();
        for (int i = 0; i < (int)ashtray.materialProps.size(); i++) {
            int idx = baseUnit + currentOffset + i;
            std::string kdName = "materialKd[" + std::to_string(idx) + "]";
            std::string hasTexName = "materialHasTex[" + std::to_string(idx) + "]";
            
            GLint locKd = glGetUniformLocation(prg, kdName.c_str());
            GLint locHasTex = glGetUniformLocation(prg, hasTexName.c_str());
            
            if(locKd >= 0) glProgramUniform3fv(prg, locKd, 1, ashtray.materialProps[i].Kd);
            if(locHasTex >= 0) glProgramUniform1i(prg, locHasTex, ashtray.materialProps[i].hasTexture ? 1 : 0);
        }//for
        currentOffset += (int)ashtray.materialProps.size();

        // 5. Matériaux de la pipe (pipe)
        for (int i = 0; i < (int)pipe.materialProps.size(); i++) {
            int idx = baseUnit + currentOffset + i;
            std::string kdName = "materialKd[" + std::to_string(idx) + "]";
            std::string hasTexName = "materialHasTex[" + std::to_string(idx) + "]";
            
            GLint locKd = glGetUniformLocation(prg, kdName.c_str());
            GLint locHasTex = glGetUniformLocation(prg, hasTexName.c_str());
            
            if(locKd >= 0) glProgramUniform3fv(prg, locKd, 1, pipe.materialProps[i].Kd);
            if(locHasTex >= 0) glProgramUniform1i(prg, locHasTex, pipe.materialProps[i].hasTexture ? 1 : 0);
        }//for
        currentOffset += (int)pipe.materialProps.size();

        // 5. Matériaux du canapé (couch)
        for (int i = 0; i < (int)couch.materialProps.size(); i++) {
            int idx = baseUnit + currentOffset + i;
            std::string kdName = "materialKd[" + std::to_string(idx) + "]";
            std::string hasTexName = "materialHasTex[" + std::to_string(idx) + "]";
            
            GLint locKd = glGetUniformLocation(prg, kdName.c_str());
            GLint locHasTex = glGetUniformLocation(prg, hasTexName.c_str());
            
            if(locKd >= 0) glProgramUniform3fv(prg, locKd, 1, couch.materialProps[i].Kd);
            if(locHasTex >= 0) glProgramUniform1i(prg, locHasTex, couch.materialProps[i].hasTexture ? 1 : 0);
        }//for
        currentOffset += (int)couch.materialProps.size();

        // 6. Matériaux de la cheminée (fireplace)
        for (int i = 0; i < (int)fireplace.materialProps.size(); i++) {
            int idx = baseUnit + currentOffset + i;
            std::string kdName = "materialKd[" + std::to_string(idx) + "]";
            std::string hasTexName = "materialHasTex[" + std::to_string(idx) + "]";
            
            GLint locKd = glGetUniformLocation(prg, kdName.c_str());
            GLint locHasTex = glGetUniformLocation(prg, hasTexName.c_str());
            
            if(locKd >= 0) glProgramUniform3fv(prg, locKd, 1, fireplace.materialProps[i].Kd);
            if(locHasTex >= 0) glProgramUniform1i(prg, locHasTex, fireplace.materialProps[i].hasTexture ? 1 : 0);
        }//for
        currentOffset += (int)fireplace.materialProps.size();

        // 7. Matériaux de la bougie (candle)
        for (int i = 0; i < (int)candle.materialProps.size(); i++) {
            int idx = baseUnit + currentOffset + i;
            std::string kdName = "materialKd[" + std::to_string(idx) + "]";
            std::string hasTexName = "materialHasTex[" + std::to_string(idx) + "]";
            
            GLint locKd = glGetUniformLocation(prg, kdName.c_str());
            GLint locHasTex = glGetUniformLocation(prg, hasTexName.c_str());
            
            if(locKd >= 0) glProgramUniform3fv(prg, locKd, 1, candle.materialProps[i].Kd);
            if(locHasTex >= 0) glProgramUniform1i(prg, locHasTex, candle.materialProps[i].hasTexture ? 1 : 0);
        }//for
        currentOffset += (int)candle.materialProps.size();

        // ====================================================================
        // PHASE 1 : OBJETS OPAQUES (Murs, Sol, OBJs)
        // ====================================================================
        glUseProgram(prg); 
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE); // Écrire dans le Depth Buffer
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK); // Culling par défaut
        glDisable(GL_BLEND);

        if(locRenderPass >= 0) glUniform1i(locRenderPass, 0); // Opaque Pass

        // SHADOW MAPPING --- PASSE 2 : RENDU FINAL ---
        glUseProgram(prg);
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Retour à l'écran
        glViewport(0, 0, winWidth, winHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Envoyer la matrice de lumière (utiliser l'ID du programme)
        GLint locLSM = glGetUniformLocation(prg, "lightSpaceMatrix");
        if(locLSM >= 0) glUniformMatrix4fv(locLSM, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        GLint lView  = glGetUniformLocation(prg, "viewMatrix");
        if(lView  >= 0) glUniformMatrix4fv(lView,  1, GL_FALSE, viewMatrix);
        GLint lProj  = glGetUniformLocation(prg, "projMatrix");
        if(lProj  >= 0) glUniformMatrix4fv(lProj,  1, GL_FALSE, projMatrix);
        // ??? GLint lModel = glGetUniformLocation(prg, "model");
        // ??? if(lModel >= 0) glUniformMatrix4fv(lModel, 1, GL_FALSE, glm::value_ptr(modelRoom)); 
        // On lie la texture de la Shadow Map qu'on vient de remplir en PASSE 1
        glActiveTexture(GL_TEXTURE1); 
        glBindTexture(GL_TEXTURE_2D, shadow.depthTexture);
        GLint locSM2 = glGetUniformLocation(prg, "shadowMap");
        glUniform1i(locSM2, 1);
        GLint locModel = glGetUniformLocation(prg, "model");
        drawScene(prg, locModel, vao, indices.size(), sTable, sFrame, sAshtray, sPipe, sCouch, sFireplace, sCandle);
        if(glGetError() != GL_NO_ERROR) {std::cout << "Erreur après drawScene (2)!" << std::endl; break;}

        
        // Ajouter la fenêtre au rendu final
        glm::mat4 modelWindow = glm::mat4(1.0f);
        glDisable(GL_CULL_FACE); // On désactive pour être sûr de voir la vitre
        glUniform1i(locRenderPass, 1);
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glBindVertexArray(windowVao);
        glDrawElements(GL_TRIANGLES, windowIndices.size(), GL_UNSIGNED_INT, 0);
        if(glGetError() != GL_NO_ERROR) {std::cout << "Erreur après Window!" << std::endl; break;}
        glEnable(GL_CULL_FACE);
        // glBindVertexArray(0);        

        // Update smoke (deltaTime = 0.016f pour 60 FPS, ou utilisez un timer réel)
        smokeEmitter.update(0.010f);

        // ====================================================================
        // PHASE 2 : OBJETS TRANSPARENTS (Vitre et God Rays)
        // ====================================================================
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE); // Ne pas écrire dans le depth buffer

        if(locRenderPass >= 0) glUniform1i(locRenderPass, 1); // Transparent Pass
        
        // Configuration pour les God Rays Volumétriques (Culling Inversé ou désactivé)
        // On utilise glCullFace(GL_FRONT) pour les rayons volumétriques
        glCullFace(GL_FRONT); 
        
        // Redessiner la pièce (seuls MatID 6 et 7 seront dessinés)
        modelRoom = glm::mat4(1.0f);
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelRoom));
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        if(glGetError() != GL_NO_ERROR) {std::cout << "Erreur après MatID 6 & 7!" << std::endl; break;}


        // Configuration pour fumée 
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE); // Particules visibles des 2 côtés

        glBindTextureUnit(0, smokeTex);
        GLint locSmokeTex = glGetUniformLocation(smokeProgram, "smokeTexture");
        if (locSmokeTex >= 0) glUniform1i(locSmokeTex, 0);

        // Convertir viewMatrix et projMatrix en glm::mat4
        glm::mat4 viewMat = glm::make_mat4(viewMatrix);
        glm::mat4 projMat = glm::make_mat4(projMatrix);
        glm::vec3 camPos(cameraPosition[0], cameraPosition[1], cameraPosition[2]);

        smokeEmitter.render(smokeProgram, viewMat, projMat, camPos);

        // Restaurer l'état
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glCullFace(GL_BACK); // Rétablir le culling par défaut
        // glEnable(GL_CULL_FACE);
        // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // ====================================================================
        // PHASE 3 : Rendu de la Flamme
        // ====================================================================
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);

        glUseProgram(flameProgram);

        // Vérifier que le programme est valide
        GLint isLinked = 0;
        glGetProgramiv(flameProgram, GL_LINK_STATUS, &isLinked);
        if (isLinked == GL_FALSE) {
            std::cerr << "ERROR: Flame program not linked!" << std::endl;
        }

        // Position de la flamme au-dessus de la bougie
        // glm::vec3 flamePos = glm::vec3(-0.44f, 1.515f, -1.9f);

        // Récupérer les locations des uniforms
        GLint locFlameView = glGetUniformLocation(flameProgram, "viewMatrix");
        GLint locFlameProj = glGetUniformLocation(flameProgram, "projMatrix");
        GLint locFlamePos = glGetUniformLocation(flameProgram, "flamePosition");
        GLint locFlameSize = glGetUniformLocation(flameProgram, "flameSize");
        GLint locFlameTime = glGetUniformLocation(flameProgram, "time");
        GLint locFlameTex = glGetUniformLocation(flameProgram, "flameTexture");

        // Debug: afficher les locations (une seule fois)
        static bool debugPrinted = false;
        if (!debugPrinted) {
            std::cout << "Flame uniform locations:" << std::endl;
            std::cout << "  viewMatrix: " << locFlameView << std::endl;
            std::cout << "  projMatrix: " << locFlameProj << std::endl;
            std::cout << "  flamePosition: " << locFlamePos << std::endl;
            std::cout << "  flameSize: " << locFlameSize << std::endl;
            std::cout << "  time: " << locFlameTime << std::endl;
            std::cout << "  flameTexture: " << locFlameTex << std::endl;
            debugPrinted = true;
        }

        // Définir les uniforms
        if (locFlameView >= 0) glUniformMatrix4fv(locFlameView, 1, GL_FALSE, viewMatrix);
        if (locFlameProj >= 0) glUniformMatrix4fv(locFlameProj, 1, GL_FALSE, projMatrix);
        if (locFlamePos >= 0) glUniform3fv(locFlamePos, 1, glm::value_ptr(flamePos));
        if (locFlameSize >= 0) glUniform1f(locFlameSize, currentFlameSize); 
        if (locFlameTime >= 0) glUniform1f(locFlameTime, currentTime);

        // Lier la texture AVANT de définir l'uniform
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, flameTex);
        if (locFlameTex >= 0) {
            glUniform1i(locFlameTex, 0);
        }

        // Vérifier erreurs OpenGL avant le rendu
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL error before flame render: " << err << std::endl;
        }

        // Dessiner le billboard
        glBindVertexArray(flameQuadVao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        if(glGetError() != GL_NO_ERROR) {std::cout << "Erreur après Draw Flame!" << std::endl; break;}


        // Vérifier erreurs après rendu
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL error after flame render: " << err << std::endl;
        }
        // Restaurer l'état OpenGL
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);        

        // Done
        SDL_GL_SwapWindow(window);
    }//while

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    return 0;
}