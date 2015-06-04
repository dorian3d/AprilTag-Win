#ifndef WIN32
#include <alloca.h>
#endif
#include <limits>
#include "visualization.h"

#include <stdio.h>
#include <stdlib.h>

visualization * visualization::static_gui;

static const char * vs =
"#version 120\n"
""
"uniform mat4 view, proj;\n"
" "
"attribute vec3 position;\n"
"attribute vec4 color;\n"
"varying vec4 color_fs;\n"
" "
"void main()\n"
"{\n"
"    color_fs = color;\n"
"    gl_Position = proj * view * vec4(position, 1);\n"
"    gl_PointSize = 8.;\n"
"}\n";

static const char * fs = ""
"#version 120\n"
" "
"varying vec4 color_fs;\n"
" "
"void main()"
"{\n"
"    gl_FragColor = color_fs;\n"
"}";

static void print_shader_info_log(GLuint obj)
{
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;

    glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

    if (infologLength > 0)
    {
        infoLog = (char *)malloc(infologLength);
        glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
        fprintf(stderr, "%s\n",infoLog);
        free(infoLog);
    }
}

static void print_program_info_log(GLuint obj)
{
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;

    glGetProgramiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

    if (infologLength > 0)
    {
        infoLog = (char *)malloc(infologLength);
        glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
        fprintf(stderr, "%s\n",infoLog);
        free(infoLog);
    }
}

void visualization_render::gl_init()
{
    GLuint p,v,f;

    v = glCreateShader(GL_VERTEX_SHADER);
    f = glCreateShader(GL_FRAGMENT_SHADER);

#if TARGET_OS_IPHONE
    glShaderSource(v, 1, &vs_es,NULL);
    glShaderSource(f, 1, &fs_es,NULL);
#else
    glShaderSource(v, 1, &vs,NULL);
    glShaderSource(f, 1, &fs,NULL);
#endif

    glCompileShader(v);
    glCompileShader(f);

    print_shader_info_log(v);
    print_shader_info_log(f);

    p = glCreateProgram();
    glAttachShader(p,v);
    glAttachShader(p,f);

    glLinkProgram(p);
    print_program_info_log(p);

    glDetachShader(p,v);
    glDetachShader(p,f);
    glDeleteShader(v);
    glDeleteShader(f);

    vertex_loc = glGetAttribLocation(p, "position");
    color_loc = glGetAttribLocation(p, "color");

    projection_matrix_loc = glGetUniformLocation(p, "proj");
    view_matrix_loc = glGetUniformLocation(p, "view");
    program = p;
}

static void draw_array(int vertex_loc, int color_loc, GLData * data, int number, int gl_type)
{
    //fprintf(stderr, "Draw %d as %d\n", number, gl_type);
	glVertexAttribPointer(vertex_loc, 3, GL_FLOAT, 0, sizeof(GLData), &data[0].position);
	glEnableVertexAttribArray(vertex_loc);
	glVertexAttribPointer(color_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GLData), &data[0].color);
	glEnableVertexAttribArray(color_loc);

    glDrawArrays(gl_type, 0, number);
}

void visualization_render::gl_render(float * view_matrix, float * projection_matrix, render_data * data)
{
    glUseProgram(program);

    glUniformMatrix4fv(projection_matrix_loc,  1, false, projection_matrix);
    glUniformMatrix4fv(view_matrix_loc,  1, false, view_matrix);

    data->data_lock.lock();

    // Draw the frame
    glLineWidth(2);
    draw_array(vertex_loc, color_loc, data->grid_vertex, data->grid_vertex_num, GL_LINES);
    glLineWidth(4);
    draw_array(vertex_loc, color_loc, data->axis_vertex, data->axis_vertex_num, GL_LINES);
    draw_array(vertex_loc, color_loc, data->pose_vertex, data->pose_vertex_num, GL_LINES);

    data->data_lock.unlock();
}

void visualization_render::gl_destroy()
{
    glDeleteProgram(program);
}

const static float initial_scale = 5;

#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
static void build_projection_matrix(float * projMatrix, float fov, float ratio, float nearP, float farP)
{
    float f = 1.0f / tan (fov * ((float)M_PI / 360.0f));

    for(int i = 0; i < 16; i++) projMatrix[i] = 0;
    projMatrix[0] = 1;
    projMatrix[5] = 1;
    projMatrix[10] = 1;
    projMatrix[15] = 1;

    projMatrix[0] = f / ratio;
    projMatrix[1 * 4 + 1] = f;
    projMatrix[2 * 4 + 2] = (farP + nearP) / (nearP - farP);
    projMatrix[3 * 4 + 2] = (2.0f * farP * nearP) / (nearP - farP);
    projMatrix[2 * 4 + 3] = -1.0f;
    projMatrix[3 * 4 + 3] = 0.0f;
}

void visualization::configure_view(int view_width, int view_height)
{
    float aspect = 1.f*view_width/view_height;
    float nearclip = 0.1f;
    float farclip = 30.f;
    if(scale > farclip) farclip = scale*1.5f;
    if(scale < nearclip) nearclip = scale*0.75f;
    build_projection_matrix(projection_matrix, 60.0f, aspect, nearclip, farclip);

    for(int i = 0; i < 16; i++)
        view_matrix[i] = 0.f;
    view_matrix[0] = 1.f;
    view_matrix[5] = 1.f;
    view_matrix[10] = 1.f;
    view_matrix[15] = 1.f;
    // Translate by -scale in Z
    view_matrix[14] = -scale;
}

void visualization::scroll(GLFWwindow * window, double xoffset, double yoffset)
{
    scale *= (1 + (float)yoffset*.05f);
}

void visualization::keyboard(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    if(key == GLFW_KEY_0 && action == GLFW_PRESS)
        scale = initial_scale;
    if(key == GLFW_KEY_EQUAL && action == GLFW_PRESS)
        scale *= 1/1.1f;
    if(key == GLFW_KEY_MINUS && action == GLFW_PRESS)
        scale *= 1.1f;
}

static void error_callback(int error, const char* description)
{
    fputs(description, stderr);
}

void visualization::start()
{
    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_RESIZABLE, true);
    glfwWindowHint(GLFW_VISIBLE, true);
    GLFWwindow* main_window = glfwCreateWindow(width, height, "Visualization", NULL, NULL);
    if (!main_window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwShowWindow(main_window);
    glfwMakeContextCurrent(main_window);
    glfwSwapInterval(1);

    glfwSetKeyCallback(main_window, visualization::keyboard_callback);
    glfwSetInputMode(main_window, GLFW_STICKY_MOUSE_BUTTONS, 1);
    glfwSetScrollCallback(main_window, visualization::scroll_callback);

    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    r.gl_init();

    fprintf(stderr, "OpenGL Version %d.%d loaded\n", GLVersion.major, GLVersion.minor);

    while (!glfwWindowShouldClose(main_window))
    {
        glfwGetFramebufferSize(main_window, &width, &height);
        glViewport(0, 0, width, height);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Calculate layout
        int main_width = width, main_height = height;

        // Draw
        glViewport(0, 0, main_width, main_height);
        configure_view(main_width, main_height);
        r.gl_render(view_matrix, projection_matrix, data);
        glfwSwapBuffers(main_window);
        glfwPollEvents();
    }

    // Cleanup
    r.gl_destroy();
    glfwDestroyWindow(main_window);
    glfwTerminate();
}

visualization::visualization(render_data * _data)
    : data(_data), scale(initial_scale), width(640), height(480)
{
    static_gui = this;
}

visualization::~visualization()
{
}
