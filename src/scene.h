#ifndef SCENE_H
#define SCENE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

// Structure pour simplifier le passage des objets OBJ
struct SimpleObj {
    GLuint vao;
    int count;
};

// Ajoutez ici tous les paramètres nécessaires
glm::mat4 drawScene(
    GLuint shaderId, 
    GLint modelLoc, 
    GLuint roomVao, 
    size_t roomIndicesSize,
    const SimpleObj& table,
    const SimpleObj& frame,
    const SimpleObj& ashtray,
    const SimpleObj& pipe,
    const SimpleObj& couch,
    const SimpleObj& fireplace,
    const SimpleObj& candle
) {
    glUseProgram(shaderId);

    float wallZ = -4.0f;
    float offset = 0.001f;

    // 1. Pièce
    glm::mat4 modelRoom = glm::mat4(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelRoom));
    glBindVertexArray(roomVao);
    // Note geGL: souvent il faut spécifier le type explicitement
    glDrawElements(GL_TRIANGLES, (GLsizei)roomIndicesSize, GL_UNSIGNED_INT, nullptr);

    // 2. Table
    glm::mat4 modelTable = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.0f));
    modelTable = glm::scale(modelTable, glm::vec3(0.015f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelTable));
    glBindVertexArray(table.vao);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)table.count);

    // 3. Cadre (Frame)
    glm::mat4 modelFrame = glm::mat4(1.0f);
    modelFrame = glm::translate(modelFrame, glm::vec3(0.0f, 1.8f, wallZ + offset)); 
    modelFrame = glm::scale(modelFrame, glm::vec3(0.05f)); 
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelFrame));
    glBindVertexArray(frame.vao);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)frame.count);

    // 4. Cendrier (Ashtray)
    glm::mat4 modelAshtray = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, -2.0f)); 
    modelAshtray = glm::scale(modelAshtray, glm::vec3(0.05f)); 
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelAshtray));
    glBindVertexArray(ashtray.vao);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)ashtray.count);

    // 5. Pipe
    glm::mat4 modelPipe = glm::translate(glm::mat4(1.0f), glm::vec3(0.15f, 1.0f, -2.0f)); 
    modelPipe = glm::scale(modelPipe, glm::vec3(0.05f)); 
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelPipe));
    glBindVertexArray(pipe.vao);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)pipe.count);

    // 6. Canapé (Couch)
    glm::mat4 modelCouch = glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 0.0f, 3.4f)); 
    modelCouch = glm::scale(modelCouch, glm::vec3(0.30f)); 
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelCouch));
    glBindVertexArray(couch.vao);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)couch.count);

    // 7. Cheminée (Fireplace)
    glm::mat4 modelFireplace = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, wallZ + 0.29f)); 
    modelFireplace = glm::scale(modelFireplace, glm::vec3(0.03f)); 
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelFireplace));
    glBindVertexArray(fireplace.vao);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)fireplace.count);

    // 8. Bougie (Candle)
    glm::mat4 modelCandle = glm::translate(glm::mat4(1.0f), glm::vec3(-0.45f, 1.0f, -1.9f)); 
    modelCandle = glm::scale(modelCandle, glm::vec3(0.015f)); 
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelCandle));
    glBindVertexArray(candle.vao);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)candle.count);

    return modelRoom;
}

#endif