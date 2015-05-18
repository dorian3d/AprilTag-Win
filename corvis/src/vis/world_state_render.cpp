#include "world_state_render.h"

#include <stdio.h>
#include <stdlib.h>

#include "gl_util.h"
#include "render.h"
#include "video_render.h"

static video_render frame_render;
static video_render plot_render;
static render render;

bool world_state_render_init()
{
    render.gl_init();
    glEnable(GL_DEPTH_TEST);

    return true;
}

void world_state_render_teardown()
{
    render.gl_destroy();
}

bool world_state_render_video_init()
{
    frame_render.gl_init();
    return true;
}

void world_state_render_video_teardown()
{
    frame_render.gl_destroy();
}

void world_state_render_video(world_state * world)
{
    glClear(GL_COLOR_BUFFER_BIT);
    world->display_lock.lock();
    world->image_lock.lock();
    frame_render.render(world->last_image.image, world->last_image.width, world->last_image.height, true);
    glLineWidth(2.0f);
    frame_render.draw_overlay(world->feature_ellipse_vertex, world->feature_ellipse_vertex_num, GL_LINES);
    world->image_lock.unlock();
    world->display_lock.unlock();
}

void world_state_render(world_state * world, float * viewMatrix, float * projMatrix)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    render.start_render(viewMatrix, projMatrix);

    world->display_lock.lock();

#if !(TARGET_OS_IPHONE)
    glPointSize(3.0f);
#endif
    glLineWidth(2.0f);
    render.draw_array(world->grid_vertex, world->grid_vertex_num, GL_LINES);
    glLineWidth(4.0f);
    render.draw_array(world->axis_vertex, world->axis_vertex_num, GL_LINES);
    render.draw_array(world->orientation_vertex, world->orientation_vertex_num, GL_LINES);
    render.draw_array(world->feature_vertex, world->feature_vertex_num, GL_POINTS);
    render.draw_array(world->path_vertex, world->path_vertex_num, GL_POINTS);

    world->display_lock.unlock();
}

bool world_state_render_plot_init()
{
    plot_render.gl_init();
    return true;
}

void world_state_render_plot_teardown()
{
    plot_render.gl_destroy();
}

static int plot_width = 600;
static int plot_height = 400;
static uint8_t * plot_frame = NULL;
#ifdef WIN32
static void create_plot() {}
#else // !WIN32

#include "lodepng.h"
#define _MSC_VER 1900 // Force mathgl to avoid C99's typeof
#include <mgl2/mgl.h>
#undef _MSC_VER

static void create_plot(world_state * state, int index)
{
    mglGraph gr(0,plot_width,plot_height); // 600x400 graph, plotted to an image
    if(!plot_frame)
        plot_frame = (uint8_t *)malloc(plot_width*plot_height*4*sizeof(uint8_t));

    // mglData stores the x and y data to be plotted
    state->render_plot(index, [&] (world_state::plot &plot) {
        gr.NewFrame();
        gr.Alpha(false);
        gr.Clf('w');
        gr.SubPlot(1,1,0,"T");
        gr.Box();
        float minx = std::numeric_limits<float>::max(), maxx = std::numeric_limits<float>::min();
        float miny = std::numeric_limits<float>::max(), maxy = std::numeric_limits<float>::min();
        std::string names;
        const char *colors[] = {"r","g","b"}; int i=0;

        for (auto &kv : plot) {
            const std::string &name = kv.first; const plot_data &p = kv.second;
            names += (names.size() ? " " : "") + name;

            mglData data_x(p.size());
            mglData data_y(p.size());
            auto first = sensor_clock::tp_to_micros(p.front().first);

            int j = 0;
            for(auto data : p) {
                float seconds = (sensor_clock::tp_to_micros(data.first) - first)/1e6;
                if(seconds < minx) minx = seconds;
                if(seconds > maxx) maxx = seconds;
                data_x.a[j] = seconds;

                float val = data.second;
                if(val < miny) miny = val;
                if(val > maxy) maxy = val;
                data_y.a[j++] = val;
            }

            gr.SetRange('x', minx, maxx);
            gr.SetRange('y', miny, maxy);
            gr.Plot(data_x, data_y, colors[i++%3]);
        }


        gr.Axis();
        gr.Title(names.c_str(),"k",6);
        gr.EndFrame();
        gr.GetRGBA((char *)plot_frame, plot_width*plot_height*4);

        //Encode the image
        //std::string filename = names + ".png";
        //unsigned error = lodepng::encode(filename.c_str(), plot_frame, plot_width, plot_height);
        //if(error)
        //    fprintf(stderr, "encoder error %d: %s\n", error, lodepng_error_text(error));
    });
}
#endif //WIN32

void world_state_render_plot(world_state * world, int index)
{
    glClear(GL_COLOR_BUFFER_BIT);
    create_plot(world, index);
    if(plot_frame)
        plot_render.render(plot_frame, plot_width, plot_height, false);
}
