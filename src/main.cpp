//
//  main.cpp
//  FCE-To-OBJ
//
//  Created by Amrik Sadhra on 27/01/2015.
//

#include <cstdlib>
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "TGALoader.h"
#include <fstream>
#include <iostream>

using namespace glm;

GLFWwindow *window;

// Source
#include "obj_loader.h"
#include "shader.h"
#include "controls.h"
#include "fce_reader.h"
#include "TGALoader.h"


GLint load_tga_texture(const char* path){
    NS_TGALOADER::IMAGE texture_loader;

    bool tex_load_status = texture_loader.LoadTGA(path);

    if(!tex_load_status){
        printf("Texture loading failed!\n");
        exit(2);
    }

    GLuint textureID;
    glGenTextures(1, &textureID);

    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, texture_loader.getWidth(), texture_loader.getHeight(), 0, GL_BGR, GL_UNSIGNED_BYTE, texture_loader.getDataForOpenGL());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    return textureID;
}

int main(int argc, const char *argv[]) {
    std::cout << "----------- NFS3 Model Viewer v0.5 -----------" << std::endl;
    //Convert FCE to OBJ as temporary bodge
    //TODO: Replace with direct vertex transfer to GL Vert buffer
    convertFCE("car.fce", "Model.obj");

    // Read our .obj file
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    bool res = loadOBJFile("Model.obj", vertices, uvs);

    if(!res){
        cout << "OBJ loading failed. Exiting." << std::endl;
        exit(2);
    }

    // Initialise GLFW
    if (!glfwInit()) {
        fprintf(stderr, "GLFW Init failed.\n");
        getchar();
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Appease the OSX Gods
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(1024, 768, "Need for Speed 3 FCE Loader", nullptr, nullptr);

    if (window == nullptr) {
        fprintf(stderr, "Failed to create a GLFW window.\n");
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = GL_TRUE; // Needed for core profile

    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited movement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, 1024 / 2, 768 / 2);

    // Dark blue background
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Cull triangles which normal is not towards the camera
    glEnable(GL_CULL_FACE);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    GLuint programID = LoadShaders("../shaders/TransformVertexShader.vertexshader",
                                   "../shaders/TextureFragmentShader.fragmentshader");

    // Get a handle for our "MVP" uniform
    GLint MatrixID = glGetUniformLocation(programID, "MVP");

    // Load the texture
    GLuint Texture = load_tga_texture("CAR00.TGA");

    // Get a handle for our "myTextureSampler" uniform
    GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");


    // Load vertices into a VBO
    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

    GLuint uvbuffer;
    glGenBuffers(1, &uvbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);

    do {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use our shader
        glUseProgram(programID);

        // Compute the MVP matrix from keyboard and mouse input
        computeMatricesFromInputs();
        glm::mat4 ProjectionMatrix = getProjectionMatrix();
        glm::mat4 ViewMatrix = getViewMatrix();
        glm::mat4 ModelMatrix = glm::mat4(1.0);

        glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

        // Send our transformation to the currently bound shader,
        // in the "MVP" uniform
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(
                0,                  // attribute
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void *) 0            // array buffer offset
        );

        // 2nd attribute buffer : UVs
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glVertexAttribPointer(
                1,                                // attribute
                2,                                // size
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                                // stride
                (void*)0                          // array buffer offset
        );


        // Draw the triangle !
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    }
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0);

    // Cleanup VBO and shader
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &uvbuffer);
    glDeleteProgram(programID);
    //glDeleteTextures(1, &Texture);
    glDeleteVertexArrays(1, &VertexArrayID);


    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}

