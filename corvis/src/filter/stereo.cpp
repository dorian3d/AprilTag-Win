#include "stereo.h"
#include "stereo_mesh.h"
#include "filter.h"

bool debug_triangulate = false;
bool debug_info = true;
bool debug_frames = true;
// if enabled, adds a 3 pixel jitter in all directions to correspondence
bool enable_jitter = false;

void write_image(const char * path, uint8_t * image, int width, int height)
{
    FILE * f = fopen(path, "w");
    fprintf(f, "P5 %d %d 255\n", width, height);
    fwrite(image, 1, width*height, f);
    fclose(f);
}

bool line_endpoints(v4 line, int width, int height, float endpoints[4])
{
    float a = line[0], b = line[1], c = line[2];
    float intersect_y, intersect_x;
    float min_x = 0, max_x = width-1, min_y = 0, max_y = height-1;
    int idx = 0;

    f_t eps = 1e-14;
    // if the line is not vertical
    if(fabs(a) > eps) {
        // intersection with the left edge
        intersect_y = -(a * min_x + c) / b;
        if(intersect_y >= min_y && intersect_y <= max_y) {
            endpoints[idx++] = min_x;
            endpoints[idx++] = intersect_y;
        }

        // intersection with the right edge
        intersect_y = -(a * max_x + c) / b;
        if(intersect_y >= min_y && intersect_y <= max_y) {
            endpoints[idx++] = max_x;
            endpoints[idx++] = intersect_y;
        }
    }

    // if the line is not horizontal
    if(fabs(b) > eps) {
        // intersection with the top edge
        if(idx < 3) {
            intersect_x = -(b * min_y + c) / a;
            if(intersect_x >= min_x && intersect_x <= max_x) {
                endpoints[idx++] = intersect_x;
                endpoints[idx++] = min_y;
            }
        }

        // intersection with the bottom edge
        if(idx < 3) {
            intersect_x = -(b * max_y + c) / a;
            if(intersect_x >= min_x && intersect_x <= max_x) {
                endpoints[idx++] = intersect_x;
                endpoints[idx++] = max_y;
            }
        }
    }

    // if we still haven't found an intersection, return false
    if(idx < 3)
        return false;

    return true;
}

#define WINDOW 10
static const float maximum_match_score = -0.5;
// 5 pixels average deviation from the mean across the patch
static const float constant_patch_thresh = 5*5*(WINDOW*2 + 1)*(WINDOW*2 + 1);
float score_match(const unsigned char *im1, int xsize, int ysize, int stride, const int x1, const int y1, const unsigned char *im2, const int x2, const int y2, float max_error)
{
    int window = WINDOW;
    int area = (WINDOW*2 + 1) * (WINDOW * 2 + 1);
    
    if(x1 < window || y1 < window || x2 < window || y2 < window || x1 >= xsize - window || x2 >= xsize - window || y1 >= ysize - window || y2 >= ysize - window) return max_error + 1.;

    const unsigned char *p1 = im1 + stride * (y1 - window) + x1;
    const unsigned char *p2 = im2 + stride * (y2 - window) + x2;

    int sum1 = 0, sum2 = 0;
    for(int dy = -window; dy <= window; ++dy, p1+=stride, p2+=stride) {
        for(int dx = -window; dx <= window; ++dx) {
            sum1 += p1[dx];
            sum2 += p2[dx];
        }
    };
    
    float mean1 = sum1 / (float)area;
    float mean2 = sum2 / (float)area;
    
    p1 = im1 + stride * (y1 - window) + x1;
    p2 = im2 + stride * (y2 - window) + x2;
    float top = 0, bottom1 = 0, bottom2 = 0;
    for(int dy = -window; dy <= window; ++dy, p1+=stride, p2+=stride) {
        for(int dx = -window; dx <= window; ++dx) {
            float t1 = (float)p1[dx] - mean1;
            float t2 = (float)p2[dx] - mean2;
            top += t1 * t2;
            bottom1 += (t1 * t1);
            bottom2 += (t2 * t2);
        }
    }
    // constant patches can't be matched
    if(fabs(bottom1) < constant_patch_thresh || fabs(bottom2) < constant_patch_thresh)
      return max_error + 1.;

    return -top/sqrtf(bottom1 * bottom2);
}

bool track_window(uint8_t * im1, uint8_t * im2, int width, int height, int im1_x, int im1_y, int upper_left_x, int upper_left_y, int lower_right_x, int lower_right_y, int & bestx, int & besty, float & bestscore)
{
    bool valid_match = false;
    for(int y = upper_left_y; y < lower_right_y; y++) {
        for(int x = upper_left_x; x < lower_right_x; x++) {
            float score = score_match(im1, width, height, width, im1_x, im1_y, im2, x, y, maximum_match_score);
            if(score < bestscore) {
                valid_match = true;
                bestscore = score;
                bestx = x;
                besty = y;
            }
        }
    }
    return valid_match;
}

bool track_line(uint8_t * im1, uint8_t * im2, int width, int height, int im1_x, int im1_y, int x0, int y0, int x1, int y1, int & bestx, int & besty, float & bestscore)
{
    int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1; 
    int err = (dx>dy ? dx : -dy)/2, e2;

    bool valid_match = false;
    bestscore = maximum_match_score;

    while(true) {
        float score = score_match(im1, width, height, width, im1_x, im1_y, im2, x0, y0, maximum_match_score);

        if(score < bestscore) {
          valid_match = true;
          bestscore = score;
          bestx = x0;
          besty = y0;
        }

        // move along the line
        if (x0==x1 && y0==y1) break;
        e2 = err;
        if (e2 >-dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
    }

    return valid_match;
}

f_t estimate_kr(v4 point, f_t k1, f_t k2, f_t k3)
{
    f_t r2 = point[0]*point[0] + point[1]*point[1];
    f_t r4 = r2 * r2;
    f_t r6 = r4 * r2;
    f_t kr = 1. + r2 * k1 + r4 * k2 + r6 * k3;
    return kr;
}

v4 calibrate_im_point(v4 normalized_point, float k1, float k2, float k3)
{
    f_t kr;
    v4 calibrated_point = normalized_point;
    //forward calculation - guess calibrated from initial
    kr = estimate_kr(normalized_point, k1, k2, k3);
    calibrated_point[0] /= kr;
    calibrated_point[1] /= kr;
    //backward calculation - use calibrated guess to get new parameters and recompute
    kr = estimate_kr(calibrated_point, k1, k2, k3);
    calibrated_point = normalized_point;
    calibrated_point[0] /= kr;
    calibrated_point[1] /= kr;
    return calibrated_point;
}

v4 project_point(f_t x, f_t y, f_t center_x, f_t center_y, float focal_length)
{
    return v4((x - center_x)/focal_length, (y - center_y)/focal_length, 1, 1);
}

// From http://paulbourke.net/geometry/pointlineplane/lineline.c
bool line_line_intersect(v4 p1, v4 p2, v4 p3, v4 p4, v4 & pa, v4 & pb)
{
    v4 p13,p43,p21;
    double d1343,d4321,d1321,d4343,d2121;
    double numer,denom;

    f_t eps = 1e-14;

    p13 = p1 - p3;
    p43 = p4 - p3;
    if (fabs(p43[0]) < eps && fabs(p43[1]) < eps && fabs(p43[2]) < eps)
      return false;

    p21 = p2 - p1;
    if (fabs(p21[0]) < eps && fabs(p21[1]) < eps && fabs(p21[2]) < eps)
      return false;

    d1343 = sum(p13 * p43); //p13.x * p43.x + p13.y * p43.y + p13.z * p43.z;
    d4321 = sum(p43 * p21); //p43.x * p21.x + p43.y * p21.y + p43.z * p21.z;
    d1321 = sum(p13 * p21); //p13.x * p21.x + p13.y * p21.y + p13.z * p21.z;
    d4343 = sum(p43 * p43); //p43.x * p43.x + p43.y * p43.y + p43.z * p43.z;
    d2121 = sum(p21 * p21); //p21.x * p21.x + p21.y * p21.y + p21.z * p21.z;

    denom = d2121 * d4343 - d4321 * d4321;
    if (fabs(denom) < eps)
      return false;
    numer = d1343 * d4321 - d1321 * d4343;

    float mua = numer / denom;
    float mub = (d1343 + d4321 * mua) / d4343;

    pa = p1 + mua*p21;
    pb = p3 + mub*p43;
    // Return homogeneous points
    pa[3] = 1; 
    pb[3] = 1;

    return true;
}

//TODO: estimate_F doesnt agree with eight point F. This is now correct for F corresponding to X2 = R * X1 + T

m4 estimate_F(const struct stereo_global &g, const stereo_frame &reference, const stereo_frame &target)
{
    /*
    x1_w = R1 * x1 + T1
    x2 = R2^t * (R1 * x1 + T1 - T2)
    
    R21 = R2^t * R1
    T21 = R2^t * (T1 - T2)
    */
    m4 R1w = to_rotation_matrix(reference.W);
    m4 R2w = to_rotation_matrix(target.W);
    m4 dR = transpose(R2w)*R1w;
    v4 dT = transpose(R2w) * (reference.T - target.T);

    // E21 is 3x3
    m4 E21 = skew3(dT) * dR;

    m4 Kinv;
    Kinv[0][0] = 1./g.focal_length;
    Kinv[1][1] = 1./g.focal_length;
    Kinv[0][2] = -g.center_x/g.focal_length;
    Kinv[1][2] = -g.center_y/g.focal_length;
    Kinv[2][2] = 1;
    Kinv[3][3] = 1;

    m4 F21 = transpose(Kinv)*E21*Kinv;

    return F21;
}

// F is from reference to target
bool find_correspondence(const stereo_frame & reference, const stereo_frame & target, const m4 &F, int reference_x, int reference_y, int & target_x, int & target_y, int width, int height, float & correspondence_score)
{
    v4 p1 = v4(reference_x, reference_y, 1, 0);

    // p2 should lie on this line
    v4 l1 = p1*transpose(F);

    // ground truth sanity check
    // Normalize the line equation so that distances can be computed
    // with the dot product
    //l1 = l1 / sqrt(l1[0]*l1[0] + l1[1]*l1[1]);
    //float d = sum(l1*p2);
    //fprintf(stderr, "distance p1 %f\n", d);

    bool success = false;
    float endpoints[4];
    if(line_endpoints(l1, width, height, endpoints)) {
        success = track_line(reference.image, target.image, width, height, p1[0], p1[1],
                                 endpoints[0], endpoints[1], endpoints[2], endpoints[3],
                                 target_x, target_y, correspondence_score);
        if(enable_jitter && success) {
            int upper_left_x = target_x - 3;
            int upper_left_y = target_y - 3;
            int lower_right_x = target_x + 3;
            int lower_right_y = target_y + 3;
            // if this function returns true, then we have changed target_x and target_y to a new value.
            // This happens in most cases, likely due to camera distortion
            track_window(reference.image, target.image, width, height, p1[0], p1[1], upper_left_x, upper_left_y, lower_right_x, lower_right_y, target_x, target_y, correspondence_score);
        }
    }

    return success;
}

m4 eight_point_F(v4 p1[], v4 p2[], int npts)
{
    // normalize points
    v4 center1 = v4(0., 0., 0., 0.);
    v4 center2 = v4(0., 0., 0., 0.);
    float norm1 = 0, norm2 = 0;
    for(int i = 0; i < npts; i++) {
        center1 += p1[i];
        center2 += p2[i];
    }
    center1 /= npts;
    center2 /= npts;
    for(int i = 0; i < npts; i++) {
        norm1 += sum((p1[i] - center1)*(p1[i] - center1));
        norm2 += sum((p2[i] - center1)*(p2[i] - center2));
    }
    norm1 = sqrt(2.*npts / norm1);
    norm2 = sqrt(2.*npts / norm2);

    matrix T1(3, 3);
    matrix T2(3, 3);
    T1(0, 0) = norm1; T1(0, 1) = 0;     T1(0, 2) = -norm1*center1[0];
    T1(1, 0) = 0;     T1(1, 1) = norm1; T1(1, 2) = -norm1*center1[1];
    T1(2, 0) = 0;     T1(2, 1) = 0;     T1(2, 2) = 1;


    T2(0, 0) = norm2; T2(0, 1) = 0;     T2(0, 2) = -norm2*center2[0];
    T2(1, 0) = 0;     T2(1, 1) = norm2; T2(1, 2) = -norm2*center2[1];
    T2(2, 0) = 0;     T2(2, 1) = 0;     T2(2, 2) = 1;

    matrix constraints(npts, 9);
    for(int i = 0; i < npts; i++) {
        v4 p1n = (p1[i] - center1) * norm1;
        v4 p2n = (p2[i] - center2) * norm2;
        constraints(i, 0) = p1n[0]*p2n[0];
        constraints(i, 1) = p1n[1]*p2n[0];
        constraints(i, 2) = p2n[0];
        constraints(i, 3) = p1n[0]*p2n[1];
        constraints(i, 4) = p1n[1]*p2n[1];
        constraints(i, 5) = p2n[1];
        constraints(i, 6) = p1n[0];
        constraints(i, 7) = p1n[1];
        constraints(i, 8) = 1;
    }


    // some columns of U are inverted compared to matlab
    matrix U(npts, npts);
    matrix S(1, min(npts, 9));
    matrix Vt(9, 9);

    matrix_svd(constraints, U, S, Vt);

    matrix F(3, 3);
    F(0, 0) = Vt(8, 0); F(0, 1) = Vt(8, 1); F(0, 2) = Vt(8, 2);
    F(1, 0) = Vt(8, 3); F(1, 1) = Vt(8, 4); F(1, 2) = Vt(8, 5);
    F(2, 0) = Vt(8, 6); F(2, 1) = Vt(8, 7); F(2, 2) = Vt(8, 8);

    matrix UF(3, 3);
    matrix SF(1, 3);
    matrix VFt(3, 3);
    matrix_svd(F, UF, SF, VFt);
    matrix SE(3, 3);
    // Convert S back to a diagonal matrix (matrix_svd fills a vector)
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
        {
            if(i == j)
                SE(i,j) = SF(0,i);
            else
                SE(i,j) = 0;
        }
    SE(2,2) = 0;
    
    // TODO: there is probably a cleaner way to do this:
    // F = transpose(T2) * U * S * Vt * T1;
    matrix temp1(3, 3);
    matrix temp2(3, 3);
    matrix_product(temp1, VFt, T1);
    matrix_product(temp2, SE, temp1); 
    matrix_product(temp1, UF, temp2); 
    matrix_product(temp2, T2, temp1, true); // transpose T2
    F = temp2;

    // F = F / norm(F) / sign(F(3,3))
    float Fnorm = 0;
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
            Fnorm += F(i,j)*F(i,j);
    Fnorm = sqrt(Fnorm);

    if(F(2,2) < 0)
      Fnorm = -Fnorm;

    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
            F(i,j) = F(i,j) / Fnorm;

    m4 estimatedF;
    for(int i = 0; i < 3; i++)
      for(int j = 0; j < 3; j++)
        estimatedF[i][j] = F(i,j);
    estimatedF[3][3] = 1;

    return estimatedF;
}

bool estimate_F_eight_point(const stereo_frame & reference, const stereo_frame & target, m4 & F)
{
    vector<v4> reference_pts;
    vector<v4> target_pts;

    // This assumes reference->features and target->features are sorted by id
    for(list<stereo_feature>::const_iterator s1iter = reference.features.begin(); s1iter != reference.features.end(); ++s1iter) {
        stereo_feature f1 = *s1iter;
        for(list<stereo_feature>::const_iterator s2iter = target.features.begin(); s2iter != target.features.end(); ++s2iter) {
            stereo_feature f2 = *s2iter;
            if(f1.id == f2.id) {
                reference_pts.push_back(f1.current);
                target_pts.push_back(f2.current);
            }
        }
    }

    if(reference_pts.size() < 8) {
        return false;
    }

    F = eight_point_F(&reference_pts[0], &target_pts[0], (int)reference_pts.size());

    return true;
}

// Triangulates a point in the world reference frame from two views
bool stereo::triangulate_internal(const stereo_frame & reference, const stereo_frame & target, int reference_x, int reference_y, int target_x, int target_y, v4 & intersection, float & error) const
{
    v4 o1_transformed, o2_transformed;
    v4 pa, pb;
    bool success;

    v4 p1_projected = project_point(reference_x, reference_y, center_x, center_y, focal_length);
    v4 p1_calibrated = calibrate_im_point(p1_projected, k1, k2, k3);
    v4 p2_projected = project_point(target_x, target_y, center_x, center_y, focal_length);
    v4 p2_calibrated = calibrate_im_point(p2_projected, k1, k2, k3);

    m4 R1w = to_rotation_matrix(reference.W);
    m4 R2w = to_rotation_matrix(target.W);

    v4 p1_cal_transformed = R1w*p1_calibrated + reference.T;
    v4 p2_cal_transformed = R2w*p2_calibrated + target.T;
    o1_transformed = reference.T;
    o2_transformed = target.T;

    // pa is the point on the first line closest to the intersection
    // pb is the point on the second line closest to the intersection
    success = line_line_intersect(o1_transformed, p1_cal_transformed, o2_transformed, p2_cal_transformed, pa, pb);
    if(!success) {
        if(debug_triangulate)
            fprintf(stderr, "Failed intersect\n");
        return false;
    }

    error = norm(pa - pb);
    v4 cam1_intersect = transpose(R1w) * (pa - reference.T);
    v4 cam2_intersect = transpose(R2w) * (pb - target.T);
    if(debug_triangulate)
        fprintf(stderr, "Lines were %.2fcm from intersecting at a depth of %.2fcm\n", error*100, cam1_intersect[2]*100);

    if(cam1_intersect[2] < 0 || cam2_intersect[2] < 0) {
        if(debug_triangulate)
            fprintf(stderr, "Lines intersected at a negative camera depth, failing\n");
        return false;
    }

    if(error/cam1_intersect[2] > .1) {
        if(debug_triangulate)
            fprintf(stderr, "Error too large, failing\n");
        return false;
    }
    intersection = pa + (pb - pa)/2;

    return true;
}

bool stereo::preprocess_internal(const stereo_frame &from, const stereo_frame &to, m4 &F, bool use_eight_point)
{
    bool success = true;
    used_eight_point = use_eight_point;
    // estimate_F uses R,T, and the camera calibration
    if(!use_eight_point)
        F = estimate_F(*this, from, to);
    else
    // estimate_F_eight_point uses common tracked features between the two frames
        success = estimate_F_eight_point(from, to, F);

    if(debug_info)
        write_debug_info();

    if(debug_frames)
        write_frames();

    return success;
}

bool stereo::triangulate_mesh(int reference_x1, int reference_y1, v4 & intersection) const
{
    if(!reference || !target)
        return false;

    bool result = stereo_mesh_triangulate(mesh, *this, reference_x1, reference_y1, intersection);
    return result;

}

bool stereo::triangulate(int reference_x1, int reference_y1, v4 & intersection, float * correspondence_score, int * x2, int * y2) const
{
    if(!reference || !target)
        return false;
    
    int target_x1, target_y1;
    float score;
    float error;
    
    // sets target_x1, target_y1
    bool ok = find_correspondence(*reference, *target, F, reference_x1, reference_y1, target_x1, target_y1, width, height, score);
    if(ok)
        ok = triangulate_internal(*reference, *target, reference_x1, reference_y1, target_x1, target_y1, intersection, error);

    if(x2) *x2 = target_x1;
    if(y2) *y2 = target_y1;

    if(correspondence_score) *correspondence_score = score;
    
    return ok;
}

bool compare_id(const stereo_feature & f1, const stereo_feature & f2)
{
    return f1.id < f2.id;
}

int intersection_length(list<stereo_feature> &l1, list<stereo_feature> &l2)
{
    l1.sort(compare_id);
    l2.sort(compare_id);

    vector<stereo_feature> intersection;
    intersection.resize(max(l1.size(),l2.size()));
    vector<stereo_feature>::iterator it;

    it = std::set_intersection(l1.begin(), l1.end(), l2.begin(), l2.end(), intersection.begin(), compare_id);
    int len = (int)(it - intersection.begin());

    return len;
}

v4 stereo::baseline()
{
    if(!target)
        return v4(0,0,0,0);
    
    m4 R1w = to_rotation_matrix(target->W);
    
    return transpose(R1w) * (T - target->T);
}

void stereo::process_frame(const struct stereo_global &g, const uint8_t *data, list<stereo_feature> &features, bool final)
{
    stereo_global::operator=(g);
    
    if(final) {
        if(reference) delete reference;
        reference = new stereo_frame(frame_number++, data, g.width, g.height, g.T, g.W, features);
    } else {
        if(features.size() >= 15) {
            if(!target || intersection_length(target->features, features) < 15) {
                if(target) delete target;
                target = new stereo_frame(frame_number++, data, g.width, g.height, g.T, g.W, features);
            }
        }
    }
}

bool stereo::preprocess(bool use_eight_point)
{
    if(!target || !reference) return false;
    return preprocess_internal(*reference, *target, F, use_eight_point);
}

bool stereo::preprocess_mesh(void(*progress_callback)(float))
{
    if(!target || !reference) return false;
    
    char filename[1024];
    char suffix[1024] = "";

    if(used_eight_point)
        sprintf(suffix, "-eight-point");

    // creates a mesh by searching for correspondences from reference to target
    mesh = stereo_mesh_create(*this, progress_callback);
    
    snprintf(filename, 1024, "%s%s.ply", debug_basename, suffix);
    stereo_mesh_write(filename, mesh, debug_texturename);
    snprintf(filename, 1024, "%s%s.json", debug_basename, suffix);
    stereo_mesh_write_json(filename, mesh, debug_texturename);
    snprintf(filename, 1024, "%s%s-correspondences.csv", debug_basename, suffix);
    stereo_mesh_write_correspondences(filename, mesh);

    stereo_remesh_delaunay(mesh);
    
    snprintf(filename, 1024, "%s%s-remesh.ply", debug_basename, suffix);
    stereo_mesh_write(filename, mesh, debug_texturename);
    snprintf(filename, 1024, "%s%s-remesh.json", debug_basename, suffix);
    stereo_mesh_write_json(filename, mesh, debug_texturename);
    snprintf(filename, 1024, "%s%s-remesh-correspondences.csv", debug_basename, suffix);
    stereo_mesh_write_correspondences(filename, mesh);

    return true;
}

void stereo::write_frames()
{
    char buffer[1024];
    snprintf(buffer, 1024, "%s-target.pgm", debug_basename);
    write_image(buffer, target->image, width, height);
    snprintf(buffer, 1024, "%s-reference.pgm", debug_basename);
    write_image(buffer, reference->image, width, height);
}

void m4_file_print(FILE * fp, const char * name, m4 M)
{
    fprintf(fp, "%s = [", name);
    for(int r=0; r<4; r++) {
        for(int c=0; c<4; c++) {
            fprintf(fp, " %f ", M[r][c]);
        }
        if(r == 3)
            fprintf(fp, "];\n");
        else
            fprintf(fp, ";\n");
    }

}

void v4_file_print(FILE * fp, const char * name, v4 V)
{
    fprintf(fp, "%s = [%f; %f; %f; %f];\n", name, V[0], V[1], V[2], V[3]);
}

void stereo::write_debug_info()
{
    char filename[1024];
    if(used_eight_point)
        snprintf(filename, 1024, "%s-eight-point-debug-info.m", debug_basename);
    else
        snprintf(filename, 1024, "%s-debug-info.m", debug_basename);

    FILE * debug_info = fopen(filename, "w");


    fprintf(debug_info, "width = %d;\n", width);
    fprintf(debug_info, "height = %d;\n", height);
    v4_file_print(debug_info, "Tglobal", T);
    m4 R = to_rotation_matrix(W);
    m4_file_print(debug_info, "Rglobal", R);

    m4 Rtarget = to_rotation_matrix(target->W);
    m4 Rreference = to_rotation_matrix(reference->W);
    m4_file_print(debug_info, "Rtarget", Rtarget);
    m4_file_print(debug_info, "Rreference", Rreference);
    v4_file_print(debug_info, "Ttarget", target->T);
    v4_file_print(debug_info, "Treference", reference->T);
    m4 dR = transpose(Rreference)*Rtarget;
    v4 dT = transpose(Rreference) * (target->T - reference->T);
    m4_file_print(debug_info, "dR", dR);
    v4_file_print(debug_info, "dT", dT);
    fprintf(debug_info, "enable_jitter = %d;\n", enable_jitter);

    fprintf(debug_info, "focal_length = %f;\n", focal_length);
    fprintf(debug_info, "center_x = %f;\n", center_x);
    fprintf(debug_info, "center_y = %f;\n", center_y);
    fprintf(debug_info, "k1 = %f;\n", k1);
    fprintf(debug_info, "k2 = %f;\n", k2);
    fprintf(debug_info, "k3 = %f;\n", k3);
    m4_file_print(debug_info, "F", F);

    fclose(debug_info);
}

stereo_frame::stereo_frame(const int _frame_number, const uint8_t *_image, int width, int height, const v4 &_T, const rotation_vector &_W, const list<stereo_feature > &_features): frame_number(_frame_number), T(_T), W(_W), features(_features)
{
    image = new uint8_t[width * height];
    memcpy(image, _image, width*height);
    // Sort features by id so when we do eight point later, they are already sorted
    features.sort(compare_id);
}

stereo_frame::~stereo_frame() { delete [] image; }
