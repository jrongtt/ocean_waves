#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

const char* vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec3 position;
    uniform mat4 view;
    uniform mat4 projection;
    out vec3 color;
    
    void main() {
        gl_Position = projection * view * vec4(position, 1.0);
        color = vec3(1.0, 0.0, 0.0);  // Bright red
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    in vec3 color;
    out vec4 FragColor;
    void main() {
        FragColor = vec4(color, 1.0);
    }
)";

GLuint createShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "Shader compilation failed:\n" << infoLog << std::endl;
    }
    return shader;
}

GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = createShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentSource);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "Program linking failed:\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

int main() {
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(800, 800, "Simple Quad", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Create a simple quad in 3D space
    float vertices[] = {
        // Position
        -1.0f, -1.0f, -5.0f,  // bottom left
         1.0f, -1.0f, -5.0f,  // bottom right
         1.0f,  1.0f, -5.0f,  // top right
        -1.0f,  1.0f, -5.0f   // top left
    };
    
    unsigned int indices[] = {
        0, 1, 2,    // first triangle
        2, 3, 0     // second triangle
    };

    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    std::cout << "Shader program created: " << shaderProgram << std::endl;

    // Camera parameters
    float cameraDistance = 6.0f;
    float cameraTheta = 0.0f;    // horizontal angle
    float cameraPhi = 0.0f;      // vertical angle

    glEnable(GL_DEPTH_TEST);

    std::cout << "Starting render loop - you should see a red quad" << std::endl;
    
    while (!glfwWindowShouldClose(window)) {
        // Input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraDistance -= 0.1f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraDistance += 0.1f;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            cameraTheta -= 0.02f;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            cameraTheta += 0.02f;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            cameraPhi = std::min(cameraPhi + 0.02f, 1.5f);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            cameraPhi = std::max(cameraPhi - 0.02f, -1.5f);

        // Clear the screen
        glClearColor(0.0f, 0.0f, 0.2f, 1.0f);  // Dark blue background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use shader
        glUseProgram(shaderProgram);

        // Calculate camera position
        float camX = cameraDistance * std::cos(cameraPhi) * std::cos(cameraTheta);
        float camY = cameraDistance * std::sin(cameraPhi);
        float camZ = cameraDistance * std::cos(cameraPhi) * std::sin(cameraTheta);

        static int frameCount = 0;
        if (frameCount++ % 60 == 0) {
            std::cout << "Camera pos: (" << camX << ", " << camY << ", " << camZ << ")" << std::endl;
        }

        // Create view matrix
        float viewMatrix[16] = {0};
        viewMatrix[0] = 1.0f;
        viewMatrix[5] = 1.0f;
        viewMatrix[10] = 1.0f;
        viewMatrix[12] = -camX;
        viewMatrix[13] = -camY;
        viewMatrix[14] = -camZ;
        viewMatrix[15] = 1.0f;

        // Create projection matrix
        float projectionMatrix[16] = {0};
        float fov = 45.0f * 3.14159f / 180.0f;
        float aspect = 800.0f / 800.0f;
        float near = 0.1f;
        float far = 100.0f;
        float f = 1.0f / std::tan(fov / 2.0f);
        projectionMatrix[0] = f / aspect;
        projectionMatrix[5] = f;
        projectionMatrix[10] = -(far + near) / (far - near);
        projectionMatrix[11] = -1.0f;
        projectionMatrix[14] = -(2.0f * far * near) / (far - near);

        // Set uniforms
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, viewMatrix);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, projectionMatrix);

        // Draw quad
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}