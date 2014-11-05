//
//  ARDelegate.m
//  RC3DKSampleAR
//
//  Created by Eagle Jones on 10/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "ARDelegate.h"
#import <QuartzCore/CAEAGLLayer.h>
#import "RCGLManagerFactory.h"
#import "RCGLShaderProgram.h"
#include "RCDebugLog.h"

@implementation ARDelegate
{
    RCGLShaderProgram *program;
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

- (void)renderWithSensorFusionData:(RCSensorFusionData *)data withPerspectiveMatrix:(GLKMatrix4)projection
{
    if(!data.cameraParameters || !data.cameraTransformation) return;
    glUseProgram(program.program);
    
    GLKMatrix4 camera;
    [[data.cameraTransformation getInverse] getOpenGLMatrix:camera.m];
    
    GLKMatrix4 model;
    [initialCamera getOpenGLMatrix:model.m];
    

    glUniformMatrix4fv([program getUniformLocation:@"projection_matrix"], 1, false, projection.m);
    glUniformMatrix4fv([program getUniformLocation:@"camera_matrix"], 1, false, camera.m);

    glEnableVertexAttribArray([program getAttribLocation:@"position"]);
    
    glUniform3f([program getUniformLocation:@"light_direction"], 0, 0, 1);
    glUniform4f([program getUniformLocation:@"light_ambient"], .8, .8, .8, 1);
    glUniform4f([program getUniformLocation:@"light_diffuse"], .8, .8, .8, 1);
    glUniform4f([program getUniformLocation:@"light_specular"], .8, .8, .8, 1);
    
    glUniform4f([program getUniformLocation:@"material_ambient"], 0., 0., 1., 1);
    glUniform4f([program getUniformLocation:@"material_diffuse"], 0., 0.5, 1., 1);
    glUniform4f([program getUniformLocation:@"material_specular"], 1., 1., 1., 1);
    glUniform1f([program getUniformLocation:@"material_shininess"], 200.);

    model = GLKMatrix4Translate(model, 0, 0, 1);
    model = GLKMatrix4Scale(model, .1, .1, .1);
    glUniformMatrix4fv([program getUniformLocation:@"model_matrix"], 1, false, model.m);
    
    {
        GLfloat face[] = {
            -1,-1,1, -1,1,1, 1,-1,1,
            -1,1,1, 1,-1,1, 1,1,1
        };
        GLfloat normal[] = {
            0,0,1, 0,0,1, 0,0,1,
            0,0,1, 0,0,1, 0,0,1
        };
        
        glVertexAttribPointer([program getAttribLocation:@"position"], 3, GL_FLOAT, 0, 0, face);
        glEnableVertexAttribArray([program getAttribLocation:@"position"]);
        
        glVertexAttribPointer([program getAttribLocation:@"normal"], 3, GL_FLOAT, 0, 0, normal);
        glEnableVertexAttribArray([program getAttribLocation:@"normal"]);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    {
        GLfloat face[] = {
            -1,-1,-1, -1,1,-1, 1,-1,-1,
            -1,1,-1, 1,-1,-1, 1,1,-1
        };
        GLfloat normal[] = {
            0,0,-1, 0,0,-1, 0,0,-1,
            0,0,-1, 0,0,-1, 0,0,-1
        };
        
        glVertexAttribPointer([program getAttribLocation:@"position"], 3, GL_FLOAT, 0, 0, face);
        glEnableVertexAttribArray([program getAttribLocation:@"position"]);
        
        glVertexAttribPointer([program getAttribLocation:@"normal"], 3, GL_FLOAT, 0, 0, normal);
        glEnableVertexAttribArray([program getAttribLocation:@"normal"]);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    {
        GLfloat face[] = {
            1,-1,-1, 1,1,-1, 1,-1,1,
            1,1,-1, 1,-1,1, 1,1,1
        };
        GLfloat normal[] = {
            1,0,0, 1,0,0, 1,0,0,
            1,0,0, 1,0,0, 1,0,0
        };
        
        glVertexAttribPointer([program getAttribLocation:@"position"], 3, GL_FLOAT, 0, 0, face);
        glEnableVertexAttribArray([program getAttribLocation:@"position"]);
        
        glVertexAttribPointer([program getAttribLocation:@"normal"], 3, GL_FLOAT, 0, 0, normal);
        glEnableVertexAttribArray([program getAttribLocation:@"normal"]);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    {
        GLfloat face[] = {
            -1,-1,-1, -1,1,-1, -1,-1,1,
            -1,1,-1, -1,-1,1, -1,1,1
        };
        GLfloat normal[] = {
            -1,0,0, -1,0,0, -1,0,0,
            -1,0,0, -1,0,0, -1,0,0
        };
        
        glVertexAttribPointer([program getAttribLocation:@"position"], 3, GL_FLOAT, 0, 0, face);
        glEnableVertexAttribArray([program getAttribLocation:@"position"]);
        
        glVertexAttribPointer([program getAttribLocation:@"normal"], 3, GL_FLOAT, 0, 0, normal);
        glEnableVertexAttribArray([program getAttribLocation:@"normal"]);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    {
        GLfloat face[] = {
            -1,1,-1, 1,1,-1, -1,1,1,
            1,1,-1, -1,1,1, 1,1,1
        };
        GLfloat normal[] = {
            0,1,0, 0,1,0, 0,1,0,
            0,1,0, 0,1,0, 0,1,0
        };
        
        glVertexAttribPointer([program getAttribLocation:@"position"], 3, GL_FLOAT, 0, 0, face);
        glEnableVertexAttribArray([program getAttribLocation:@"position"]);
        
        glVertexAttribPointer([program getAttribLocation:@"normal"], 3, GL_FLOAT, 0, 0, normal);
        glEnableVertexAttribArray([program getAttribLocation:@"normal"]);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    {
        GLfloat face[] = {
            -1,-1,-1, 1,-1,-1, -1,-1,1,
            1,-1,-1, -1,-1,1, 1,-1,1
        };
        GLfloat normal[] = {
            0,-1,0, 0,-1,0, 0,-1,0,
            0,-1,0, 0,-1,0, 0,-1,0
        };
        
        glVertexAttribPointer([program getAttribLocation:@"position"], 3, GL_FLOAT, 0, 0, face);
        glEnableVertexAttribArray([program getAttribLocation:@"position"]);
        
        glVertexAttribPointer([program getAttribLocation:@"normal"], 3, GL_FLOAT, 0, 0, normal);
        glEnableVertexAttribArray([program getAttribLocation:@"normal"]);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }


/*    static const GLfloat cubeVerticesStrip[] = {
        
        // Front face
        -1,-1,1, 1,-1,1, -1,1,1, 1,1,1,
        // Right face
        1,1,1, 1,-1,1, 1,1,-1, 1,-1,-1,
        // Back face
        1,-1,-1, -1,-1,-1, 1,1,-1, -1,1,-1,
        // Left face
        -1,1,-1, -1,-1,-1, -1,1,1, -1,-1,1,
        // Bottom face
        -1,-1,1, -1,-1,-1, 1,-1,1, 1,-1,-1,
        
        // move to top
        1,-1,-1, -1,1,1,
        
        // Top Face
        -1,1,1, 1,1,1, -1,1,-1, 1,1,-1
    };*/
    //glUniform4f(uniformLocation[UNIFORM_COLOR], 0, 1, 0, 1);


}


@end

