#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include <geGL/geGL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct ShadowMapping {
    GLuint fbo;
    GLuint depthTexture;
    const unsigned int resolution = 2048; // Haute résolution pour des ombres nettes

    void init() {
        // 1. Créer le Framebuffer
        glGenFramebuffers(1, &fbo);

        // 2. Créer la texture de profondeur
        glGenTextures(1, &depthTexture);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        
        // Paramètres de texture critiques pour le Shadow Mapping
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        // 3. Attacher la texture au Framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
        glDrawBuffer(GL_NONE); // On ne dessine pas de couleurs
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    glm::mat4 getLightSpaceMatrix(glm::vec3 sunPos, glm::vec3 sunDir) {
        // On définit une zone de 10x10 mètres autour du centre de la pièce
        float size = 10.0f;
        glm::mat4 lightProjection = glm::ortho(-size, size, -size, size, 0.1f, 20.0f);
        // Le soleil regarde vers la pièce
        glm::mat4 lightView = glm::lookAt(sunPos, sunPos + sunDir, glm::vec3(0.0f, 1.0f, 0.0f));
        return lightProjection * lightView;
    }
};

#endif