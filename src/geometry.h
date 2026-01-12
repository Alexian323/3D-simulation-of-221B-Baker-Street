#ifndef GEOMETRY_H
#define GEOMETRY_H

struct WindowSettings {
    float width = 2.0f;
    float height = 2.0f;
    float yCenter = 1.75f;
    float xPos; // hw (Mur droit)
    
    // Valeurs calculées pour faciliter l'accès
    float yMin() const { return yCenter - height * 0.5f; }
    float yMax() const { return yCenter + height * 0.5f; }
    float zMin() const { return 0.0f - width * 0.5f; }
    float zMax() const { return 0.0f + width * 0.5f; }
};

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

#endif