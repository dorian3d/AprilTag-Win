from numpy import *

def set_device_parameters(dc, config_name):
    if config_name == 'iphone4s':
        dc.Fx = 610.;
        dc.Fy = 610.;
        dc.Cx = 319.5;
        dc.Cy = 239.5;
        dc.px = 0.;
        dc.py = 0.;
        dc.K[0] = .20;
        dc.K[1] = -.55;
        dc.K[2] = 0.;
        dc.Tc[0] = 0.;
        dc.Tc[1] = 0.015;
        dc.Tc[2] = 0.;
        dc.Wc[0] = sqrt(2.)/2. * pi;
        dc.Wc[1] = -sqrt(2.)/2. * pi;
        dc.Wc[2] = 0.;
        a_bias_stdev = 0.2 * 9.8; # 20 mg
        w_bias_stdev = 10. / 180. * pi; # 10 dps
        for i in range(3):
            dc.a_bias[i] = 0.;
            dc.w_bias[i] = 0.;
            dc.a_bias_var[i] = 1.e-4; #a_bias_stdev * a_bias_stdev;
            dc.w_bias_var[i] = 1.e-4; #w_bias_stdev * w_bias_stdev;
            dc.Tc_var[i] = 1.e-6;
            dc.Wc_var[i] = 1.e-7;
        w_stdev = .03 * sqrt(50.) / 180. * pi; #.03 dps / sqrt(hz) at 50 hz
        dc.w_meas_var = w_stdev * w_stdev;
        a_stdev = .000218 * sqrt(50.) * 9.8; # 218 ug / sqrt(hz) at 50 hz
        dc.a_meas_var = a_stdev * a_stdev;
        dc.image_width = 640;
        dc.image_height = 480;
        dc.shutter_delay = 0;
        dc.shutter_period = 31000;
    
    elif config_name == 'iphone5':
        dc.Fx = 585.;
        dc.Fy = 585.;
        dc.Cx = 319.5;
        dc.Cy = 239.5;
        dc.px = 0.;
        dc.py = 0.;
        dc.K[0] = .10;
        dc.K[1] = -.10;
        dc.K[2] = 0.;
        dc.Tc[0] = 0.000;
        dc.Tc[1] = 0.000;
        dc.Tc[2] = -0.008;
        dc.Wc[0] = sqrt(2.)/2. * pi;
        dc.Wc[1] = -sqrt(2.)/2. * pi;
        dc.Wc[2] = 0.;
        a_bias_stdev = .02 * 9.8; # 20 mg
        w_bias_stdev = 10. / 180. * pi; #10 dps
        for i in range(3):
            dc.a_bias[i] = 0.;
            dc.w_bias[i] = 0.;
            dc.a_bias_var[i] = 1.e-4; #a_bias_stdev * a_bias_stdev;
            dc.w_bias_var[i] = 1.e-4; #w_bias_stdev * w_bias_stdev;
            dc.Tc_var[i] = 1.e-7;
            dc.Wc_var[i] = 1.e-7;
        w_stdev = .03 * sqrt(50.) / 180. * pi; #.03 dps / sqrt(hz) at 50 hz
        dc.w_meas_var = w_stdev * w_stdev;
        a_stdev = .000218 * sqrt(50.) * 9.8; # 218 ug / sqrt(hz) at 50 hz
        dc.a_meas_var = a_stdev * a_stdev;
        dc.image_width = 640;
        dc.image_height = 480;
        dc.shutter_delay = 0;
        dc.shutter_period = 31000;

    elif config_name == 'iphone5_jordan':
        dc.Fx = 585.;
        dc.Fy = 585.;
        dc.Cx = 319.5;
        dc.Cy = 239.5;
        dc.px = 0.;
        dc.py = 0.;
        dc.K[0] = .10;
        dc.K[1] = -.10;
        dc.K[2] = 0.;
        dc.Tc[0] = 0.000;
        dc.Tc[1] = 0.000;
        dc.Tc[2] = -0.008;
        dc.Wc[0] = sqrt(2.)/2. * pi;
        dc.Wc[1] = -sqrt(2.)/2. * pi;
        dc.Wc[2] = 0.;
        a_bias_stdev = .02 * 9.8; # 20 mg
        w_bias_stdev = 10. / 180. * pi; #10 dps
        for i in range(3):
            dc.a_bias[i] = 0.;
            dc.w_bias[i] = 0.;
            dc.a_bias_var[i] = 1.e-4; #a_bias_stdev * a_bias_stdev;
            dc.w_bias_var[i] = 1.e-4; #w_bias_stdev * w_bias_stdev;
            dc.Tc_var[i] = 1.e-7;
            dc.Wc_var[i] = 1.e-7;
        w_stdev = .03 * sqrt(50.) / 180. * pi; #.03 dps / sqrt(hz) at 50 hz
        dc.w_meas_var = w_stdev * w_stdev;
        a_stdev = .000218 * sqrt(50.) * 9.8; # 218 ug / sqrt(hz) at 50 hz
        dc.a_meas_var = a_stdev * a_stdev;
        dc.image_width = 640;
        dc.image_height = 480;
        dc.shutter_delay = 0;
        dc.shutter_period = 31000;

    elif config_name == 'iphone5_sam':
        dc.Fx = 585.;
        dc.Fy = 585.;
        dc.Cx = 319.5;
        dc.Cy = 239.5;
        dc.px = 0.;
        dc.py = 0.;
        dc.K[0] = .10;
        dc.K[1] = -.10;
        dc.K[2] = 0.;
        dc.Tc[0] = 0.000;
        dc.Tc[1] = 0.000;
        dc.Tc[2] = -0.008;
        dc.Wc[0] = sqrt(2.)/2. * pi;
        dc.Wc[1] = -sqrt(2.)/2. * pi;
        dc.Wc[2] = 0.;
        a_bias_stdev = .02 * 9.8; # 20 mg
        w_bias_stdev = 10. / 180. * pi; #10 dps
        for i in range(3):
            dc.a_bias[i] = 0.;
            dc.w_bias[i] = 0.;
            dc.a_bias_var[i] = 1.e-4; #a_bias_stdev * a_bias_stdev;
            dc.w_bias_var[i] = 1.e-4; #w_bias_stdev * w_bias_stdev;
            dc.Tc_var[i] = 1.e-7;
            dc.Wc_var[i] = 1.e-7;
        w_stdev = .03 * sqrt(50.) / 180. * pi; #.03 dps / sqrt(hz) at 50 hz
        dc.w_meas_var = w_stdev * w_stdev;
        a_stdev = .000218 * sqrt(50.) * 9.8; # 218 ug / sqrt(hz) at 50 hz
        dc.a_meas_var = a_stdev * a_stdev;
        dc.image_width = 640;
        dc.image_height = 480;
        dc.shutter_delay = 0;
        dc.shutter_period = 31000;

    elif config_name == 'ipad2':
        dc.Fx = 790.;
        dc.Fy = 790.;
        dc.Cx = 319.5;
        dc.Cy = 239.5;
        dc.px = 0.;
        dc.py = 0.;
        dc.K[0] = -1.2546e-1;
        dc.K[1] = 5.9923e-1;
        dc.K[2] = -.9888;
        dc.Tc[0] = -.015;
        dc.Tc[1] = .100;
        dc.Tc[2] = 0.;
        dc.Wc[0] = sqrt(2.)/2. * pi;
        dc.Wc[1] = -sqrt(2.)/2. * pi;
        dc.Wc[2] = 0.;
        a_bias_stdev = .02 * 9.8; # 20 mg
        w_bias_stdev = 10. / 180. * pi; # 10 dps
        for i in range(3):
            dc.a_bias[i] = 0.;
            dc.w_bias[i] = 0.;
            dc.a_bias_var[i] = 1.e-4; #a_bias_stdev * a_bias_stdev;
            dc.w_bias_var[i] = 1.e-4; #w_bias_stdev * w_bias_stdev;
            dc.Tc_var[i] = 1.e-6;
            dc.Wc_var[i] = 1.e-7;
        w_stdev = .03 * sqrt(50.) / 180. * pi; # .03 dps / sqrt(hz) at 50 hz
        dc.w_meas_var = w_stdev * w_stdev;
        a_stdev = .000218 * sqrt(50.) * 9.8; # 218 ug / sqrt(hz) at 50 hz
        dc.a_meas_var = a_stdev * a_stdev;
        dc.image_width = 640;
        dc.image_height = 480;
        dc.shutter_delay = 0;
        dc.shutter_period = 31000;

    elif config_name == 'ipad2_ben':
        dc.Fx = 790.;
        dc.Fy = 790.;
        dc.Cx = 319.5;
        dc.Cy = 239.5;
        dc.px = 0.;
        dc.py = 0.;
        dc.K[0] = -1.2546e-1;
        dc.K[1] = 5.9923e-1;
        dc.K[2] = -.9888;
        dc.Tc[0] = -.015;
        dc.Tc[1] = .100;
        dc.Tc[2] = 0.;
        dc.Wc[0] = sqrt(2.)/2. * pi;
        dc.Wc[1] = -sqrt(2.)/2. * pi;
        dc.Wc[2] = 0.;
        a_bias_stdev = .02 * 9.8; # 20 mg
        w_bias_stdev = 10. / 180. * pi; # 10 dps
        for i in range(3):
            dc.a_bias[i] = 0.;
            dc.w_bias[i] = 0.;
            dc.a_bias_var[i] = 1.e-4; #a_bias_stdev * a_bias_stdev;
            dc.w_bias_var[i] = 1.e-4; #w_bias_stdev * w_bias_stdev;
            dc.Tc_var[i] = 1.e-6;
            dc.Wc_var[i] = 1.e-7;
        w_stdev = .03 * sqrt(50.) / 180. * pi; # .03 dps / sqrt(hz) at 50 hz
        dc.w_meas_var = w_stdev * w_stdev;
        a_stdev = .000218 * sqrt(50.) * 9.8; # 218 ug / sqrt(hz) at 50 hz
        dc.a_meas_var = a_stdev * a_stdev;
        dc.image_width = 640;
        dc.image_height = 480;
        dc.shutter_delay = 0;
        dc.shutter_period = 31000;

    elif config_name == 'ipad2_brian':
        dc.Fx = 782.68;
        dc.Fy = 782.68;
        dc.Cx = 332.37;
        dc.Cy = 235.08;
        dc.px = 0.;
        dc.py = 0.;
        dc.K[0] = -0.033041;
        dc.K[1] = 0.18723;
        dc.K[2] = 0;
        dc.Tc[0] = -.0173;
        dc.Tc[1] = .1018;
        dc.Tc[2] = 0.048;
        dc.Wc[0] = sqrt(2.)/2. * pi;
        dc.Wc[1] = -sqrt(2.)/2. * pi;
        dc.Wc[2] = 0.;
        # Imu parameters a and w (alpha, omega)
        a_bias_stdev = .02 * 9.8; # 20 mg
        w_bias_stdev = 10. / 180. * pi; # 10 dps
        dc.a_bias[0] = 0.1898
        dc.a_bias[1] = -0.0868
        dc.a_bias[2] = 0.1118
        dc.w_bias[0] = 0.0019
        dc.w_bias[1] = 0.0082
        dc.w_bias[2] = -0.0070
        for i in range(3):
            dc.a_bias_var[i] = a_bias_stdev * a_bias_stdev;
            dc.w_bias_var[i] = w_bias_stdev * w_bias_stdev;
            dc.Tc_var[i] = 1.e-6;
            dc.Wc_var[i] = 1.e-7;
        w_stdev = .03 * sqrt(50.) / 180. * pi; # .03 dps / sqrt(hz) at 50 hz
        dc.w_meas_var = w_stdev * w_stdev;
        a_stdev = .000218 * sqrt(50.) * 9.8; # 218 ug / sqrt(hz) at 50 hz
        dc.a_meas_var = a_stdev * a_stdev;
        dc.image_width = 640;
        dc.image_height = 480;
        dc.shutter_delay = 0;
        dc.shutter_period = 31000;

    elif config_name == 'ipad3':
        dc.Fx = 620.;
        dc.Fy = 620.;
        dc.Cx = 319.5;
        dc.Cy = 239.5;
        dc.px = 0.;
        dc.py = 0.;
        dc.K[0] = .17;
        dc.K[1] = -.38;
        dc.K[2] = 0.;
        dc.Tc[0] = .05;
        dc.Tc[1] = .005;
        dc.Tc[2] = -.010;
        dc.Wc[0] = sqrt(2.)/2. * pi;
        dc.Wc[1] = -sqrt(2.)/2. * pi;
        dc.Wc[2] = 0.;
        a_bias_stdev = .02 * 9.8; # 20 mg
        w_bias_stdev = 10. / 180. * pi; # 10 dps
        for i in range(3):
            dc.a_bias[i] = 0.;
            dc.w_bias[i] = 0.;
            dc.a_bias_var[i] = 1.e-4; #a_bias_stdev * a_bias_stdev;
            dc.w_bias_var[i] = 1.e-4; #w_bias_stdev * w_bias_stdev;
            dc.Tc_var[i] = 1.e-7;
            dc.Wc_var[i] = 1.e-7;
        w_stdev = .03 * sqrt(50.) / 180. * pi; # .03 dps / sqrt(hz) at 50 hz
        dc.w_meas_var = w_stdev * w_stdev;
        a_stdev = .000218 * sqrt(50.) * 9.8; # 218 ug / sqrt(hz) at 50 hz
        dc.a_meas_var = a_stdev * a_stdev;
        dc.image_width = 640;
        dc.image_height = 480;
        dc.shutter_delay = 0;
        dc.shutter_period = 31000;

    elif config_name == 'ipadmini':
        dc.Fx = 590.;
        dc.Fy = 590.;
        dc.Cx = 319.5;
        dc.Cy = 239.5;
        dc.px = 0.;
        dc.py = 0.;
        dc.K[0] = .20;
        dc.K[1] = -.40;
        dc.K[2] = 0.;
        dc.Tc[0] = -.012;
        dc.Tc[1] = .047;
        dc.Tc[2] = .003;
        dc.Wc[0] = sqrt(2.)/2. * pi;
        dc.Wc[1] = -sqrt(2.)/2. * pi;
        dc.Wc[2] = 0.;
        a_bias_stdev = .02 * 9.8; # 20 mg
        w_bias_stdev = 10. / 180. * pi; # 10 dps
        for i in range(3):
            dc.a_bias[i] = 0.;
            dc.w_bias[i] = 0.;
            dc.a_bias_var[i] = 1.e-4; #a_bias_stdev * a_bias_stdev;
            dc.w_bias_var[i] = 1.e-4; #w_bias_stdev * w_bias_stdev;
            dc.Tc_var[i] = 1.e-7;
            dc.Wc_var[i] = 1.e-7;
        w_stdev = .03 * sqrt(50.) / 180. * pi; # .03 dps / sqrt(hz) at 50 hz
        dc.w_meas_var = w_stdev * w_stdev;
        a_stdev = .000218 * sqrt(50.) * 9.8; # 218 ug / sqrt(hz) at 50 hz
        dc.a_meas_var = a_stdev * a_stdev;
        dc.image_width = 640;
        dc.image_height = 480;
        dc.shutter_delay = 0;
        dc.shutter_period = 31000;

    elif config_name == 'ipad3-front':
        #parameters for generic ipad 3 - front cam
        dc.Fx = 604.;
        dc.Fy = 604.;
        dc.Cx = 319.5;
        dc.Cy = 239.5;
        dc.px = 0.
        dc.py = 0.
        dc.K[0] = 0.; #.2774956;
        dc.K[1] = 0.; #-1.0795446;
        dc.K[2] = 0.; #1.14524733;
        dc.a_bias[0] = 0.;
        dc.a_bias[1] = 0.;
        dc.a_bias[2] = 0.;
        dc.w_bias[0] = 0.;
        dc.w_bias[1] = 0.;
        dc.w_bias[2] = 0.;
        dc.Tc[0] = 0.#-.0940;
        dc.Tc[1] = 0.#.0396;
        dc.Tc[2] = 0.#.0115;
        dc.Wc[0] = 0.;
        dc.Wc[1] = 0.;
        dc.Wc[2] = -pi/2.;
        #dc.a_bias_var[0] = 1.e-4;
        #dc.a_bias_var[1] = 1.e-4;
        #dc.a_bias_var[2] = 1.e-4;
        #dc.w_bias_var[0] = 1.e-7;
        #dc.w_bias_var[1] = 1.e-7;
        #dc.w_bias_var[2] = 1.e-7;
        a_bias_stdev = .02 * 9.8; #20 mg
        dc.a_bias_var = a_bias_stdev * a_bias_stdev;
        w_bias_stdev = 10. / 180. * pi; #10 dps
        dc.w_bias_var = w_bias_stdev * w_bias_stdev;
        dc.Tc_var[0] = 1.e-6;
        dc.Tc_var[1] = 1.e-6;
        dc.Tc_var[2] = 1.e-6;
        dc.Wc_var[0] = 1.e-7;
        dc.Wc_var[1] = 1.e-7;
        dc.Wc_var[2] = 1.e-7;
        w_stdev = .03 * sqrt(50.) / 180. * pi; #.03 dps / sqrt(hz) at 50 hz
        dc.w_meas_var = w_stdev * w_stdev;
        a_stdev = .000218 * sqrt(50.) * 9.8; #218 ug / sqrt(hz) at 50 hz
        dc.a_meas_var = a_stdev * a_stdev;
        dc.image_width = 640;
        dc.image_height = 480;
        dc.shutter_delay = 0;
        dc.shutter_period = 31000;

    else:
       print "WARNING: Unrecognized configuration"
