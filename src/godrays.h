#ifndef GODRAYS_H
#define GODRAYS_H

void generateGodRayVolume(std::vector<float>& vertices, std::vector<uint32_t>& indices, float winX, float winYMin, float winYMax, float winZMin, float winZMax, glm::vec3 sunDir, float length) 
{
    // On définit les 4 points de la fenêtre (face avant)
    glm::vec3 p1(winX, winYMin, winZMin);
    glm::vec3 p2(winX, winYMax, winZMin);
    glm::vec3 p3(winX, winYMax, winZMax);
    glm::vec3 p4(winX, winYMin, winZMax);

    // On projette ces points vers l'intérieur de la pièce selon la direction du soleil
    // On inverse sunDir car il pointe vers la source, on veut aller dans le sens du rayon
    glm::vec3 dir = normalize(-sunDir); 
    glm::vec3 p1_end = p1 + dir * length;
    glm::vec3 p2_end = p2 + dir * length;
    glm::vec3 p3_end = p3 + dir * length;
    glm::vec3 p4_end = p4 + dir * length;

    auto addVertex = [&](glm::vec3 p) {
        vertices.push_back(p.x); vertices.push_back(p.y); vertices.push_back(p.z);
        vertices.push_back(0); vertices.push_back(0); vertices.push_back(0); // Normale (peu importe ici)
        vertices.push_back(0); vertices.push_back(0); // UV
        vertices.push_back(7.0f); // Material ID 7 pour les rayons
    };

    // Ajout des 8 sommets
    addVertex(p1); addVertex(p2); addVertex(p3); addVertex(p4);         // Face Fenêtre (0,1,2,3)
    addVertex(p1_end); addVertex(p2_end); addVertex(p3_end); addVertex(p4_end); // Face Fin (4,5,6,7)

    // Indices pour les 6 faces du volume (12 triangles)
    uint32_t localIndices[] = {
        0,1,2, 0,2,3, // Devant
        4,5,6, 4,6,7, // Fond
        0,1,5, 0,5,4, // Côté bas
        3,2,6, 3,6,7, // Côté haut
        0,3,7, 0,7,4, // Côté gauche
        1,2,6, 1,6,5  // Côté droit
    };
    for(uint32_t i : localIndices) indices.push_back(i);
}

#endif

