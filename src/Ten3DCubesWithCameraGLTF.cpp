#include <emscripten/emscripten.h>
#define GLFW_INCLUDE_ES3
#include <GLFW/glfw3.h>
#include <GLES3/gl3.h>

#include "stb_image/stb_image.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "imgui/imgui/imgui.h"
#include "imgui/imgui/imgui_impl_glfw.h"
#include "imgui/imgui/imgui_impl_opengl3.h"

#include <iostream>
#include <vector>

#include "GLTFLoader.h"

void loop();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

bool  firstMouse = true;
float yaw   = 0.0f;
float pitch = 0.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
float fov   = 45.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

const char* vertexShaderSource = "#version 300 es                                             \n"
                                 "                                                              \n"
                                 "layout (location = 0) in vec3 aPos;                           \n"
                                 "layout (location = 1) in vec2 aTexCoord;                      \n"
                                 "                                                              \n"
                                 "out vec2 TexCoord;                                            \n"
                                 "                                                              \n"
                                 "uniform mat4 model;                                           \n"
                                 "uniform mat4 view;                                            \n"
                                 "uniform mat4 projection;                                      \n"
                                 "                                                              \n"
                                 "void main()                                                   \n"
                                 "{                                                             \n"
                                 "   gl_Position = projection * view * model * vec4(aPos, 1.0); \n"
                                 "   TexCoord    = aTexCoord;                                   \n"
                                 "}                                                             \0";

const char* fragmentShaderSource = "#version 300 es                                 \n"
                                   "precision mediump float;                        \n"
                                   "                                                \n"
                                   "in  vec2 TexCoord;                              \n"
                                   "                                                \n"
                                   "out vec4 FragColor;                             \n"
                                   "                                                \n"
                                   "uniform sampler2D texture1;                     \n"
                                   "uniform sampler2D texture2;                     \n"
                                   "                                                \n"
                                   "void main()                                     \n"
                                   "{                                               \n"
                                   "   FragColor = mix(texture(texture1, TexCoord), \n"
                                   "                   texture(texture2, TexCoord), \n"
                                   "                   0.2);                        \n"
                                   "}                                               \0";

GLFWwindow* window;
int shaderProgram;
unsigned int VAO;
unsigned int texture1, texture2;
std::vector<glm::vec3> cubePositions;
std::vector<AnimatedMesh> mGroundMeshes;

int main()
{
   glfwInit();
   // Tell GLFW we want to use OpenGL version 3.3
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   // Tell GLFW we want to use the OpenGL Core Profile
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

   window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
   if (window == NULL)
   {
      std::cout << "Failed to create GLFW window" << std::endl;
      glfwTerminate();
      return -1;
   }

   glfwMakeContextCurrent(window);

   glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

   //glfwSetCursorPosCallback(window, mouse_callback);

   glfwSetScrollCallback(window, scroll_callback);

   //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

   // Initialize ImGui
   // Setup Dear ImGui context
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO& io = ImGui::GetIO(); (void)io;
   //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
   //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

   // Setup Dear ImGui style
   ImGui::StyleColorsDark();

   // Setup Platform/Renderer bindings
   ImGui_ImplGlfw_InitForOpenGL(window, true);
   ImGui_ImplOpenGL3_Init("#version 300 es");

   glEnable(GL_DEPTH_TEST);

   // Vertex shader
   int vertexShader = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
   glCompileShader(vertexShader);

   int success;
   char infoLog[512];

   glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
   if (!success)
   {
      glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
   }

   if (glGetError() != GL_NO_ERROR) { std::cout << "GL ERROR 2!" << std::endl; }

   // Fragment shader
   int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
   glCompileShader(fragmentShader);

   glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
   if (!success)
   {
      glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
   }

   if (glGetError() != GL_NO_ERROR) { std::cout << "GL ERROR 3!" << std::endl; }

   // Shader program
   shaderProgram = glCreateProgram();
   glAttachShader(shaderProgram, vertexShader);
   glAttachShader(shaderProgram, fragmentShader);
   glLinkProgram(shaderProgram);

   glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
   if (!success)
   {
      glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
   }

   glDeleteShader(vertexShader);
   glDeleteShader(fragmentShader);

   if (glGetError() != GL_NO_ERROR) { std::cout << "GL ERROR 4!" << std::endl; }

   //                     Positions           Texture coords
   //                    <--------------->    <-------->
   float vertices[] = { -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
                         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
                         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
                         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
                        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
                        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

                        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
                         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
                         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
                         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
                        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
                        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

                        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
                        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
                        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
                        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
                        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
                        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

                         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
                         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
                         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
                         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
                         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
                         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

                        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
                         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
                         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
                         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
                        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
                        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

                        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
                         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
                         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
                         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
                        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
                        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f };

   cubePositions = { glm::vec3( 0.0f,  0.0f,  0.0f),
                     glm::vec3( 2.0f,  5.0f, -15.0f),
                     glm::vec3(-1.5f, -2.2f, -2.5f),
                     glm::vec3(-3.8f, -2.0f, -12.3f),
                     glm::vec3( 2.4f, -0.4f, -3.5f),
                     glm::vec3(-1.7f,  3.0f, -7.5f),
                     glm::vec3( 1.3f, -2.0f, -2.5f),
                     glm::vec3( 1.5f,  2.0f, -2.5f),
                     glm::vec3( 1.5f,  0.2f, -1.5f),
                     glm::vec3(-1.3f,  1.0f, -1.5f) };

   unsigned int VBO;

   glGenVertexArrays(1, &VAO);
   glGenBuffers(1, &VBO);

   glBindVertexArray(VAO);

   glBindBuffer(GL_ARRAY_BUFFER, VBO);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

   if (glGetError() != GL_NO_ERROR) { std::cout << "GL ERROR 5!" << std::endl; }

   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
   glEnableVertexAttribArray(0);

   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*) (3 * sizeof(GL_FLOAT)));
   glEnableVertexAttribArray(1);

   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);

   if (glGetError() != GL_NO_ERROR) { std::cout << "GL ERROR 6!" << std::endl; }

   // Texture 1

   glGenTextures(1, &texture1);

   glBindTexture(GL_TEXTURE_2D, texture1);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   if (glGetError() != GL_NO_ERROR) { std::cout << "GL ERROR 7!" << std::endl; }

   stbi_set_flip_vertically_on_load(true);

   int width, height, nrChannels;
   unsigned char *data = stbi_load("resources/container.jpg", &width, &height, &nrChannels, 0);

   if (data)
   {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D);
   }
   else
   {
      std::cout << "Failed to load texture" << std::endl;
   }

   glBindTexture(GL_TEXTURE_2D, 0);

   if (glGetError() != GL_NO_ERROR) { std::cout << "GL ERROR 8!" << std::endl; }

   stbi_image_free(data);

   // Texture 2

   glGenTextures(1, &texture2);

   glBindTexture(GL_TEXTURE_2D, texture2);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   if (glGetError() != GL_NO_ERROR) { std::cout << "GL ERROR 9!" << std::endl; }

   data = stbi_load("resources/awesomeface.png", &width, &height, &nrChannels, 0);

   if (glGetError() != GL_NO_ERROR) { std::cout << "GL ERROR 9.01!" << std::endl; }

   if (data)
   {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D);

      if (glGetError() != GL_NO_ERROR) { std::cout << "GL ERROR 9.1!" << std::endl; }
   }
   else
   {
      std::cout << "Failed to load texture" << std::endl;
   }

   glBindTexture(GL_TEXTURE_2D, 0);

   if (glGetError() != GL_NO_ERROR) { std::cout << "GL ERROR 10!" << std::endl; }

   stbi_image_free(data);

   glUseProgram(shaderProgram);

   unsigned int tex1Loc = glGetUniformLocation(shaderProgram, "texture1");
   glUniform1i(tex1Loc, 0);
   if (tex1Loc == -1) { std::cout << "GL ERROR A!" << std::endl; }

   unsigned int tex2Loc = glGetUniformLocation(shaderProgram, "texture2");
   glUniform1i(tex2Loc, 1);
   if (tex2Loc == -1) { std::cout << "GL ERROR B!" << std::endl; }

   if (glGetError() != GL_NO_ERROR) { std::cout << "GL ERROR 11!" << std::endl; }

   // ---

   cgltf_data* gltfData = LoadGLTFFile("resources/IKCourse.gltf");
   mGroundMeshes = LoadStaticMeshes(gltfData);
   FreeGLTFFile(gltfData);

   int positionsAttribLoc = 0;
   //int normalsAttribLocOfStaticShader   = mStaticMeshShader->getAttributeLocation("normal");
   int texCoordsAttribLoc = 1;
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mGroundMeshes.size());
        i < size;
        ++i)
   {
      mGroundMeshes[i].ConfigureVAO(positionsAttribLoc,
                                    -1, //normalsAttribLocOfStaticShader,
                                    texCoordsAttribLoc);
   }

   // ---

   //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

   emscripten_set_main_loop(loop, 0, true);

   glDeleteVertexArrays(1, &VAO);
   glDeleteBuffers(1, &VBO);

   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();

   glfwTerminate();

   return 0;
}

bool changeBGColor = false;

void loop()
{
   if (glGetError() != GL_NO_ERROR)
   {
      std::cout << "GL ERROR!" << std::endl;
   }

   float currentFrame = (float)glfwGetTime();
   deltaTime = currentFrame - lastFrame;
   lastFrame = currentFrame;

   processInput(window);

   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();

   //bool show = true;
   //ImGui::ShowDemoWindow(&show);

   ImGui::Begin("Test Window");
   ImGui::Checkbox("Change BG Color", &changeBGColor);
   ImGui::End();

   if (changeBGColor)
   {
      glClearColor(0.7f, 0.3f, 0.3f, 1.0f);
   }
   else
   {
      glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
   }

   //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, texture1);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, texture2);

   if (glGetError() != GL_NO_ERROR)
   {
      std::cout << "GL ERROR 20!" << std::endl;
   }

   glUseProgram(shaderProgram);

   if (glGetError() != GL_NO_ERROR)
   {
      std::cout << "GL ERROR 21!" << std::endl;
   }

   glm::mat4 projection = glm::perspective(glm::radians(fov),                    // FoV
                                           (float)SCR_WIDTH / (float)SCR_HEIGHT, // Aspect ratio
                                           0.1f,                                 // Near
                                           100.0f);                              // Far

   unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
   if (projectionLoc == -1) { std::cout << "GL ERROR C!" << std::endl; }
   glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

   glm::mat4 view = glm::lookAt(cameraPos,               // From
                                cameraPos + cameraFront, // At: A point + A vector = A point at the tip of the vector
                                cameraUp);               // Up

   if (glGetError() != GL_NO_ERROR)
   {
      std::cout << "GL ERROR 22!" << std::endl;
   }

   unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
   if (viewLoc == -1) { std::cout << "GL ERROR D!" << std::endl; }
   glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

   glBindVertexArray(VAO);

   if (glGetError() != GL_NO_ERROR)
   {
      std::cout << "GL ERROR 23!" << std::endl;
   }

   for (unsigned int i = 0; i < 10; ++i)
   {
      glm::mat4 model(1); // Unit matrix
      float angle = 20.0f * i;

      model = glm::translate(model, cubePositions[i]);
      model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

      unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
      if (modelLoc == -1) { std::cout << "GL ERROR E!" << std::endl; }
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

      // Draw
      glDrawArrays(GL_TRIANGLES, 0, 36);

      if (glGetError() != GL_NO_ERROR)
      {
         std::cout << "GL ERROR 24!" << std::endl;
      }
   }

   glBindVertexArray(0);

   unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
   glm::mat4 model(1);
   glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

   // Loop over the ground meshes and render each one
   for (unsigned int i = 0,
      size = static_cast<unsigned int>(mGroundMeshes.size());
      i < size;
      ++i)
   {
      mGroundMeshes[i].Render();
   }

   glUseProgram(0);

   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

   glfwSwapBuffers(window);
   glfwPollEvents();
}

// This function is used to exit the rendering the loop when the escape key is pressed
// It is called in every iteration of the render loop
void processInput(GLFWwindow* window)
{
   if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
   {
      glfwSetWindowShouldClose(window, true);
   }

   float cameraSpeed = 2.5f * deltaTime;

   if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) // Up
   {
      // Move in the -Z direction
      cameraPos += cameraSpeed * cameraFront;
   }

   if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) // Down
   {
      // Move in the +Z direction
      cameraPos -= cameraSpeed * cameraFront;
   }

   if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) // Left
   {
      // Move in the -X direction
      cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
   }

   if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) // Right
   {
      // Move in the +X direction
      cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
   }
}

// When the window is resized, the rendering window must also be resized
// For this reason, we call glViewport in the resize callback with the new dimensions of the window
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
   // glViewPort specifies the size of the rendering window
   // The first two parameters (x, y) set the location of the lower left corner of the rendering window
   // The third and fourth parameters set the width and the height of the rendering window
   // The specified coordinates tell OpenGL how it should map its Normalized Device Coordinates (NDC),
   // which range from -1 to 1, to window coordinates (whose range is defined here)
   // We could make the rendering window smaller than GLFW's window, and use the extra space to display a menu
   // In this case, glViewport maps 2D coordinates as illustrated below:
   // Horizontally: (-1, 1) -> (0, 800)
   // Vertically:   (-1, 1) -> (0, 600)
   glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
   // Controlling an FPS camera with mouse input involves the following:
   //    1) Calculate the mouse's offset with respect to the last frame
   //    2) Add the offset values to the camera's yaw and pitch values
   //    3) Add some constraints to the maximum and minimum pitch values
   //    4) Calculate the direction vector

   // Point (0, 0) is at the top left corner of the window
   // Point (w, h) is at the upper right corner of the window

   if (firstMouse)
   {
      lastX = static_cast<float>(xpos);
      lastY = static_cast<float>(ypos);
      firstMouse = false;
   }

   float xoffset = static_cast<float>(xpos) - lastX;
   float yoffset = lastY - static_cast<float>(ypos); // Reversed since y-coordinates range from bottom to top

   lastX = static_cast<float>(xpos);
   lastY = static_cast<float>(ypos);

   float sensitivity = 0.15f;
   xoffset *= sensitivity;
   yoffset *= sensitivity;

   yaw   += xoffset;
   pitch += yoffset;

   if (pitch > 89.0f)  { pitch =  89.0f; }
   if (pitch < -89.0f) { pitch = -89.0f; }

   glm::vec3 front;
   front.x     = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
   front.y     = sin(glm::radians(pitch));
   front.z     = -1 * cos(glm::radians(pitch)) * cos(glm::radians(yaw));
   cameraFront = glm::normalize(front);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
   if (fov >= 1.0f && fov <= 45.0f) { fov -= static_cast<float>(yoffset); }
   if (fov <= 1.0f) { fov = 1.0f; }
   if (fov >= 45.0f) { fov = 45.0f; }
}
