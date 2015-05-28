#ifndef WIN32
#include <alloca.h>
#endif
#include <limits>
#include "gui.h"
gui * gui::static_gui;

#include "world_state_render.h"
#include "../filter/replay.h"
#include "gl_util.h"

const static float initial_scale = 5;

void gui::configure_view(int view_width, int view_height)
{
    float aspect = 1.f*view_width/view_height;
    float nearclip = 0.1f;
    float farclip = 30.f;
    if(scale > farclip) farclip = scale*1.5f;
    if(scale < nearclip) nearclip = scale*0.75f;
    build_projection_matrix(_projectionMatrix, 60.0f, aspect, nearclip, farclip);

    m4 R = to_rotation_matrix(arc.get_quaternion());
    R(2, 3) = -scale; // Translate by -scale
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            _modelViewMatrix[j * 4 + i] = (float)R(i, j);
        }
    }
}

void gui::mouse_move(GLFWwindow * window, double x, double y)
{
    if(is_rotating)
        arc.continue_rotation((float)x, (float)y);
}

void gui::mouse(GLFWwindow * window, int button, int action, int mods)
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        arc.start_rotation((float)x, (float)y);
        is_rotating = true;
    }
    else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        is_rotating = false;
    }
}

void gui::scroll(GLFWwindow * window, double xoffset, double yoffset)
{
    scale *= (1 + (float)yoffset*.05f);
}

#include "lodepng.h"
void gui::write_frame()
{
    state->image_lock.lock();
    const int W = state->last_image.width;
    const int H = state->last_image.height;
    auto image = (uint8_t*)alloca(W*H*4);
    for(int i = 0; i < W*H; i++) {
        image[i*4 + 0] = state->last_image.image[i];
        image[i*4 + 1] = state->last_image.image[i];
        image[i*4 + 2] = state->last_image.image[i];
        image[i*4 + 3] = 255;
    }

    state->image_lock.unlock();

    //Encode the image
    unsigned error = lodepng::encode("last_frame.png", image, W, H);
    if(error)
        fprintf(stderr, "encoder error %d: %s\n", error, lodepng_error_text(error));
}

void gui::keyboard(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    if(key == GLFW_KEY_0 && action == GLFW_PRESS)
        scale = initial_scale;
    if(key == GLFW_KEY_EQUAL && action == GLFW_PRESS)
        scale *= 1/1.1f;
    if(key == GLFW_KEY_MINUS && action == GLFW_PRESS)
        scale *= 1.1f;
    if(key == GLFW_KEY_SPACE && action == GLFW_PRESS)
       replay_control->toggle_pause();
    if(key == GLFW_KEY_S && action == GLFW_PRESS)
       replay_control->step();
    if(key == GLFW_KEY_F && action == GLFW_PRESS)
       write_frame();
    if(key == GLFW_KEY_N && action == GLFW_PRESS)
       current_plot = state->next_plot(current_plot);
    if(key == GLFW_KEY_V && action == GLFW_PRESS)
        show_video = !show_video;
    if(key == GLFW_KEY_M && action == GLFW_PRESS)
        show_main = !show_main;
    if(key == GLFW_KEY_P && action == GLFW_PRESS)
        show_plots = !show_plots;
}

void gui::render(int view_width, int view_height)
{
}

static void error_callback(int error, const char* description)
{
    fputs(description, stderr);
}

void gui::start_glfw()
{
    if (!glfwInit())
        exit(EXIT_FAILURE);

    int pos_x = 50;
    int pos_y = 50;

    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_RESIZABLE, true);
    glfwWindowHint(GLFW_VISIBLE, true);
    GLFWwindow* main_window = glfwCreateWindow(width, height, "Replay", NULL, NULL);
    if (!main_window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwSetWindowPos(main_window, pos_x, pos_y);
    if(show_main)
        glfwShowWindow(main_window);
    glfwMakeContextCurrent(main_window);
    glfwSwapInterval(1);

    glfwSetKeyCallback(main_window, gui::keyboard_callback);
    glfwSetInputMode(main_window, GLFW_STICKY_MOUSE_BUTTONS, 1);
    glfwSetMouseButtonCallback(main_window, gui::mouse_callback);
    glfwSetCursorPosCallback(main_window, gui::move_callback);
    glfwSetScrollCallback(main_window, gui::scroll_callback);

    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    world_state_render_init();
    world_state_render_video_init();
    world_state_render_plot_init();

    //fprintf(stderr, "OpenGL Version %d.%d loaded\n", GLVersion.major, GLVersion.minor);

    while (!glfwWindowShouldClose(main_window))
    {
        glfwGetFramebufferSize(main_window, &width, &height);
        glViewport(0, 0, width, height);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Calculate layout
        int main_width = width, main_height = height;
        int video_width = 0, video_height = 0;
        int plots_width = 0, plots_height = 0;
        float right_column_percent = .5f;
        float video_height_percent = .5f;
        float plots_height_percent = 1.f - video_height_percent;

        if(!show_main)
            right_column_percent = 1.f;
        if(!show_plots)
            video_height_percent = 1.f;
        if(!show_video)
            plots_height_percent = .5f;
        if(show_video) {
            int frame_width = world_state_render_video_width(state);
            int frame_height = world_state_render_video_height(state);
            video_width = width*right_column_percent;
            video_height = height*video_height_percent;
            if(1.*video_height/video_width > 1.f*frame_height/frame_width)
                video_height = video_width *  1.f*frame_height/frame_width;
            else
                video_width = video_height * 1.f*frame_width/frame_height;
        }
        if(show_plots) {
            plots_width = width*right_column_percent;
            plots_height = height - video_height;
            if(show_video)
                plots_width = video_width;
            if(!show_video && show_main)
                plots_height = height*plots_height_percent;
        }
        if(show_main && show_video && show_plots)
            main_width = width - video_width;

        // Update data
        state->update_vertex_arrays();

        // Draw
        if(show_main) {
            glViewport(0, 0, main_width, main_height);
            configure_view(main_width, main_height);
            world_state_render(state, _modelViewMatrix, _projectionMatrix);
        }
        if(show_video) {
            // y coordinate is 0 = bottom, height = top (opengl)
            glViewport(width - video_width, 0, video_width, video_height);
            world_state_render_video(state, video_width, video_height);
        }
        if(show_plots) {
            // y coordinate is 0 = bottom, height = top (opengl)
            glViewport(width - plots_width, video_height, plots_width, plots_height);
            world_state_render_plot(state, current_plot, plots_width, plots_height);
        }
        glfwSwapBuffers(main_window);
        glfwPollEvents();
    }
    glfwDestroyWindow(main_window);
    glfwTerminate();
    std::cerr << "GUI closed, replay still running in realtime (press Ctrl+c to quit)\n";
}


void gui::start(replay * rp)
{
    replay_control = rp;
    arc.reset();
    start_glfw();
}

gui::gui(world_state * world, bool show_main_, bool show_video_, bool show_plots_)
    : state(world), scale(initial_scale), width(640), height(480),
      show_main(show_main_), show_video(show_video_), show_plots(show_plots_)
{
    static_gui = this;
}

gui::~gui()
{
}
