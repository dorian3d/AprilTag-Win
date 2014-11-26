//
//  MPARDelegate.m
//  MeasuredPhoto
//
//  Created by Eagle Jones on 10/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPARDelegate.h"
#import <QuartzCore/CAEAGLLayer.h>
#import "RCCore/RCGLManagerFactory.h"
#import "RCCore/RCGLShaderProgram.h"

@implementation MPARDelegate
{
    RCGLShaderProgram *program;
    float progressHorizontal;
    float progressVertical;
}

@synthesize initialCamera;

-(id)init
{
    if (self = [super init])
    {
        program = [[RCGLShaderProgram alloc] init];
        [program buildWithVertexFileName:@"shader.vsh" withFragmentFileName:@"shader.fsh"];
    }
    return self;
}

const static int vertex_size = 2;

//primary arrow and progress indicator to the right

const static GLfloat progress_vertices[9 * 2] = {
    .125, -.125,
    .125, .125,
    .625, -.125,

    .125, .125,
    .625, .125,
    .625, -.125,

    .625, -.375,
    .625, .375,
    1., 0.,
};

const static GLfloat arrow_vertices[7 * 2] = {
    .125, .125,
    .625, .125,
    .625, .375,
    1., 0.,
    .625, -.375,
    .625, -.125,
    .125, -.125,
};


const static GLfloat other_vertices[21 * vertex_size] = {
    //bottom
    .125, -.125,
    .125, -.625,
    .375, -.625,
    0., -1.,
    -.375, -.625,
    -.125, -.625,
    -.125, -.125,
    
    //left
    -.125, -.125,
    -.625, -.125,
    -.625, -.375,
    -1., 0.,
    -.625, .375,
    -.625, .125,
    -.125, .125,
    
    //top
    -.125, .125,
    -.125, .625,
    -.375, .625,
    0., 1.,
    .375, .625,
    .125, .625,
    .125, .125,
};

const static GLfloat cube_vertices[6*6 * 3] = {
    //top
    1,1,1, -1,1,1, -1,-1,1,
    -1,-1,1, 1,-1,1, 1,1,1,
    //bottom
    1,1,-1, 1,-1,-1, -1,-1,-1,
    -1,-1,-1, -1,1,-1, 1,1,-1,
    
    //left
    -1,1,1, -1,1,-1, -1,-1,-1,
    -1,-1,-1, -1,-1,1, -1,1,1,
    //right
    1,1,1, 1,-1,1, 1,-1,-1,
    1,-1,-1, 1,1,-1, 1,1,1,
    
    //back
    1,-1,1, -1,-1,1, -1,-1,-1,
    -1,-1,-1, 1,-1,-1, 1,-1,1,
    //front
    1,1,1, 1,1,-1, -1,1,-1,
    -1,1,-1, -1,1,1, 1,1,1
};

const static GLfloat cube_normals[6*6 * 3] = {
    //top
    0,0,1, 0,0,1, 0,0,1,
    0,0,1, 0,0,1, 0,0,1,
    //bottom
    0,0,-1, 0,0,-1, 0,0,-1,
    0,0,-1, 0,0,-1, 0,0,-1,
    
    //left
    -1,0,0, -1,0,0, -1,0,0,
    -1,0,0, -1,0,0, -1,0,0,
    //right
    1,0,0, 1,0,0, 1,0,0,
    1,0,0, 1,0,0, 1,0,0,
    
    //back
    0,-1,0, 0,-1,0, 0,-1,0,
    0,-1,0, 0,-1,0, 0,-1,0,
    //front
    0,1,0, 0,1,0, 0,1,0,
    0,1,0, 0,1,0, 0,1,0,
};


- (void) setProgressHorizontal:(float)horizontal withVertical:(float)vertical
{
    progressHorizontal = horizontal;
    progressVertical = vertical;
}

const static float arrowDepth = 2.;
const static float arrowScale = .5;

- (void)renderWithSensorFusionData:(RCSensorFusionData *)data withCameraToScreenMatrix:(GLKMatrix4)cameraToScreen
{
    if(!data.cameraParameters || !data.cameraTransformation) return;
    glUseProgram(program.program);

    glUniformMatrix4fv([program getUniformLocation:@"projection_matrix"], 1, false, cameraToScreen.m);

    GLKMatrix4 camera;
    [[data.cameraTransformation getInverse] getOpenGLMatrix:camera.m];
    glUniformMatrix4fv([program getUniformLocation:@"camera_matrix"], 1, false, camera.m);
    
    glUniform3f([program getUniformLocation:@"light_direction"], 0, 0, 1);
    glUniform4f([program getUniformLocation:@"light_ambient"], .8, .8, .8, 1);
    glUniform4f([program getUniformLocation:@"light_diffuse"], .8, .8, .8, 1);
    glUniform4f([program getUniformLocation:@"light_specular"], .8, .8, .8, 1);
    
    glUniform4f([program getUniformLocation:@"material_ambient"], 0.05, 1., 0.1, 1);
    glUniform4f([program getUniformLocation:@"material_diffuse"], 0., 1., 0.5, 1);
    glUniform4f([program getUniformLocation:@"material_specular"], 1., 1., 1., 1);
    glUniform1f([program getUniformLocation:@"material_shininess"], 200.);
    
    float progress;
    float arrowTheta;
    if(fabs(progressVertical) > fabs(progressHorizontal))
    {
        if(progressVertical > 0.) arrowTheta = M_PI_2;
        else arrowTheta = -M_PI_2;
        progress = fabs(progressVertical);
    } else {
        if(progressHorizontal > 0.) arrowTheta = 0.;
        else arrowTheta = M_PI;
        progress = fabs(progressHorizontal);
    }
    
    //Concatenating GLKit matrices goes left to right, and our shaders multiply with matrices on the left and vectors on the right.
    //So the last transformation listed is applied to our vertices first
    GLKMatrix4 model;
    
    //Place it in front of the initial camera position
    [initialCamera getOpenGLMatrix:model.m];
    model = GLKMatrix4Translate(model, 0, 0, arrowDepth);
    model = GLKMatrix4Scale(model, arrowScale, arrowScale, arrowScale);
    model = GLKMatrix4RotateZ(model, arrowTheta);
    glUniformMatrix4fv([program getUniformLocation:@"model_matrix"], 1, false, model.m);

    /*********** outline ******/
    glEnableVertexAttribArray([program getAttribLocation:@"position"]);
    glVertexAttribPointer([program getAttribLocation:@"position"], vertex_size, GL_FLOAT, 0, 0, arrow_vertices);
    
    glLineWidth(2.);
    glDrawArrays(GL_LINE_STRIP, 0, 7);

    /******* other 3 outline *****/
    glUniform4f([program getUniformLocation:@"material_ambient"], 0.05, 1., 0.1, 1. - progress);
    glUniform4f([program getUniformLocation:@"material_diffuse"], 0., 1., 0.5, 1. - progress);
    glUniform4f([program getUniformLocation:@"material_specular"], 0., 0., 0., 0.);

    glVertexAttribPointer([program getAttribLocation:@"position"], vertex_size, GL_FLOAT, 0, 0, other_vertices);
    
    glLineWidth(2.);
    glDrawArrays(GL_LINE_STRIP, 0, 21);

    
//#define ARROW_PROGRESS
    /************filled arrow*****/
    
    glUniform4f([program getUniformLocation:@"material_ambient"], 0.05, 1., 0.1, .5);
    glUniform4f([program getUniformLocation:@"material_diffuse"], 0., 1., 0.5, .5);
    glUniform4f([program getUniformLocation:@"material_specular"], 0., 0., 0., 0.);
#ifdef ARROW_PROGRESS
    model = GLKMatrix4Scale(model, progress, progress, progress);

    glUniformMatrix4fv([program getUniformLocation:@"model_matrix"], 1, false, model.m);
    glVertexAttribPointer([program getAttribLocation:@"position"], 2, GL_FLOAT, 0, 0, progress_vertices);
    glDrawArrays(GL_TRIANGLES, 0, 9);
#else
    GLfloat bar_vertices[] = {
        0., -.125,
        0., .125,
        progress, .125,
        progress, -.125
    };
    
    glVertexAttribPointer([program getAttribLocation:@"position"], 2, GL_FLOAT, 0, 0, bar_vertices);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
#endif
    
    /********** cube ***/
    
    glUniform4f([program getUniformLocation:@"material_ambient"], 0.05, 1., 0.1, 1);
    glUniform4f([program getUniformLocation:@"material_diffuse"], 0., 1., 0.5, 1);
    glUniform4f([program getUniformLocation:@"material_specular"], 1., 1., 1., 1);

    [initialCamera getOpenGLMatrix:model.m];
    model = GLKMatrix4Translate(model, progressHorizontal * arrowScale, progressVertical * arrowScale, arrowDepth);

    model = GLKMatrix4Scale(model, arrowScale / 8., arrowScale / 8., arrowScale / 8.);
    
    glUniformMatrix4fv([program getUniformLocation:@"model_matrix"], 1, false, model.m);
    glEnableVertexAttribArray([program getAttribLocation:@"normal"]);
    
    glVertexAttribPointer([program getAttribLocation:@"position"], 3, GL_FLOAT, 0, 0, cube_vertices);
    glVertexAttribPointer([program getAttribLocation:@"normal"], 3, GL_FLOAT, 0, 0, cube_normals);

    glDrawArrays(GL_TRIANGLES, 0, 6*6);
    
    
    glDisableVertexAttribArray([program getAttribLocation:@"position"]);
    glDisableVertexAttribArray([program getAttribLocation:@"normal"]);

}


@end

