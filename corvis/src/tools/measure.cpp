#include "../filter/replay.h"
#include "../filter/calibration_json_store.h"
#include "../vis/world_state.h"
#include "../vis/offscreen_render.h"
#include "../vis/gui.h"
#include "benchmark.h"
#include <iomanip>

int main(int c, char **v)
{
    if (0) { usage:
        cerr << "Usage: " << v[0] << " [--qvga] [--no-depth] [--realtime] [--pause] [--no-gui] [--no-plots] [--no-video] [--no-main] [--render <file.png>] [--save <calibration-json>] <filename>\n";
        cerr << "       " << v[0] << " [--qvga] [--no-depth] --benchmark <directory>\n";
        return 1;
    }

    bool realtime = false, start_paused = false, benchmark = false;
    std::string save;
    bool qvga = false, depth = true;
    bool enable_gui = true, show_plots = false, show_video = true, show_depth = true, show_main = true;
    char *filename = nullptr, *rendername = nullptr;
    for (int i=1; i<c; i++)
        if      (v[i][0] != '-' && !filename) filename = v[i];
        else if (strcmp(v[i], "--no-gui") == 0) enable_gui = false;
        else if (strcmp(v[i], "--realtime") == 0) realtime = true;
        else if (strcmp(v[i], "--no-realtime") == 0) realtime = false;
        else if (strcmp(v[i], "--no-plots") == 0) show_plots = false;
        else if (strcmp(v[i], "--no-depth") == 0) show_depth = false;
        else if (strcmp(v[i], "--no-video") == 0) show_video = false;
        else if (strcmp(v[i], "--no-main")  == 0) show_main  = false;
        else if (strcmp(v[i], "--pause")  == 0) start_paused  = true;
        else if (strcmp(v[i], "--render") == 0 && i+1 < c) rendername = v[++i];
        else if (strcmp(v[i], "--qvga") == 0) qvga = true;
        else if (strcmp(v[i], "--drop-depth") == 0) depth = false;
        else if (strcmp(v[i], "--save") == 0 && i+1 < c) save = v[++i];
        else if (strcmp(v[i], "--benchmark") == 0) benchmark = true;
        else goto usage;

    if (!filename)
        goto usage;

    auto configure = [&](replay &rp, const char *capture_file) -> bool {
        if(qvga) rp.enable_qvga();
        if(!depth) rp.disable_depth();
        if(realtime) rp.enable_realtime();

        if(!rp.open(capture_file))
            return false;

        if(!rp.set_calibration_from_filename(capture_file)) {
          cerr << "calibration not found: " << capture_file << ".json nor calibration.json\n";
          return false;
        }

        if(!rp.set_reference_from_filename(capture_file) && !enable_gui) {
            cerr << capture_file << ": unable to find a reference to measure against\n";
            return false;
        }

        return true;
    };

    auto print_results = [](replay &rp, const char *capture_file) {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Reference Straight-line length is " << 100*rp.get_reference_length() << " cm, total path length " << 100*rp.get_reference_path_length() << " cm\n";
        std::cout << "Computed  Straight-line length is " << 100*rp.get_length()           << " cm, total path length " << 100*rp.get_path_length()           << " cm\n";
        std::cout << "Dispatched " << rp.get_packets_dispatched() << " packets " << rp.get_bytes_dispatched()/1.e6 << " Mbytes\n";
    };

    if (benchmark) {
        enable_gui = false; if (realtime || start_paused) goto usage;

        benchmark_run(std::cout, filename, [&](const char *capture_file, struct benchmark_result &res) -> bool {
            std::unique_ptr<replay> rp_ = std::make_unique<replay>(start_paused); replay &rp = *rp_; // avoid blowing the stack when threaded

            if (!configure(rp, capture_file)) return false;

            std::cout << "Running  " << capture_file << std::endl;
            rp.start();
            std::cout << "Finished " << capture_file << std::endl;

            res.length_cm.reference = 100*rp.get_reference_length();  res.path_length_cm.reference = 100*rp.get_reference_path_length();
            res.length_cm.measured  = 100*rp.get_length();            res.path_length_cm.measured  = 100*rp.get_path_length();

            print_results(rp, capture_file);

            return true;
        });
        return 0;
    }

    replay rp(start_paused);

    if (!configure(rp, filename))
        return 2;

#if defined(ANDROID) || defined(WIN32)
    rp.start();
#else
    world_state ws;
    gui vis(&ws, show_main, show_video, show_depth, show_plots);
    rp.set_camera_callback([&](const filter * f, camera_data &&d) {
        ws.receive_camera(f, std::move(d));
    });

    if(enable_gui) { // The GUI must be on the main thread
        std::thread replay_thread([&](void) { rp.start(); });
        vis.start(&rp);
        rp.stop();
        replay_thread.join();
    } else
        rp.start();

    if(rendername && !offscreen_render_to_file(rendername, &ws)) {
        cerr << "Failed to render\n";
        return 1;
    }
    std::cout << ws.get_feature_stats();
#endif

    if (!save.empty()) {
        std::string json;
        if (calibration_serialize(rp.get_device_parameters(), json)) {
            std::ofstream out(save);
            out << json;
        }
    }

    print_results(rp, filename);
    return 0;
}
