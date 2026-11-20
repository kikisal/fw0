// #define GLAD_GL_IMPLEMENTATION
#include "external/glad/glad.h"
#define GLFW_INCLUDE_NONE
#include "external/GLFW/glfw3.h"

#include "linmath.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#define WINDOW_WIDTH  640 
#define WINDOW_HEIGHT 480
#define WINDOW_TITLE  "fw0 beta"

#define SHADER_INVALID  -1
#define PROGRAM_INVALID -1


static char* loadFile(const char* path);

static void glfwErrorCallback(int error, const char* description);
static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

// -- shader module h --

typedef struct shader_st {
    GLuint      id;
    GLenum      type;
    const char* path;
    char*       data;
} shader_t;

#define PROGRAM_MAX_LOCS     16
#define PROGRAM_MAX_UNIFORMS 16

#define VERTEX_POS_LOCATION 0
#define VERTEX_COL_LOCATION 1
#define VERTEX_UV_LOCATION  2

#define UNIFORM_MVP_LOCATION 0

typedef struct shader_program_st {
    GLuint        id;
    const char* name;
    shader_t      vs;
    shader_t      fs;
    GLint        locs[PROGRAM_MAX_LOCS];
    GLint        uniforms[PROGRAM_MAX_UNIFORMS];
} shader_program_t;

void             shaderCompile(shader_t shader);
shader_t         shaderLoadFromFile(const char* path, GLenum type);
bool             programCheckLinkStatus(shader_program_t p);
bool             shaderCheckCompileStatus(shader_t shader);
shader_program_t shaderCreateProgram(const char* name, const char* vs_src, const char* fs_src);
void             shaderDelete(shader_t shader);
// -- end shader module --


typedef struct material_st {
    shader_program_t program;
} material_t;

typedef struct Vertex
{
    vec3 pos;
    vec3 col;
    vec2 uv;
} Vertex;

typedef struct mesh_st {
    GLuint    vbo, vao, ebo;
    Vertex*   vertices;
    size_t    vertex_count;
    uint16_t* triangles;
    size_t    triangles_count;
} mesh_t;

void   meshRender(mesh_t mesh, mat4x4 matrix, material_t m);
mesh_t meshFromArrayVertices(Vertex* vertices, size_t count, uint16_t* triangles, size_t triangle_count);

static const vec3 COLOR_RED = {1.f, 0.1f, 0.1f};
static shader_program_t default_Program;
static material_t defaultMaterial;
// cull CCW faces
static const Vertex g_PlaneVertices[4] = {
    { { -.5f, 0.0f,  -0.5f }, {COLOR_RED[0], COLOR_RED[1], COLOR_RED[2]}, {0.0f, 1.0f} },
    { { -.5f, 0.0f,   0.5f }, {COLOR_RED[0], COLOR_RED[1], COLOR_RED[2]}, {0.0f, 0.0f} },
    { {  .5f, 0.0f,   0.5f }, {COLOR_RED[0], COLOR_RED[1], COLOR_RED[2]}, {1.0f, 0.0f} },
    { {  .5f, 0.0f,  -0.5f }, {COLOR_RED[0], COLOR_RED[1], COLOR_RED[2]}, {1.0f, 1.0f} }
};

static uint16_t g_PlaneTriangles[] = {
    0, 1, 2,
    2, 3, 0
};

// 3d camera
typedef struct camera_st {
    vec3 pos;
    vec3 target;
} camera_t;

typedef struct renderer_st {
    GLFWwindow* window;
    camera_t*   camera;
} renderer_t;

static renderer_t s_Renderer = { NULL };

int main(void) {

    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit())
        exit(EXIT_FAILURE);
 
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, glfwKeyCallback);

    glfwMakeContextCurrent(window);
    gladLoadGL();
    glfwSwapInterval(1);

    s_Renderer.window = window;

    default_Program = shaderCreateProgram("DefaultProgram", "./res/vert.glsl", "./res/frag.glsl");
    
    if (default_Program.id == PROGRAM_INVALID)
        return -1;

    defaultMaterial.program = default_Program;
    mesh_t triangleMesh     = meshFromArrayVertices((Vertex*)g_PlaneVertices, sizeof(g_PlaneVertices) / sizeof(g_PlaneVertices[0]), g_PlaneTriangles, 2);

    
    camera_t camera = {
        .pos    = {0.0f, 2.0f, 2.0f}, 
        .target = {.0f, .0f, .0f}
    };
    
    s_Renderer.camera = &camera;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 
        mat4x4 m;
        mat4x4_identity(m);
        // mat4x4_rotate_Z(m, m, (float) glfwGetTime());

        meshRender(triangleMesh, m, defaultMaterial);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
 
    glfwDestroyWindow(window);
 
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

static char* loadFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL)
        return NULL;

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    long size = ftell(file);
    if (size < 0) {
        fclose(file);
        return NULL;
    }

    rewind(file);

    char* buffer = malloc((size_t)size + 1);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    size_t bytesRead = fread(buffer, 1, (size_t)size, file);

    if (bytesRead != (size_t)size && ferror(file)) {
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

static void glfwErrorCallback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}
 
static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

// shader module
void shaderCompile(shader_t shader) {
    if (shader.id == SHADER_INVALID)
        return;

    glCompileShader(shader.id);
}

shader_t shaderLoadFromFile(const char* path, GLenum type) {
    shader_t shader = {
        .id     = glCreateShader(type),
        .data   = loadFile(path),
        .path   = path,
        .type   = type
    };

    glShaderSource(shader.id, 1, (const char**)&shader.data, NULL);
    return shader;
}

void shaderDelete(shader_t shader) {
    glDeleteShader(shader.id);
}

bool shaderCheckCompileStatus(shader_t shader) {
    GLint isCompiled = 0;
    glGetShaderiv(shader.id, GL_COMPILE_STATUS, &isCompiled);
    if(isCompiled == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetShaderiv(shader.id, GL_INFO_LOG_LENGTH, &maxLength);

        GLchar errorLog[maxLength];
        glGetShaderInfoLog(shader.id, maxLength, &maxLength, &errorLog[0]);

        printf("Failed to compile shader %s: \n", shader.path);
        printf("%s\n", errorLog);
        return false;
    }

    return true;
}

bool programCheckLinkStatus(shader_program_t p) {
    if (p.id == PROGRAM_INVALID) 
        return false;

    GLint isLinked = 0;
    glGetProgramiv(p.id, GL_LINK_STATUS, &isLinked);

    if(isLinked == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetProgramiv(p.id, GL_INFO_LOG_LENGTH, &maxLength);
        GLchar errorLog[maxLength];

        glGetProgramInfoLog(p.id, maxLength, &maxLength, errorLog);

        printf("Failed to link shader program %s (vs: %s, fs: %s).\n", p.name, p.vs.path, p.fs.path);
        printf("%s\n", errorLog);
        return false;
    }

    return true;
}

shader_program_t shaderCreateProgram(const char* name, const char* vs_src, const char* fs_src)
{
    shader_program_t program = {
        .id   = PROGRAM_INVALID,
        .name = name,
        .vs   = 0,
        .fs   = 0
    };

    shader_t vs = shaderLoadFromFile(vs_src, GL_VERTEX_SHADER);
    shader_t fs = shaderLoadFromFile(fs_src, GL_FRAGMENT_SHADER);
 
    shaderCompile(vs);
    shaderCompile(fs);

    bool shaderStatus = shaderCheckCompileStatus(vs);
    shaderStatus      = shaderStatus && shaderCheckCompileStatus(fs);

    if (!shaderStatus) {
        shaderDelete(vs);
        shaderDelete(fs);
        free(vs.data);
        free(fs.data);
        return program;
    }
    
    const GLuint gl_program = glCreateProgram();
    glAttachShader(gl_program, vs.id);
    glAttachShader(gl_program, fs.id);
    glLinkProgram(gl_program);

    program.id = gl_program;
    program.vs = vs;
    program.fs = fs;

    if (!programCheckLinkStatus(program)) {
        glDeleteProgram(gl_program);
        shaderDelete(vs);
        shaderDelete(fs);
        free(vs.data);
        free(fs.data);

        program.id = PROGRAM_INVALID;
        program.vs = (shader_t) {0};
        program.fs = (shader_t) {0};
    }

    // Load default attributes and uniforms
    program.locs[VERTEX_POS_LOCATION]  = 
        glGetAttribLocation(program.id,  "position");
    program.locs[VERTEX_COL_LOCATION]  = 
        glGetAttribLocation(program.id,  "color");
    program.locs[VERTEX_UV_LOCATION]   = 
        glGetAttribLocation(program.id,  "uv");

    program.uniforms[UNIFORM_MVP_LOCATION] = 
        glGetUniformLocation(program.id, "uMVP");

    return program;
}

void meshRender(mesh_t mesh, mat4x4 matrix, material_t m) {
    mat4x4 p, mv, mvp;
    
    int width, height;
    glfwGetFramebufferSize(s_Renderer.window, &width, &height);
    const float ratio = width / (float) height;
    

    vec3 up = {0.0f, 1.0f, 0.0f};

    mat4x4 viewMatrix;

    mat4x4_look_at(viewMatrix, s_Renderer.camera->pos, s_Renderer.camera->target, up);
    mat4x4_mul(mv, viewMatrix, matrix);
    mat4x4_perspective(p, M_PI / 4, ratio, .1, 100.0f);
    mat4x4_mul(mvp, p, mv);

    glViewport(0, 0, width, height);

    glUseProgram(m.program.id);
    glUniformMatrix4fv(m.program.locs[UNIFORM_MVP_LOCATION], 1, GL_FALSE, (const GLfloat*) &mvp[0]);
    
    glBindVertexArray(mesh.vao);
    
    glEnableVertexAttribArray(VERTEX_POS_LOCATION);
    glEnableVertexAttribArray(VERTEX_COL_LOCATION);
    glEnableVertexAttribArray(VERTEX_UV_LOCATION);

    if (!mesh.ebo) {
        glDrawArrays(GL_TRIANGLES, 0, mesh.vertex_count);
    } else {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
        glDrawElements(GL_TRIANGLES, mesh.triangles_count * 3, GL_UNSIGNED_SHORT, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    glBindVertexArray(0);
    glUseProgram(0);
}

mesh_t meshFromArrayVertices(Vertex* vertices, size_t count, uint16_t* triangles, size_t triangle_count) {
    GLuint vbo, vao, ebo = 0;
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);
    mesh_t m = {0};

    if (triangles) {
        m.triangles       = malloc(triangle_count * sizeof(triangles[0]) * 3);
        m.triangles_count = triangle_count;
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangle_count * sizeof(triangles[0]) * 3, triangles, GL_STATIC_DRAW);
        memcpy(m.triangles, triangles, triangle_count * sizeof(triangles[0]) * 3);
    }
 
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(vertices[0]), vertices, GL_STATIC_DRAW); 
    
    glBindVertexArray(vao);
    glVertexAttribPointer(VERTEX_POS_LOCATION, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) offsetof(Vertex, pos));
    glVertexAttribPointer(VERTEX_COL_LOCATION, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) offsetof(Vertex, col));
    glVertexAttribPointer(VERTEX_UV_LOCATION, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) offsetof(Vertex, uv));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    m.vbo             = vbo,
    m.vao             = vao,
    m.ebo             = ebo,
    m.vertex_count    = count,
    m.vertices        = malloc(count * sizeof(vertices[0])),

    memcpy(m.vertices, vertices, count * sizeof(vertices[0]));

    return m;
}