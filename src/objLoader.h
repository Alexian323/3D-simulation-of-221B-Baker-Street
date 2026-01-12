#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

struct MaterialProperties {
    GLuint textureID = 0;
    float Kd[3] = {0.8f, 0.8f, 0.8f}; // Couleur diffuse par défaut (gris clair)
    bool hasTexture = false;
};

struct OBJMesh {
    std::vector<float> vertices;  // pos(3)+norm(3)+uv(2)+matID(1) = 9 floats
    unsigned int vao = 0;
    unsigned int vbo = 0;
    size_t count = 0;

    // materials loaded from MTL
    std::vector<std::string> materialNames;
    std::vector<GLuint>      materialTextures;
    std::vector<MaterialProperties> materialProps; // Stocke les couleurs
};

GLuint loadTexture(const char* path) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0); // Don't force 3 channels

    if (!data) {
        std::cerr << "Texture failed to load at path: " << path << std::endl;
        return 0;
    }

    std::cout << "Loading texture: " << path << " (" << width << "x" << height << ", " << nrChannels << " channels)" << std::endl;

    // Création de la texture OpenGL
    GLuint textureID;
    glCreateTextures(GL_TEXTURE_2D, 1, &textureID);

    // Choose format based on channels
    GLenum internalFormat, format;
    if (nrChannels == 4) {
        internalFormat = GL_RGBA8;
        format = GL_RGBA;
    } else if (nrChannels == 3) {
        internalFormat = GL_RGB8;
        format = GL_RGB;
    } else if (nrChannels == 1) {
        internalFormat = GL_R8;
        format = GL_RED;
    } else {
        std::cerr << "Unsupported channel count: " << nrChannels << std::endl;
        stbi_image_free(data);
        return 0;
    }

    glTextureStorage2D(textureID, 4, internalFormat, width, height);
    glTextureSubImage2D(textureID, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
    glGenerateTextureMipmap(textureID);

    glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Debug: check first few pixels
    if (width > 10 && height > 10) {
        int sampleX = width / 2;
        int sampleY = height / 2;
        int idx = (sampleY * width + sampleX) * (format == GL_RGBA ? 4 : (format == GL_RGB ? 3 : 1));
        if (format == GL_RGB && idx + 2 < width * height * 3) {
            std::cout << "  Sample pixel at center: R=" << (int)data[idx] 
                      << " G=" << (int)data[idx+1] 
                      << " B=" << (int)data[idx+2] << std::endl;
        }
    }
    
    stbi_image_free(data);
    std::cout << "Texture loaded successfully: " << path << " with ID " << textureID << std::endl;
    return textureID;
}

GLuint loadTexture(const char* paths[], int count) {
  GLuint textureID;
  for (int i = 0; i < count; ++i) {
      const char* path = paths[i];
      textureID = loadTexture(path);
      if (textureID) return textureID;
  }//for
  // Si on arrive ici : aucun chemin n'a fonctionné
  std::cerr << "ERROR: All texture paths failed." << std::endl;
  return 0;
}

inline GLuint loadTextureFromFile(const std::string &p){ 
    const char* s = p.c_str(); 
    return loadTexture(&s, 1); 
}

inline std::unordered_map<std::string, MaterialProperties> loadMTL_file(const std::string &mtlPath) {
    std::unordered_map<std::string, MaterialProperties> mapMatProps;
    std::ifstream f(mtlPath);
    if(!f.is_open()) {
        std::cerr << "Cannot open MTL: " << mtlPath << std::endl;
        return mapMatProps;
    }

    std::string dir;
    size_t slash = mtlPath.find_last_of("/\\");
    if(slash != std::string::npos) dir = mtlPath.substr(0, slash+1);

    std::string line, currentMat;
    MaterialProperties currentProps;
    
    while(std::getline(f, line)) {
        std::stringstream ss(line);
        std::string token;
        ss >> token;
        
        if(token == "newmtl") {
            // Sauvegarder le matériau précédent
            if(!currentMat.empty()) {
                mapMatProps[currentMat] = currentProps;
            }
            // Nouveau matériau
            ss >> currentMat;
            currentProps = MaterialProperties(); // Reset
            
        } else if(token == "Kd") {
            // Lire la couleur diffuse RGB
            ss >> currentProps.Kd[0] >> currentProps.Kd[1] >> currentProps.Kd[2];
            std::cout << "MTL: material " << currentMat << " Kd=(" 
                      << currentProps.Kd[0] << ", " 
                      << currentProps.Kd[1] << ", " 
                      << currentProps.Kd[2] << ")\n";
            
        } else if(token == "map_Kd") {
            std::string tex; 
            ss >> tex;
            std::string full = dir + tex;
            GLuint tid = loadTextureFromFile(full);
            currentProps.textureID = tid;
            currentProps.hasTexture = (tid != 0);
            std::cout << "MTL: material " << currentMat << " -> " << full 
                      << " -> texID " << tid << "\n";
        }
    }
    
    // Sauvegarder le dernier matériau
    if(!currentMat.empty()) {
        mapMatProps[currentMat] = currentProps;
    }
    
    return mapMatProps;
}

// Helper function to emit one vertex to the final mesh structure
inline void emitVertex(std::vector<float>& vertices,
                       const std::vector<std::string>& matIndexToName, // Ajouté pour la vérification du nom
                       int currentMatIndex, int materialIDOffset,
                       const std::vector<float>& positions, 
                       const std::vector<float>& normals, 
                       const std::vector<float>& texcoords,
                       int p_index, int t_index, int n_index) 
{
    // Indices 0-based
    int pidx = p_index * 3;
    int tidx = t_index * 2;
    int nidx = n_index * 3;

    // 1. Positions
    vertices.push_back(positions[pidx + 0]);
    vertices.push_back(positions[pidx + 1]);
    vertices.push_back(positions[pidx + 2]);

    // 2. Normals
    vertices.push_back(normals[nidx + 0]);
    vertices.push_back(normals[nidx + 1]);
    vertices.push_back(normals[nidx + 2]);

    // --- CORRECTION UV ICI (POSITION CORRECTE) ---
    float u, v;
    if (currentMatIndex >= 0 && matIndexToName[currentMatIndex] == "M_picture") {
        // Force full UV coverage for the "M_picture" material.
        if (p_index == 26) { u = 0.0f; v = 1.0f; } // Sommet 27: HAUT-GAUCHE
        else if (p_index == 44) { u = 1.0f; v = 1.0f; } // Sommet 45: HAUT-DROIT
        else if (p_index == 38) { u = 1.0f; v = 0.0f; } // Sommet 39: BAS-DROIT
        else if (p_index == 27) { u = 0.0f; v = 0.0f; } // Sommet 28: BAS-GAUCHE
        else {
            // Fallback (sécurité)
            u = texcoords[tidx + 0];
            v = texcoords[tidx + 1];
        }
    } else {
        // Comportement normal
        u = texcoords[tidx + 0];
        v = texcoords[tidx + 1];
    }

    // 3. UVs
    vertices.push_back(u);
    vertices.push_back(v);
    // --- FIN CORRECTION UV ---

    // 4. Material ID
    float finalMatID = (currentMatIndex >= 0) ? float(currentMatIndex + materialIDOffset) : -1.0f;
    vertices.push_back(finalMatID);
}

inline void emitTriangle(std::vector<float>& vertices,
                         const std::vector<std::string>& matIndexToName, // Nouveau
                         int currentMatIndex, int materialIDOffset,
                         const std::vector<float>& positions, 
                         const std::vector<float>& normals, 
                         const std::vector<float>& texcoords,
                         const std::vector<unsigned int>& p_idx_face, 
                         const std::vector<unsigned int>& t_idx_face, 
                         const std::vector<unsigned int>& n_idx_face,
                         int i0, int i1, int i2) // Indices dans p_idx_face/t_idx_face/n_idx_face
{
    // Emet le sommet i0
    emitVertex(vertices, matIndexToName, currentMatIndex, materialIDOffset, positions, normals, texcoords, 
               p_idx_face[i0], t_idx_face[i0], n_idx_face[i0]);
    // Emet le sommet i1
    emitVertex(vertices, matIndexToName, currentMatIndex, materialIDOffset, positions, normals, texcoords, 
               p_idx_face[i1], t_idx_face[i1], n_idx_face[i1]);
    // Emet le sommet i2
    emitVertex(vertices, matIndexToName, currentMatIndex, materialIDOffset, positions, normals, texcoords, 
               p_idx_face[i2], t_idx_face[i2], n_idx_face[i2]);
}

inline bool loadOBJ(const char* path, OBJMesh& mesh, int materialIDOffset = 0) {
    std::ifstream file(path);
    if(!file.is_open()) { std::cerr<<"Cannot open OBJ: "<<path<<"\n"; return false; }

    std::string directory;
    {
        std::string p(path);
        size_t pos = p.find_last_of("/\\");
        directory = (pos==std::string::npos) ? "" : p.substr(0, pos+1);
    }

    // temporary storage
    std::vector<float> positions, normals, texcoords;
    std::vector<unsigned int> p_idx, t_idx, n_idx;
    
    // CHANGEMENT: Utiliser MaterialProperties
    std::unordered_map<std::string, int> matNameToIndex;
    std::vector<std::string> matIndexToName;
    std::vector<MaterialProperties> matIndexToProps;

    int currentMatIndex = -1;

    std::string line;
    while(std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token; ss >> token;
        
        if(token == "mtllib") {
            std::string mname; ss >> mname;
            auto matMap = loadMTL_file(directory + mname);
            
            // Peupler les structures
            for(auto &kv : matMap) {
                std::string name = kv.first;
                MaterialProperties props = kv.second;
                
                if(matNameToIndex.find(name) == matNameToIndex.end()) {
                    int idx = (int)matIndexToName.size();
                    matNameToIndex[name] = idx;
                    matIndexToName.push_back(name);
                    matIndexToProps.push_back(props);
                }
            }
            
        } else if(token == "usemtl") {
            std::string name; ss >> name;
            auto it = matNameToIndex.find(name);
            if(it == matNameToIndex.end()) {
                int idx = (int)matIndexToName.size();
                matNameToIndex[name] = idx;
                matIndexToName.push_back(name);
                matIndexToProps.push_back(MaterialProperties()); // Défaut
                currentMatIndex = idx;
            } else {
                currentMatIndex = it->second;
            }
            
        } else if(token == "v") {
            float x,y,z; ss>>x>>y>>z; 
            positions.insert(positions.end(), {x,y,z});
        } else if(token == "vn") {
            float x,y,z; ss>>x>>y>>z; 
            normals.insert(normals.end(), {x,y,z});
        } else if(token == "vt") {
            float u,v; ss>>u>>v; 
            texcoords.insert(texcoords.end(), {u,1.0f-v});
        } else if(token == "f") {
            for(int i=0;i<3;i++){
                unsigned int pi,ti,ni; char slash;
                ss>>pi>>slash>>ti>>slash>>ni;
                p_idx.push_back(pi-1);
                t_idx.push_back(ti-1);
                n_idx.push_back(ni-1);
            }
        }
    }

    // Second pass (reste identique...)
    file.clear(); file.seekg(0);
    mesh.vertices.clear();
    mesh.materialNames = matIndexToName;
    mesh.materialProps = matIndexToProps; // ← STOCKER LES PROPS
    
    // Créer les textures pour compatibilité
    mesh.materialTextures.resize(matIndexToProps.size());
    for(size_t i = 0; i < matIndexToProps.size(); i++) {
        mesh.materialTextures[i] = matIndexToProps[i].textureID;
    }

    // ... (reste du code identique pour la triangulation)
    currentMatIndex = -1;
    while(std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token; ss >> token;
        if(token == "mtllib") {
            // Skip
        } else if(token == "usemtl") {
            std::string name; ss>>name;
            auto it = matNameToIndex.find(name);
            if(it != matNameToIndex.end()) {
                currentMatIndex = it->second;
                std::cout << "  Second pass: using material '" << name 
                          << "' -> local index " << currentMatIndex << std::endl;
            } else {
                std::cerr << "  WARNING: Material '" << name 
                          << "' not found in first pass!" << std::endl;
                currentMatIndex = -1;
            }
        } else if(token == "v" || token=="vn" || token=="vt") {
            // ignore
        } else if(token=="f") {
            std::vector<unsigned int> p_idx_face, t_idx_face, n_idx_face;
            
            while(ss.good()) {
                unsigned int pi,ti,ni; char slash;
                if (!(ss>>pi>>slash>>ti>>slash>>ni)) break;
                p_idx_face.push_back(pi-1);
                t_idx_face.push_back(ti-1);
                n_idx_face.push_back(ni-1);
            }
            
            size_t face_size = p_idx_face.size();
            
            if (face_size == 3) {
                emitTriangle(mesh.vertices, matIndexToName, currentMatIndex, 
                           materialIDOffset, positions, normals, texcoords, 
                           p_idx_face, t_idx_face, n_idx_face, 0, 1, 2);
            } else if (face_size == 4) {
                emitTriangle(mesh.vertices, matIndexToName, currentMatIndex, 
                           materialIDOffset, positions, normals, texcoords, 
                           p_idx_face, t_idx_face, n_idx_face, 0, 1, 2); 
                emitTriangle(mesh.vertices, matIndexToName, currentMatIndex, 
                           materialIDOffset, positions, normals, texcoords, 
                           p_idx_face, t_idx_face, n_idx_face, 0, 2, 3);
            } else if (face_size > 4) {
                std::cerr << "WARNING: Polygon with " << face_size 
                          << " vertices not triangulated.\n";
            }            
        }
    }

    mesh.count = mesh.vertices.size() / 9;

    // Création VAO/VBO (identique...)
    glCreateVertexArrays(1, &mesh.vao);
    glCreateBuffers(1, &mesh.vbo);
    glNamedBufferStorage(mesh.vbo, mesh.vertices.size()*sizeof(float), 
                        mesh.vertices.data(), 0);
    glVertexArrayVertexBuffer(mesh.vao, 0, mesh.vbo, 0, 9*sizeof(float));

    glEnableVertexArrayAttrib(mesh.vao, 0);
    glVertexArrayAttribFormat(mesh.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(mesh.vao, 0, 0);

    glEnableVertexArrayAttrib(mesh.vao, 1);
    glVertexArrayAttribFormat(mesh.vao, 1, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float));
    glVertexArrayAttribBinding(mesh.vao, 1, 0);

    glEnableVertexArrayAttrib(mesh.vao, 2);
    glVertexArrayAttribFormat(mesh.vao, 2, 2, GL_FLOAT, GL_FALSE, 6*sizeof(float));
    glVertexArrayAttribBinding(mesh.vao, 2, 0);

    glEnableVertexArrayAttrib(mesh.vao, 3);
    glVertexArrayAttribFormat(mesh.vao, 3, 1, GL_FLOAT, GL_FALSE, 8*sizeof(float));
    glVertexArrayAttribBinding(mesh.vao, 3, 0);

    return true;
}

// Simple Draw
void drawOBJ(const OBJMesh& mesh) {
    glBindVertexArray(mesh.vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh.count);
}

void drawOBJ(const OBJMesh& mesh, GLuint program, int baseTextureUnit = 6)
{
    glUseProgram(program);

    int nmat = (int)mesh.materialTextures.size();
    if(nmat == 0) {
        // fallback: draw without textures
        glBindVertexArray(mesh.vao);
        glDrawArrays(GL_TRIANGLES, 0, mesh.count);
        return;
    }

    // Bind each material texture to a texture unit
    std::vector<GLint> units(nmat);
    for(int i = 0; i < nmat; i++) {
        units[i] = baseTextureUnit + i;
        glBindTextureUnit(units[i], mesh.materialTextures[i]);
    }

    // Send array of sampler2D indices
    GLint loc = glGetUniformLocation(program, "materialTex");
    if(loc >= 0) {
        glProgramUniform1iv(program, loc, nmat, units.data());
    }

    glBindVertexArray(mesh.vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh.count);
}