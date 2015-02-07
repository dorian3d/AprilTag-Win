//
//  TreasureViewController.m
//  CardboardSDK-iOS
//
//

#import "TreasureViewController.h"

#include "CardboardSDK.h"

#import <AudioToolbox/AudioServices.h>
#import <OpenGLES/ES2/glext.h>
#import "RCGLShaderProgram.h"
#include "chair_chesterfield.h"


@interface TreasureRenderer : NSObject <StereoRendererDelegate>
{
    GLuint _cubeVertexArray;
    GLuint _cubeVertexBuffer;
    GLuint _cubeColorBuffer;
    GLuint _cubeFoundColorBuffer;
    GLuint _cubeNormalBuffer;

    GLuint _floorVertexArray;
    GLuint _floorVertexBuffer;
    GLuint _floorColorBuffer;
    GLuint _floorNormalBuffer;

    GLuint _ceilingColorBuffer;

    GLuint _cubeProgram;
    GLuint _highlightedCubeProgram;
    GLuint _floorProgram;
    
    GLint _cubePositionLocation;
    GLint _cubeNormalLocation;
    GLint _cubeColorLocation;
    
    GLint _cubeModelLocation;
    GLint _cubeModelViewLocation;
    GLint _cubeModelViewProjectionLocation;
    GLint _cubeLightPositionLocation;

    GLint _floorPositionLocation;
    GLint _floorNormalLocation;
    GLint _floorColorLocation;
    
    GLint _floorModelLocation;
    GLint _floorModelViewLocation;
    GLint _floorModelViewProjectionLocation;
    GLint _floorLightPositionLocation;

    GLKMatrix4 _perspective;
    GLKMatrix4 _modelCube;
    GLKMatrix4 _camera;
    GLKMatrix4 _view;
    GLKMatrix4 _modelViewProjection;
    GLKMatrix4 _modelView;
    GLKMatrix4 _modelFloor;
    GLKMatrix4 _modelCeiling;
    GLKMatrix4 _headView;
    
    float _zNear;
    float _zFar;
    
    float _cameraZ;
    float _timeDelta;
    
    float _yawLimit;
    float _pitchLimit;
    
    int _coordsPerVertex;
    
    GLKVector4 _lightPositionInWorldSpace;
    GLKVector4 _lightPositionInEyeSpace;
    
    int _score;
    float _objectDistance;
    float _floorDepth;
    
    
    RCGLShaderProgram *program;
    GLKTextureInfo *texture;

    BOOL isSensorFusionRunning;
}

@property (weak, nonatomic) CardboardViewController *cardboard;

@end


@implementation TreasureRenderer

- (instancetype)init
{
    self = [super init];
    if (!self) { return nil; }
    
    _objectDistance = 1.0f;
    _floorDepth = 1.5f;
    
    _zNear = 0.1f;
    _zFar = 100.0f;
    
    _cameraZ = 0.01f;
    
    _timeDelta = 1.0f;
    
    _yawLimit = 0.12f;
    _pitchLimit = 0.12f;
    
    _coordsPerVertex = 3;
    
    // We keep the light always position just above the user.
    _lightPositionInWorldSpace = GLKVector4Make(0.0f, 2.0f, 0.0f, 1.0f);
    _lightPositionInEyeSpace = GLKVector4Make(0.0f, 0.0f, 0.0f, 0.0f);
    
    isSensorFusionRunning = NO;

    return self;
}

- (void)setupRendererWithView:(GLKView *)GLView
{
    [EAGLContext setCurrentContext:GLView.context];

    [self setupPrograms];

    [self setupVAOS];
    
    // Etc
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.2f, 0.2f, 0.5f); // Dark background so text shows up well.

    
    // Object first appears directly in front of user.
    _modelCube = GLKMatrix4Identity;
    _modelCube = GLKMatrix4Translate(_modelCube, 0, 0, -_objectDistance);
    _modelCube = GLKMatrix4Scale(_modelCube, 2., 2., 2.);
    
    _modelFloor = GLKMatrix4Identity;
    _modelFloor = GLKMatrix4Translate(_modelFloor, 0, -_floorDepth, 0); // Floor appears below user.

    _modelCeiling = GLKMatrix4Identity;
    _modelCeiling = GLKMatrix4Translate(_modelFloor, 0, 1.5 * _floorDepth, 0); // Ceiling appears above user.

    GLCheckForError();
    
    UITapGestureRecognizer *tapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTap:)];
    tapRecognizer.numberOfTapsRequired = 1;
    [GLView addGestureRecognizer:tapRecognizer];
}

- (BOOL)setupPrograms
{
    NSString *path = nil;
    
    GLuint vertexShader = 0;
    path = [[NSBundle mainBundle] pathForResource:@"light_vertex" ofType:@"shader"];
    if (!GLCompileShaderFromFile(&vertexShader, GL_VERTEX_SHADER, path)) {
        NSLog(@"Failed to compile shader at %@", path);
        return NO;
    }
    
    GLuint gridVertexShader = 0;
    path = [[NSBundle mainBundle] pathForResource:@"light_vertex_grid" ofType:@"shader"];
    if (!GLCompileShaderFromFile(&gridVertexShader, GL_VERTEX_SHADER, path)) {
        NSLog(@"Failed to compile shader at %@", path);
        return NO;
    }
    
    GLuint gridFragmentShader = 0;
    path = [[NSBundle mainBundle] pathForResource:@"grid_fragment" ofType:@"shader"];
    if (!GLCompileShaderFromFile(&gridFragmentShader, GL_FRAGMENT_SHADER, path)) {
        NSLog(@"Failed to compile shader at %@", path);
        return NO;
    }
    
    GLuint passthroughFragmentShader = 0;
    path = [[NSBundle mainBundle] pathForResource:@"passthrough_fragment" ofType:@"shader"];
    if (!GLCompileShaderFromFile(&passthroughFragmentShader, GL_FRAGMENT_SHADER, path)) {
        NSLog(@"Failed to compile shader at %@", path);
        return NO;
    }
    
    GLuint highlightFragmentShader = 0;
    path = [[NSBundle mainBundle] pathForResource:@"highlight_fragment" ofType:@"shader"];
    if (!GLCompileShaderFromFile(&highlightFragmentShader, GL_FRAGMENT_SHADER, path)) {
        NSLog(@"Failed to compile shader at %@", path);
        return NO;
    }
    
    _cubeProgram = glCreateProgram();
    glAttachShader(_cubeProgram, vertexShader);
    glAttachShader(_cubeProgram, passthroughFragmentShader);
    GLLinkProgram(_cubeProgram);
    glUseProgram(_cubeProgram);
    
    GLCheckForError();
    
    _highlightedCubeProgram = glCreateProgram();
    glAttachShader(_highlightedCubeProgram, vertexShader);
    glAttachShader(_highlightedCubeProgram, highlightFragmentShader);
    GLLinkProgram(_highlightedCubeProgram);
    glUseProgram(_highlightedCubeProgram);
    
    GLCheckForError();
    
    _floorProgram = glCreateProgram();
    glAttachShader(_floorProgram, gridVertexShader);
    glAttachShader(_floorProgram, gridFragmentShader);
    GLLinkProgram(_floorProgram);
    glUseProgram(_floorProgram);
    
    GLCheckForError();
    
    glUseProgram(0);
    
    program = [[RCGLShaderProgram alloc] init];
    [program buildWithVertexFileName:@"shader.vsh" withFragmentFileName:@"shader.fsh"];
    
    texture = [GLKTextureLoader textureWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"chair_chesterfield_d.png" ofType: nil] options:NULL error:NULL];
    
    return YES;
}

- (void)setupVAOS
{
    const GLfloat cubeVertices[] =
    {
        // Front face
        -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        
        // Right face
        1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        
        // Back face
        1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        
        // Left face
        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        
        // Top face
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        
        // Bottom face
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
    };
    
    const GLfloat cubeColors[] =
    {
        // front, green
        0.0f, 0.5273f, 0.2656f, 1.0f,
        0.0f, 0.5273f, 0.2656f, 1.0f,
        0.0f, 0.5273f, 0.2656f, 1.0f,
        0.0f, 0.5273f, 0.2656f, 1.0f,
        0.0f, 0.5273f, 0.2656f, 1.0f,
        0.0f, 0.5273f, 0.2656f, 1.0f,
        
        // right, blue
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
        
        // back, also green
        0.0f, 0.5273f, 0.2656f, 1.0f,
        0.0f, 0.5273f, 0.2656f, 1.0f,
        0.0f, 0.5273f, 0.2656f, 1.0f,
        0.0f, 0.5273f, 0.2656f, 1.0f,
        0.0f, 0.5273f, 0.2656f, 1.0f,
        0.0f, 0.5273f, 0.2656f, 1.0f,
        
        // left, also blue
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
        
        // top, red
        0.8359375f,  0.17578125f,  0.125f, 1.0f,
        0.8359375f,  0.17578125f,  0.125f, 1.0f,
        0.8359375f,  0.17578125f,  0.125f, 1.0f,
        0.8359375f,  0.17578125f,  0.125f, 1.0f,
        0.8359375f,  0.17578125f,  0.125f, 1.0f,
        0.8359375f,  0.17578125f,  0.125f, 1.0f,
        
        // bottom, also red
        0.8359375f,  0.17578125f,  0.125f, 1.0f,
        0.8359375f,  0.17578125f,  0.125f, 1.0f,
        0.8359375f,  0.17578125f,  0.125f, 1.0f,
        0.8359375f,  0.17578125f,  0.125f, 1.0f,
        0.8359375f,  0.17578125f,  0.125f, 1.0f,
        0.8359375f,  0.17578125f,  0.125f, 1.0f,
    };
        
    const GLfloat cubeNormals[] =
    {
        // Front face
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        
        // Right face
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        
        // Back face
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        
        // Left face
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        
        // Top face
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        
        // Bottom face
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f
    };
    
    const GLfloat floorVertices[] =
    {
        200.0f,  0.0f, -200.0f,
        -200.0f,  0.0f, -200.0f,
        -200.0f,  0.0f,  200.0f,
        200.0f,  0.0f, -200.0f,
        -200.0f,  0.0f,  200.0f,
        200.0f,  0.0f,  200.0f,
    };
    
    const GLfloat floorNormals[] =
    {
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
    };
    
    const GLfloat floorColors[] =
    {
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
    };
    
    const GLfloat ceilingColors[] =
    {
        0.0f, 0.6f, 0.0f, 1.0f,
        0.0f, 0.6f, 0.0f, 1.0f,
        0.0f, 0.6f, 0.0f, 1.0f,
        0.0f, 0.6f, 0.0f, 1.0f,
        0.0f, 0.6f, 0.0f, 1.0f,
        0.0f, 0.6f, 0.0f, 1.0f,
    };

    // Cube VAO setup
    glGenVertexArraysOES(1, &_cubeVertexArray);
    glBindVertexArrayOES(_cubeVertexArray);
    
    _cubePositionLocation = glGetAttribLocation(_cubeProgram, "a_Position");
    _cubeNormalLocation = glGetAttribLocation(_cubeProgram, "a_Normal");
    _cubeColorLocation = glGetAttribLocation(_cubeProgram, "a_Color");
    
    _cubeModelLocation = glGetUniformLocation(_cubeProgram, "u_Model");
    _cubeModelViewLocation = glGetUniformLocation(_cubeProgram, "u_MVMatrix");
    _cubeModelViewProjectionLocation = glGetUniformLocation(_cubeProgram, "u_MVP");
    _cubeLightPositionLocation = glGetUniformLocation(_cubeProgram, "u_LightPos");
    
    glEnableVertexAttribArray(_cubePositionLocation);
    glEnableVertexAttribArray(_cubeNormalLocation);
    glEnableVertexAttribArray(_cubeColorLocation);
    
    glGenBuffers(1, &_cubeVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _cubeVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(chair_chesterfieldVerts), chair_chesterfieldVerts, GL_STATIC_DRAW);
    
    // Set the position of the cube
    glVertexAttribPointer(_cubePositionLocation, _coordsPerVertex, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    glGenBuffers(1, &_cubeNormalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _cubeNormalBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(chair_chesterfieldNormals), chair_chesterfieldNormals, GL_STATIC_DRAW);
    
    // Set the normal positions of the cube, again for shading
    glVertexAttribPointer(_cubeNormalLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    glGenBuffers(1, &_cubeColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _cubeColorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(chair_chesterfieldTexCoords), chair_chesterfieldTexCoords, GL_STATIC_DRAW);
    
    glVertexAttribPointer(_cubeColorLocation, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    GLCheckForError();
    
    glBindVertexArrayOES(0);
    
    
    // Floor VAO setup
    glGenVertexArraysOES(1, &_floorVertexArray);
    glBindVertexArrayOES(_floorVertexArray);
    
    _floorModelLocation = glGetUniformLocation(_floorProgram, "u_Model");
    _floorModelViewLocation = glGetUniformLocation(_floorProgram, "u_MVMatrix");
    _floorModelViewProjectionLocation = glGetUniformLocation(_floorProgram, "u_MVP");
    _floorLightPositionLocation = glGetUniformLocation(_floorProgram, "u_LightPos");
    
    _floorPositionLocation = glGetAttribLocation(_floorProgram, "a_Position");
    _floorNormalLocation = glGetAttribLocation(_floorProgram, "a_Normal");
    _floorColorLocation = glGetAttribLocation(_floorProgram, "a_Color");
    
    glEnableVertexAttribArray(_floorPositionLocation);
    glEnableVertexAttribArray(_floorNormalLocation);
    glEnableVertexAttribArray(_floorColorLocation);
    
    glGenBuffers(1, &_floorVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _floorVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
    
    // Set the position of the floor
    glVertexAttribPointer(_floorPositionLocation, _coordsPerVertex, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    glGenBuffers(1, &_floorNormalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _floorNormalBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorNormals), floorNormals, GL_STATIC_DRAW);
    
    // Set the normal positions of the floor, again for shading
    glVertexAttribPointer(_floorNormalLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    glGenBuffers(1, &_floorColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _floorColorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorColors), floorColors, GL_STATIC_DRAW);
    
    glGenBuffers(1, &_ceilingColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _ceilingColorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ceilingColors), ceilingColors, GL_STATIC_DRAW);

    GLCheckForError();
    
    glBindVertexArrayOES(0);
}

- (void)shutdownRendererWithView:(GLKView *)GLView
{
}

- (void)renderViewDidChangeSize:(CGSize)size
{
}

- (void)prepareNewFrameWithHeadViewMatrix:(GLKMatrix4)headViewMatrix
{
    // Build the Model part of the ModelView matrix
    //_modelCube = GLKMatrix4Rotate(_modelCube, GLKMathDegreesToRadians(_timeDelta), 0.5f, 0.5f, 1.0f);
    
    // Build the camera matrix and apply it to the ModelView.
    _camera = GLKMatrix4MakeLookAt(0, 0, _cameraZ,
                                   0, 0, 0,
                                   0, 1.0f, 0);
    
    _headView = headViewMatrix;
    
    GLCheckForError();
}

- (void)drawEyeWithEye:(EyeWrapper *)eye
{
    // DLog(@"%ld %@", eye.type, NSStringFromGLKMatrix4([eye eyeViewMatrix]));

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    GLCheckForError();
    
    // Apply the eye transformation to the camera
    _view = GLKMatrix4Multiply([eye eyeViewMatrix], _camera);
    
    // Set the position of the light
    _lightPositionInEyeSpace = GLKMatrix4MultiplyVector4(_view, _lightPositionInWorldSpace);
    
    const float zNear = 0.1f;
    const float zFar = 100.0f;
    _perspective = [eye perspectiveMatrixWithZNear:zNear zFar:zFar];
    [self drawCube];
    
    [self drawFloorAndCeiling];
    
    //[self drawChair];
}

- (void)finishFrameWithViewportRect:(CGRect)viewPort
{
}

// Draw the cube.
// We've set all of our transformation matrices. Now we simply pass them into the shader.
- (void)drawCube
{
    // Build the ModelView and ModelViewProjection matrices
    // for calculating cube position and light.
    _modelView = GLKMatrix4Multiply(_view, _modelCube);
    _modelViewProjection = GLKMatrix4Multiply(_perspective, _modelView);

    if (isSensorFusionRunning)
    {
        glUseProgram(_highlightedCubeProgram);
    }
    else
    {
        glUseProgram(_cubeProgram);
    }
    
    glBindVertexArrayOES(_cubeVertexArray);

    glUniform3f(_cubeLightPositionLocation,
                _lightPositionInEyeSpace.x,
                _lightPositionInEyeSpace.y,
                _lightPositionInEyeSpace.z);
    
    // Set the Model in the shader, used to calculate lighting
    glUniformMatrix4fv(_cubeModelLocation, 1, GL_FALSE, _modelCube.m);
    
    // Set the ModelView in the shader, used to calculate lighting
    glUniformMatrix4fv(_cubeModelViewLocation, 1, GL_FALSE, _modelView.m);
    
    // Set the ModelViewProjection matrix in the shader.
    glUniformMatrix4fv(_cubeModelViewProjectionLocation, 1, GL_FALSE, _modelViewProjection.m);
    
    glDrawArrays(GL_TRIANGLES, 0, chair_chesterfieldNumVerts);
    
    GLCheckForError();
    
    glBindVertexArrayOES(0);
    glUseProgram(0);
}

// Draw the floor.
// This feeds in data for the floor into the shader. Note that this doesn't feed in data about
// position of the light, so if we rewrite our code to draw the floor first, the lighting might
// look strange.
- (void)drawFloorAndCeiling
{
    glUseProgram(_floorProgram);
    glBindVertexArrayOES(_floorVertexArray);

    
    // Floor
    // Set mModelView for the floor, so we draw floor in the correct location
    _modelView = GLKMatrix4Multiply(_view, _modelFloor);
    _modelViewProjection = GLKMatrix4Multiply(_perspective, _modelView);

    // Set ModelView, MVP, position, normals, and color.
    glUniform3f(_floorLightPositionLocation,
                _lightPositionInEyeSpace.x,
                _lightPositionInEyeSpace.y,
                _lightPositionInEyeSpace.z);
    glUniformMatrix4fv(_floorModelLocation, 1, GL_FALSE, _modelFloor.m);
    glUniformMatrix4fv(_floorModelViewLocation, 1, GL_FALSE, _modelView.m);
    glUniformMatrix4fv(_floorModelViewProjectionLocation, 1, GL_FALSE, _modelViewProjection.m);
    
    glBindBuffer(GL_ARRAY_BUFFER, _floorColorBuffer);
    glVertexAttribPointer(_floorColorLocation, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    GLCheckForError();

    
    // Ceiling
    _modelView = GLKMatrix4Multiply(_view, _modelCeiling);
    _modelViewProjection = GLKMatrix4Multiply(_perspective, _modelView);
    
    glUniformMatrix4fv(_floorModelViewProjectionLocation, 1, GL_FALSE, _modelViewProjection.m);

    glBindBuffer(GL_ARRAY_BUFFER, _ceilingColorBuffer);
    glVertexAttribPointer(_floorColorLocation, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    GLCheckForError();
    
    
    glBindVertexArrayOES(0);
    glUseProgram(0);
}

- (void)drawChair
{
    glUseProgram(program.program);

    glUniformMatrix4fv([program getUniformLocation:@"projection_matrix"], 1, false, _perspective.m);

    glUniformMatrix4fv([program getUniformLocation:@"camera_matrix"], 1, false, _view.m);

    
    glUniform3f([program getUniformLocation:@"light_direction"], 0, 0, 1);
    glUniform4f([program getUniformLocation:@"light_ambient"], .8, .8, .8, 1);
    glUniform4f([program getUniformLocation:@"light_diffuse"], .8, .8, .8, 1);
    glUniform4f([program getUniformLocation:@"light_specular"], .8, .8, .8, 1);
    
    //    glUniform4f([program getUniformLocation:@"material_ambient"], 1., 0., 0., 1);
    //    glUniform4f([program getUniformLocation:@"material_diffuse"], 0., 0., 0., 1);
    glUniform4f([program getUniformLocation:@"material_specular"], 1., 1., 1., 1);
    glUniform1f([program getUniformLocation:@"material_shininess"], 200.);
    
    GLKMatrix4 model = GLKMatrix4Identity;
    glUniformMatrix4fv([program getUniformLocation:@"model_matrix"], 1, false, model.m);
 
#ifdef SHOW_AXES
    glEnableVertexAttribArray([program getAttribLocation:@"position"]);
    glVertexAttribPointer([program getAttribLocation:@"position"], 3, GL_FLOAT, 0, 0, x_vertex);
    glDrawArrays(GL_LINES, 0, 2);
    
    
    glUniform4f([program getUniformLocation:@"material_ambient"], 0., 1., 0., 1);
    glVertexAttribPointer([program getAttribLocation:@"position"], 3, GL_FLOAT, 0, 0, y_vertex);
    glDrawArrays(GL_LINES, 0, 2);
    
    glUniform4f([program getUniformLocation:@"material_ambient"], 0., 0., 1., 1);
    glVertexAttribPointer([program getAttribLocation:@"position"], 3, GL_FLOAT, 0, 0, z_vertex);
    glDrawArrays(GL_LINES, 0, 2);
#endif
    
    //    glUniform4f([program getUniformLocation:@"material_ambient"], 0.25, 0.125, .05, 1);
    //    glUniform4f([program getUniformLocation:@"material_diffuse"], 0.25, 0.125, .05, 1);
    glUniform4f([program getUniformLocation:@"material_specular"], .1, .1, .1, 1);
    glUniform1f([program getUniformLocation:@"material_shininess"], 20.);
    
    //Concatenating GLKit matrices goes left to right, and our shaders multiply with matrices on the left and vectors on the right.
    //So the last transformation listed is applied to our vertices first
    
    model = GLKMatrix4Translate(model, 0., 1.5, -1.5);
    
    //Position it at the origin
    //model = GLKMatrix4Translate(model, 0., 0., 0.);
    //Scale our model so it's 10 cm on a side
    model = GLKMatrix4Scale(model, 1.1, 1.1, 1.1);
    model = GLKMatrix4Translate(model, 0., 0., .5);
    model = GLKMatrix4RotateZ(model, M_PI);
    model = GLKMatrix4RotateX(model, M_PI_2);
    
    glUniformMatrix4fv([program getUniformLocation:@"model_matrix"], 1, false, model.m);
    
    glEnableVertexAttribArray([program getAttribLocation:@"position"]);
    glEnableVertexAttribArray([program getAttribLocation:@"normal"]);
    glEnableVertexAttribArray([program getAttribLocation:@"texture_coordinate"]);
    
    glVertexAttribPointer([program getAttribLocation:@"position"], 3, GL_FLOAT, 0, 0, chair_chesterfieldVerts);
    glVertexAttribPointer([program getAttribLocation:@"normal"], 3, GL_FLOAT, 0, 0, chair_chesterfieldNormals);
    glVertexAttribPointer([program getAttribLocation:@"texture_coordinate"], 2, GL_FLOAT, 0, 0, chair_chesterfieldTexCoords);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(texture.target, texture.name);
    glUniform1i([program getUniformLocation:@"texture_value"], 0);
    
    //glDrawArrays(GL_TRIANGLES, 0, chair_chesterfieldNumVerts);
    
    glDisableVertexAttribArray([program getAttribLocation:@"position"]);
    glDisableVertexAttribArray([program getAttribLocation:@"normal"]);
    glDisableVertexAttribArray([program getAttribLocation:@"texture_coordinate"]);
    
}

// Check if user is looking at object by calculating where the object is in eye-space.
// @return true if the user is looking at the object.
- (BOOL)isLookingAtCube
{
    GLKVector4 initVector = { 0, 0, 0, 1.0f };

    // Convert object space to camera space. Use the headView from onNewFrame.
    _modelView = GLKMatrix4Multiply(_headView, _modelCube);
    GLKVector4 objectPositionVector = GLKMatrix4MultiplyVector4(_modelView, initVector);
    
    float pitch = atan2f(objectPositionVector.y, -objectPositionVector.z);
    float yaw = atan2f(objectPositionVector.x, -objectPositionVector.z);
    
    const float yawLimit = 0.12f;
    const float pitchLimit = 0.12f;

    return fabs(pitch) < pitchLimit && fabs(yaw) < yawLimit;
}

#define ARC4RANDOM_MAX 0x100000000
// Return a random float in the range [0.0, 1.0]
float randomFloat()
{
    return ((double)arc4random() / ARC4RANDOM_MAX);
}

// Find a new random position for the object.
// We'll rotate it around the Y-axis so it's out of sight, and then up or down by a little bit.
- (void)hideCube
{
    // First rotate in XZ plane, between 90 and 270 deg away, and scale so that we vary
    // the object's distance from the user.
    float angleXZ = randomFloat() * 180 + 90;
    GLKMatrix4 transformationMatrix = GLKMatrix4MakeRotation(GLKMathDegreesToRadians(angleXZ), 0.0f, 1.0f, 0.0f);
    float oldObjectDistance = _objectDistance;
    _objectDistance = randomFloat() * 15 + 5;
    float objectScalingFactor = _objectDistance / oldObjectDistance;
    transformationMatrix = GLKMatrix4Scale(transformationMatrix, objectScalingFactor, objectScalingFactor,
                  objectScalingFactor);
    GLKVector4 positionVector = GLKMatrix4MultiplyVector4(transformationMatrix,
                                                          GLKVector4Make(_modelCube.m30,
                                                                         _modelCube.m31,
                                                                         _modelCube.m32,
                                                                         _modelCube.m33));
    
    // Now get the up or down angle, between -20 and 20 degrees.
    float angleY = randomFloat() * 80 - 40; // Angle in Y plane, between -40 and 40.
    angleY = GLKMathDegreesToRadians(angleY);
    float newY = tanf(angleY) * _objectDistance;
    
    _modelCube = GLKMatrix4Identity;
    _modelCube = GLKMatrix4Translate(_modelCube, positionVector.x, newY, positionVector.z);
}

- (void)magneticTriggerPressed
{
    [self toggleSensorFusion];
}

- (void)handleTap:(UITapGestureRecognizer *)sender
{
    if (sender.state == UIGestureRecognizerStateEnded)
    {
        [self toggleSensorFusion];
    }
}

- (void) toggleSensorFusion
{
    if (isSensorFusionRunning)
        [self stopSensorFusion];
    else
        [self startSensorFusion];
}

- (void)startSensorFusion
{
    if (!isSensorFusionRunning)
    {
        [self.cardboard startTracking];

        isSensorFusionRunning = YES;
    }
}

- (void)stopSensorFusion
{
    if (isSensorFusionRunning)
    {
        [self.cardboard stopTracking];
        isSensorFusionRunning = NO;
    }
}

@end


@interface TreasureViewController()

@property (nonatomic) TreasureRenderer *treasureRenderer;

@end


@implementation TreasureViewController

- (instancetype)init
{
    self = [super init];
    if (!self) {return nil; }
    
    self.treasureRenderer = [TreasureRenderer new];
    self.stereoRendererDelegate = self.treasureRenderer;
    self.treasureRenderer.cardboard = self;
    
    return self;
}

- (NSUInteger)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskLandscapeRight;
}

- (BOOL)shouldAutorotate { return YES; }

@end
