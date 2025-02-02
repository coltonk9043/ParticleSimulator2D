#include "Common.h"

#include <string>
#include <sstream>
#include <fstream>
#include "Entities/Entity.h"
#include <chrono>
#include "Input.h"
#include "Mesh.h"

#define BACKEND "alut"

using timer = std::chrono::steady_clock;
const char* title = "Particle Simulator";
GLuint shaderProgram;
Input* input;
std::vector<Entity*> entities;

// Meshes
Mesh* circleMesh;
Mesh* boxMesh;

/// <summary>
/// Creates a Window with a given title, width, and height.
/// </summary>
/// <param name="window">The reference to the Window to be created.</param>
/// <param name="title">The title of the screen.</param>
/// <param name="width">Thw width of the screen.</param>
/// <param name="height">The height of the screen.</param>
/// <param name="framebufferSizeCallback">The callback if the screen size is changed.</param>
void createWindow(GLFWwindow*& window, 
    const char* title) {
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, title, NULL, NULL);
    if (!window) {
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
}

/// <summary>
/// Loads the GLAD library for compiling shaders.
/// </summary>
/// <returns>Whether or not the library was loaded properly.</returns>
bool loadGlad() {
    return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
}

/// <summary>
/// Reads a file and returns it as a string.
/// </summary>
/// <param name="filename">The file name (including path) of the file.</param>
/// <returns>String representation of file.</returns>
std::string readFile(const char* filename) {
    std::ifstream file;
    std::stringstream buf;
    std::string ret = "";

    // Opens the file
    file.open(filename);

    if (file.is_open()) {
        // Read file buffer as a string.
        buf << file.rdbuf();
        ret = buf.str();
    }
    else {
        std::cout << "Could not open " << filename << std::endl;
    }

    // Close file and return
    file.close();
    return ret;
}

/// <summary>
/// Compiles an individual shader.
/// </summary>
/// <param name="filepath">The file name (including path) to the shader.</param>
/// <param name="type">The type of shader program (Vertex/Fragment).</param>
/// <returns>The index of the shader.</returns>
int compileShader(const char* filepath, GLenum type) {
    std::string shaderSrc = readFile(filepath);
    const GLchar* shader = shaderSrc.c_str();

    // Builds and compiles shaders.
    int shaderObj = glCreateShader(type);
    glShaderSource(shaderObj, 1, &shader, NULL);
    glCompileShader(shaderObj);

    // Check for errors in compile.
    int success;
    char infoLog[512];
    glGetShaderiv(shaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shaderObj, 512, NULL, infoLog);
        std::cout << "Error in shader compilation: " << infoLog << std::endl;
        return -1;
    }

    return shaderObj;
}

/// <summary>
/// Compiles a full shader program given a vertex and fragment shader.
/// </summary>
/// <param name="vertexShaderPath">The pointer to the Vertex Shader.</param>
/// <param name="fragmentShaderPath">The pointer to the Fragment Shader.</param>
/// <returns>The index of the full shader program.</returns>
int compileShaderProgram(const char* vertexShaderPath, const char* fragmentShaderPath) {
    int shaderProgram = glCreateProgram();

    // Compiles the shaders.
    int vertexShader = compileShader(vertexShaderPath, GL_VERTEX_SHADER);
    int fragmentShader = compileShader(fragmentShaderPath, GL_FRAGMENT_SHADER);

    if (vertexShader == -1 || fragmentShader == -1)
        return -1;

    // Link the shaders to the program.
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Checks for errors.
    int success;
    char infoLog[512];
    glGetShaderiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "Error in shader linking: " << infoLog << std::endl;
        return -1;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// set projection
void setOrthographicProjection(int shaderProgram, float left, float right, float bottom, float top, float near, float far) {
    Matrix4 mat = Matrix4::Orthographic(left, right, bottom, top, near, far);

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, mat.AsArray());
}

// process input
void processInput(GLFWwindow* window) {
    double xpos, ypos;

    glfwGetCursorPos(window, &xpos, &ypos);
    ypos = SCREEN_HEIGHT - ypos;

    if (input->getMouseButtonPressed(GLFW_MOUSE_BUTTON_1)) {
        entities.push_back(new EntityCircle(Vector2(xpos, ypos), circleMesh));
    }
    else if (input->getMouseButtonPressed(GLFW_MOUSE_BUTTON_2)) {
        Entity* ent = new EntityCircle(Vector2(xpos, ypos), circleMesh);
        ent->setKinematic(true);
        entities.push_back(ent);
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // Moves all balls.
    Vector2 movement (0.0f, 0.0f);
    float rotation = 0.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        movement.y = 5.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        movement.y = -5.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        movement.x = -5.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        movement.x = 5.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        rotation = 1.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        rotation = -1.0f;
    }
 
    for (int i = 0; i < entities.size(); i++) {
        if (!entities[i]->isKinematic()) {
            entities[i]->force.y += movement.y * -GRAVITY * entities[i]->mass;
            entities[i]->force.x += movement.x * entities[i]->mass;
        }
        
        if(entities[i]->type == BOX)
            entities[i]->rotation += rotation;
    }
}

void prepareCircleModel() {
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;

    // set origin
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);

    float pi = 4 * atanf(1.0f);
    float theta = 0.0f;

    unsigned int i;
    for (i = 0; i <= TRIANGLE_RESOLUTION; i++) {
        // set vertices
        vertices.push_back(cosf(theta));
        vertices.push_back(sinf(theta));

        // set indices
        indices.push_back(0);
        indices.push_back(i + 1);
        indices.push_back(i + 2);

        // step up theta
        theta += 2 * pi / TRIANGLE_RESOLUTION;
    }

    circleMesh = new Mesh(vertices, indices);
}

void prepareBoxModel() {
    std::vector<GLfloat> vertices{ 
        0.5f, 0.5f,               //0 -> Bottom Left
        0.5f,  0.5f,              //1 -> Top Left
        0.5f, 0.5f,               //2 -> Bottom Right
        0.5f, 0.5f };             //3 -> Top Right

    std::vector<GLuint> indices = { 0, 1, 2, 1, 3, 2 };
    boxMesh = new Mesh(vertices, indices);
}

int main() {
    // Initialize GLFW.
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // MacOS specific parameter
    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Create window and return error if it does not initialize.
    GLFWwindow* window = nullptr;
    createWindow(window, title);
    if (!window) {
        std::cout << "GLERROR: Could not create window." << std::endl;
        glfwTerminate();
        return -1;
    }

    // Load GLAD and return error if it does not initialize.
    if (!loadGlad()) {
        std::cout << "GLERROR: Could not initialize GLAD." << std::endl;
        glfwTerminate();
        return -1;
    }

    // Creates our input class.
    input = new Input(window);

    // Declares our viewport and compiled shaders.
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Load shaders
    shaderProgram = compileShaderProgram("main.vs", "main.fs");
    setOrthographicProjection(shaderProgram, 0, SCREEN_WIDTH, 0, SCREEN_HEIGHT, 0.0f, 1.0f);
    glUseProgram(shaderProgram);


    // AntiAliasing
    glEnable(GL_DEPTH_TEST);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glEnable(GL_MULTISAMPLE);

    // Prepare Meshes
    prepareCircleModel();
    prepareBoxModel();

    // Spawn Entities.
    entities.push_back(new EntityBox(Vector2(rand() % SCREEN_WIDTH * 0.8f, rand() % SCREEN_HEIGHT - 200), boxMesh));
    
    for (int i = 0; i < 2; i++) {
        entities.push_back(new EntityCircle(Vector2(rand() % SCREEN_WIDTH - 20, rand() % SCREEN_HEIGHT - 20), circleMesh));
    }

    double lastTime = glfwGetTime();
    double deltaTime = 0, nowTime = 0;

    // Render and Logic loop.
    while (!glfwWindowShouldClose(window)) {
        // - Measure time
        nowTime = glfwGetTime();
        deltaTime += (nowTime - lastTime) * TICKS_PER_SECOND;
        lastTime = nowTime;

        // Updates entity in scene.
        while (deltaTime >= 1.0) {
            // Process input of the Scene
            input->Update();
            processInput(window);

            for (int i = 0; i < entities.size(); i++) {
                entities[i]->Update();
            }

            for (int i = 0; i < entities.size(); i++) {
                entities[i]->CheckCollisions(entities);
            }

            deltaTime--;
        }
        
        // Clear the screen for a new frame.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render objects.
        for (int i = 0; i < entities.size(); i++) {
            entities[i]->Render(shaderProgram, deltaTime);
        }

       /*GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
        {
            std::cout << err << "\n";
        }*/

        // Swap Frame Buffers and poll for events.
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup memory
    for (int i = 0; i < entities.size(); i++) {
        delete entities[i];
    }
    delete input;
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}