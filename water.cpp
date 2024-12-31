#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>

// Shader sources
const char* vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec3 position;
    layout(location = 1) in vec2 texCoord;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform sampler2D heightMap;
    out vec3 color;
    
    void main() {
        float height = texture(heightMap, texCoord).r * 2.0; // Amplified height
        vec3 pos = position;
        pos.y = height;
        gl_Position = projection * view * model * vec4(pos, 1.0);
        // Make color more visible - red for peaks, blue for troughs
        color = vec3(0.5 + height, 0.2, 0.5 - height);
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

const char* simVertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec2 position;
    layout(location = 1) in vec2 texCoord;
    out vec2 TexCoords;
    void main() {
        TexCoords = texCoord;
        gl_Position = vec4(position, 0.0, 1.0);
    }
)";

const char* simFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    in vec2 TexCoords;
    uniform sampler2D currentState;
    uniform sampler2D previousState;
    uniform float dt;
    uniform float dx;
    uniform float c;
    
    void main() {
        vec2 texelSize = 1.0 / textureSize(currentState, 0);
        float current = texture(currentState, TexCoords).r;
        float previous = texture(previousState, TexCoords).r;
        float left = texture(currentState, TexCoords + vec2(-texelSize.x, 0.0)).r;
        float right = texture(currentState, TexCoords + vec2(texelSize.x, 0.0)).r;
        float up = texture(currentState, TexCoords + vec2(0.0, texelSize.y)).r;
        float down = texture(currentState, TexCoords + vec2(0.0, -texelSize.y)).r;
        
        float laplacian = (left + right + up + down - 4.0 * current);
        float next = 2.0 * current - previous + c * dt * dt * laplacian;
        next *= 0.999;
        
        FragColor = vec4(next, 0.0, 0.0, 1.0);
    }
)";

void checkGLError(const char* message) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cout << message << " OpenGL Error: 0x" << std::hex << err << std::dec << std::endl;
    }
}

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
    std::cout << "Starting program..." << std::endl;
    
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    std::cout << "GLFW initialized successfully" << std::endl;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    
    GLFWwindow* window = glfwCreateWindow(800, 800, "Wave Simulation", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }
    std::cout << "Window created successfully" << std::endl;
    
    glfwMakeContextCurrent(window);
    
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cout << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
        return -1;
    }
    std::cout << "GLEW initialized successfully" << std::endl;
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    // Create shader programs
    GLuint renderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    GLuint simProgram = createShaderProgram(simVertexShaderSource, simFragmentShaderSource);
    std::cout << "Shader programs created: " << renderProgram << ", " << simProgram << std::endl;

    // Create mesh
    int gridSize = 50;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    for (int z = 0; z < gridSize; z++) {
        for (int x = 0; x < gridSize; x++) {
            float xPos = (float)x / (gridSize - 1) * 2.0f - 1.0f;
            float zPos = (float)z / (gridSize - 1) * 2.0f - 1.0f;
            float u = (float)x / (gridSize - 1);
            float v = (float)z / (gridSize - 1);
            vertices.push_back(xPos);
            vertices.push_back(0.0f);
            vertices.push_back(zPos);
            vertices.push_back(u);
            vertices.push_back(v);
        }
    }
    
    for (int z = 0; z < gridSize - 1; z++) {
        for (int x = 0; x < gridSize - 1; x++) {
            unsigned int bl = z * gridSize + x;
            unsigned int br = bl + 1;
            unsigned int tl = (z + 1) * gridSize + x;
            unsigned int tr = tl + 1;
            
            indices.push_back(bl);
            indices.push_back(tl);
            indices.push_back(br);
            indices.push_back(br);
            indices.push_back(tl);
            indices.push_back(tr);
        }
    }

    // Create buffers
    GLuint waterVAO, waterVBO, waterEBO;
    glGenVertexArrays(1, &waterVAO);
    glGenBuffers(1, &waterVBO);
    glGenBuffers(1, &waterEBO);
    
    glBindVertexArray(waterVAO);
    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Create simulation quad for wave updates
    float quadVertices[] = {
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };

    GLuint quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    std::cout << "Created simulation quad" << std::endl;
    GLuint waveTex1, waveTex2, waveFBO1, waveFBO2;
    glGenTextures(1, &waveTex1);
    glGenTextures(1, &waveTex2);
    glGenFramebuffers(1, &waveFBO1);
    glGenFramebuffers(1, &waveFBO2);

    // Setup textures
    for (GLuint tex : {waveTex1, waveTex2}) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, gridSize, gridSize, 0, GL_RED, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // Setup FBOs
    glBindFramebuffer(GL_FRAMEBUFFER, waveFBO1);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, waveTex1, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, waveFBO2);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, waveTex2, 0);

    // Initial conditions - bigger, more visible wave
    std::vector<float> initialData(gridSize * gridSize, 0.0f);
    float centerX = gridSize/2.0f;
    float centerY = gridSize/2.0f;
    float waveRadius = gridSize/4.0f;  // Bigger radius
    std::cout << "Creating initial wave at center (" << centerX << "," << centerY << ") with radius " << waveRadius << std::endl;
    std::cout << "Wave should be visible in center of grid, size " << gridSize << "x" << gridSize << std::endl;
    
    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            float dx = x - centerX;
            float dy = y - centerY;
            float d = std::sqrt(dx*dx + dy*dy);
            if (d < waveRadius) {
                initialData[y * gridSize + x] = 2.0f * std::exp(-d*d/(waveRadius*waveRadius));  // Higher amplitude
            }
        }
    }
    glBindTexture(GL_TEXTURE_2D, waveTex1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, gridSize, gridSize, GL_RED, GL_FLOAT, initialData.data());

    // Simulation parameters
    float dt = 0.016f;
    float dx = 0.1f;
    float c = 0.3f;
    
    // Camera parameters
    float cameraDistance = 8.0f;
    float cameraTheta = 0.785f;
    float cameraPhi = 0.615f;

    bool isFirstTexture = true;
    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {
        static int frameCount = 0;
        
        // Camera position
        float camX = cameraDistance * std::cos(cameraPhi) * std::cos(cameraTheta);
        float camY = cameraDistance * std::sin(cameraPhi);
        float camZ = cameraDistance * std::cos(cameraPhi) * std::sin(cameraTheta);

        if (frameCount % 60 == 0) {  // Print every second at 60fps
            std::cout << "Frame " << frameCount 
                     << " Camera: dist=" << cameraDistance 
                     << " pos=(" << camX << "," << camY << "," << camZ << ")"
                     << std::endl;
        }
        frameCount++;

        // Input handling
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            cameraTheta -= 0.02f;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            cameraTheta += 0.02f;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            cameraPhi = std::min(cameraPhi + 0.02f, 1.5f);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            cameraPhi = std::max(cameraPhi - 0.02f, 0.1f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraDistance = std::max(cameraDistance - 0.1f, 1.0f);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraDistance += 0.1f;

        // Clear
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);

        // Wave simulation step
        glBindFramebuffer(GL_FRAMEBUFFER, isFirstTexture ? waveFBO2 : waveFBO1);
        glViewport(0, 0, gridSize, gridSize);
        
        glUseProgram(simProgram);
        glUniform1f(glGetUniformLocation(simProgram, "dt"), dt);
        glUniform1f(glGetUniformLocation(simProgram, "dx"), dx);
        glUniform1f(glGetUniformLocation(simProgram, "c"), c);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, isFirstTexture ? waveTex1 : waveTex2);
        glUniform1i(glGetUniformLocation(simProgram, "currentState"), 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, isFirstTexture ? waveTex2 : waveTex1);
        glUniform1i(glGetUniformLocation(simProgram, "previousState"), 1);

        // Draw fullscreen quad for simulation
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        checkGLError("After simulation step");

        // Render water mesh
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, 800, 800);
        
        glUseProgram(renderProgram);

        // Set up matrices
        float aspect = 800.0f/800.0f;
        float fov = 60.0f * 3.14159f / 180.0f;
        
        // Projection matrix
        float f = 1.0f / std::tan(fov / 2.0f);
        float projectionMatrix[16] = {0};
        projectionMatrix[0] = f / aspect;
        projectionMatrix[5] = f;
        projectionMatrix[10] = -1.1f;
        projectionMatrix[11] = -1.0f;
        projectionMatrix[14] = -0.1f;

        // View matrix
        float viewMatrix[16] = {0};
        float up[3] = {0.0f, 1.0f, 0.0f};
        float forward[3] = {-camX, -camY, -camZ};
        float right[3] = {
            std::sin(cameraTheta + 3.14159f/2.0f),
            0.0f,
            std::cos(cameraTheta + 3.14159f/2.0f)
        };
        
        viewMatrix[0] = right[0];
        viewMatrix[1] = up[0];
        viewMatrix[2] = forward[0];
        viewMatrix[4] = right[1];
        viewMatrix[5] = up[1];
        viewMatrix[6] = forward[1];
        viewMatrix[8] = right[2];
        viewMatrix[9] = up[2];
        viewMatrix[10] = forward[2];
        viewMatrix[12] = -camX;
        viewMatrix[13] = -camY;
        viewMatrix[14] = -camZ;
        viewMatrix[15] = 1.0f;

        // Model matrix (identity)
        float modelMatrix[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };

        // Set uniforms
        glUniformMatrix4fv(glGetUniformLocation(renderProgram, "projection"), 1, GL_FALSE, projectionMatrix);
        glUniformMatrix4fv(glGetUniformLocation(renderProgram, "view"), 1, GL_FALSE, viewMatrix);
        glUniformMatrix4fv(glGetUniformLocation(renderProgram, "model"), 1, GL_FALSE, modelMatrix);

        // Bind height map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, isFirstTexture ? waveTex2 : waveTex1);
        glUniform1i(glGetUniformLocation(renderProgram, "heightMap"), 0);

        // Draw mesh
        glBindVertexArray(waterVAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
        isFirstTexture = !isFirstTexture;
    }

    // Cleanup
    glDeleteVertexArrays(1, &waterVAO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &waterVBO);
    glDeleteBuffers(1, &waterEBO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteTextures(1, &waveTex1);
    glDeleteTextures(1, &waveTex2);
    glDeleteFramebuffers(1, &waveFBO1);
    glDeleteFramebuffers(1, &waveFBO2);
    glDeleteProgram(renderProgram);
    glDeleteProgram(simProgram);

    glfwTerminate();
    return 0;
}