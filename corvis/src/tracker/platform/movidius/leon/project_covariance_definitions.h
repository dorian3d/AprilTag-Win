#ifndef PROJECT_COVARIANCE_DEFINITIONS_H_
#define PROJECT_COVARIANCE_DEFINITIONS_H_

#define PROJECT_COVARIANCE_FIRST_SHAVE 0
#define PROJECT_COVARIANE_SHAVES 4

#define MAX_DATA_SIZE 96

struct project_covariance_element_data{
    int index;
    float initial_covariance;
    bool use_single_index;
};

struct project_motion_covariance_data{
    int first_shave = -1;
    int shaves_number = 0;
    const float *src = nullptr;
    int src_rows = 0;
    int src_cols = 0;
    int src_stride = 0;
    float *dst = nullptr;
    int dst_rows = 0;
    int dst_cols = 0;
    int dst_stride = 0;
    project_covariance_element_data w = {0, 0, 0};
    project_covariance_element_data dw = {0, 0, 0};
    project_covariance_element_data ddw = {0, 0, 0};
    project_covariance_element_data V = {0, 0, 0};
    project_covariance_element_data a = {0, 0, 0};
    project_covariance_element_data T = {0, 0, 0};
    project_covariance_element_data da = {0, 0, 0};
    project_covariance_element_data Q = {0, 0, 0};
    const float *dQp_s_dW = NULL;
    int camera_count = 0;
    project_covariance_element_data tr[MAX_DATA_SIZE] = { {0, 0, 0} };
    project_covariance_element_data qr[MAX_DATA_SIZE] = { {0, 0, 0} };
    float* dTrp_dQ_s_matrix[MAX_DATA_SIZE] = {NULL};
    float* dQrp_s_dW_matrix[MAX_DATA_SIZE] = {NULL};
    float* dTrp_ddT_matrix[MAX_DATA_SIZE] = {NULL};
    float dt = 0.f;
};

struct observation_data {
    int size = 0;
    int type = 0;

    observation_data(int _size=0, int _type=0) : size(_size), type(_type) {}
};

struct observation_vision_feature_data : observation_data {

    project_covariance_element_data feature = {0, 0, 0};
    project_covariance_element_data Qr = {0, 0, 0};
    project_covariance_element_data Tr = {0, 0, 0};

    float* dx_dp = NULL; //m<2,1>
    float* dx_dQr = NULL; //m<2,3>
    float* dx_dTr = NULL; //m<2,3>

    struct camera_derivative {
        project_covariance_element_data Q = {0, 0, 0};
        project_covariance_element_data T = {0, 0, 0};
        project_covariance_element_data focal_length = {0, 0, 0};
        project_covariance_element_data center = {0, 0, 0};
        project_covariance_element_data k = {0, 0, 0};

        bool e_estimate = true;
        bool i_estimate = true;

        float* dx_dQ = NULL; //m<2,3>
        float* dx_dT = NULL; //m<2,3>
        float* dx_dF = NULL; //m<2,1>
        float* dx_dc = NULL; //m<2,2>
        float* dx_dk = NULL; //m<2,4>

    } orig, curr;

    observation_vision_feature_data() : observation_data (0, 1) {}
    observation_vision_feature_data(observation_data& base) : observation_data (base) {}
};

struct observation_accelerometer_data : observation_data {

    project_covariance_element_data a_bias = {0, 0, 0};
    project_covariance_element_data Q = {0, 0, 0};
    project_covariance_element_data a = {0, 0, 0};
    project_covariance_element_data w = {0, 0, 0};
    project_covariance_element_data dw = {0, 0, 0};
    project_covariance_element_data g = {0, 0, 0};
    project_covariance_element_data eQ = {0, 0, 0};
    project_covariance_element_data eT = {0, 0, 0};

    float* da_dQ = NULL; //m3
    float* da_dw = NULL; //m3
    float* da_ddw = NULL; //m3
    float* da_dacc = NULL; //m3
    float* da_dQa = NULL; //m3
    float* da_dTa = NULL; //m3
    float* worldUp = NULL; //v3
    bool e_estimate = true;

    observation_accelerometer_data() : observation_data (0, 2) {}
    observation_accelerometer_data(observation_data& base) : observation_data (base) {}
};

struct observation_gyroscope_data : observation_data {

    project_covariance_element_data w = {0, 0, 0};
    project_covariance_element_data w_bias = {0, 0, 0};
    project_covariance_element_data Q = {0, 0, 0};

    float* RwT = NULL; //m3 //Rw.transpose();
    float* dw_dQw = NULL; //m3
    bool e_estimate = true;

    observation_gyroscope_data() : observation_data (0, 3) {}
    observation_gyroscope_data(observation_data& base) : observation_data (base) {}
};

struct project_observation_covariance_data {
    int first_shave = -1;
    int shaves_number = 0;
    const float *src = nullptr;
    int src_rows = 0;
    int src_cols = 0;
    int src_stride = 0;
    float *dst = nullptr;
    int dst_rows = 0;
    int dst_cols = 0;
    int dst_stride = 0;
    float *HP = nullptr;
    int HP_rows = 0;
    int HP_src_cols = 0;
    int HP_dst_cols = 0;
    int HP_stride = 0;

    observation_data** observations = NULL;
    int observations_size = 0;
};

#endif /* PROJECT_COVARIANCE_DEFINITIONS_H_ */
