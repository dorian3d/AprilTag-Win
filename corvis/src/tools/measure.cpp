#include "../filter/replay.h"
#include "../vis/world_state.h"
#include "../vis/offscreen_render.h"
#include "../vis/gui.h"

int main(int c, char **v)
{
    if (0) { usage:
        cerr << "Usage: " << v[0] << " [--gui] [--realtime] [--no-plots] [--no-video] [--no-main] [--render <file.png>] <filename> <devicename>\n";
        return 1;
    }

    replay rp;

    world_state ws;

    bool realtime = false;
    bool enable_gui = false, show_plots = true, show_video = true, show_main = true;
    char *devicename = nullptr, *filename = nullptr, *rendername = nullptr;
    for (int i=1; i<c; i++)
        if      (v[i][0] != '-' && !filename) filename = v[i];
        else if (v[i][0] != '-' && !devicename) devicename = v[i];
        else if (strcmp(v[i], "--gui") == 0) enable_gui = true;
        else if (strcmp(v[i], "--realtime") == 0) realtime = true;
        else if (strcmp(v[i], "--no-plots") == 0) show_plots = false;
        else if (strcmp(v[i], "--no-video") == 0) show_video = false;
        else if (strcmp(v[i], "--no-main")  == 0) show_main  = false;
        else if (strcmp(v[i], "--render") == 0 && i+1 < c) rendername = v[++i];
        else goto usage;

    if (!filename || !devicename)
        goto usage;

    if(enable_gui)
        realtime = true;

    std::function<void (float)> progress;
    std::function<void (const filter *, camera_data &&)> camera_callback;

    gui vis(&ws, show_main, show_video, show_plots);

    // TODO: make this a command line option
    // For command line visualization
    if(rendername || enable_gui)
        camera_callback = [&](const filter * f, camera_data &&d) {
            ws.receive_camera(f, std::move(d));
        };

    if(!rp.configure_all(filename, devicename, realtime, progress, camera_callback)) return -1;
    
    if(enable_gui) { // The GUI must be on the main thread
        std::thread replay_thread([&](void) { rp.start(); });
        vis.start(&rp);
        replay_thread.join();
    }
    else
        rp.start();

    if(rendername && !offscreen_render_to_file(rendername, &ws)) {
        cerr << "Failed to render\n";
        return 1;
    }

    float length = rp.get_length();
    float path_length = rp.get_path_length();
    uint64_t packets_dispatched = rp.get_packets_dispatched();
    uint64_t bytes_dispatched = rp.get_bytes_dispatched();
    printf("Straight-line length is %.2f cm, total path length %.2f cm\n", length, path_length);
    printf("Dispatched %llu packets %.2f Mbytes\n", packets_dispatched, bytes_dispatched/1.e6);

    return 0;
}
