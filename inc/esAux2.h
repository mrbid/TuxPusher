/*
--------------------------------------------------
    James William Fletcher (github.com/mrbid)
        December 2022 - esAux2.h v2.2.haxed
--------------------------------------------------

    A pretty good color converter: https://www.easyrgb.com/en/convert.php

    !!! I've hacked this version so that opacity now specifies the ambient light amount !!!

    Requires:
        - vec.h: https://gist.github.com/mrbid/77a92019e1ab8b86109bf103166bd04e
        - mat.h: https://gist.github.com/mrbid/cbc69ec9d99b0fda44204975fcbeae7c
    
    v2.2: [December 2022]
        - removed phong support and optimised shaders.

    v2.1: [November 2022]
        - fixed issues in shaders

    v2.0: [October 2021]
        - added support for fullbright texture mapping
        - added esRebind()

    v1.0: [December 2020]
        The shaders are the product of a little help from:
        http://www.cs.toronto.edu/~jacobson/phong-demo/
        https://www.mathematik.uni-marburg.de/~thormae/lectures/graphics1/code/WebGLShaderLightMat/ShaderLightMat.html
        https://www.gsn-lib.org/docs/nodes/ShaderPluginNode.php
        https://www.khronos.org/registry/OpenGL/specs/es/2.0/GLSL_ES_Specification_1.00.pdf
        https://www.khronos.org/opengles/sdk/docs/reference_cards/OpenGL-ES-2_0-Reference-card.pdf
        https://www.khronos.org/registry/OpenGL-Refpages/es2.0/

        The phong uses BlinnPhong by default, if you wish to use
        vanilla phong just specify the following definition in
        your source file: #define REGULAR_PHONG
*/

#ifndef AUX_H
#define AUX_H

#include "vec.h"
#include "mat.h"

//*************************************
// MATRIX
//*************************************

// structures
typedef struct
{
    GLuint vid; // Vertex Array Buffer ID
    GLuint iid;	// Index Array Buffer ID
    GLuint cid;	// Colour Array Buffer ID
    GLuint nid;	// Normal Array Buffer ID
    GLuint tid;	// TexCoord Array Buffer ID
} ESModel;

//*************************************
// UTILITY
//*************************************

GLuint esRand(const GLuint min, const GLuint max);
GLfloat esRandFloat(const GLfloat min, const GLfloat max);
void esBind(const GLenum target, GLuint* buffer, const void* data, const GLsizeiptr datalen, const GLenum usage);
void esRebind(const GLenum target, GLuint* buffer, const void* data, const GLsizeiptr datalen, const GLenum usage);
void esBindModel(ESModel* model, const GLfloat* vertices, const GLsizei vertlen, const GLushort* indices, const GLsizei indlen);
GLuint esLoadTexture(const GLuint w, const GLuint h, const unsigned char* data);
GLuint esLoadTextureA(const GLuint w, const GLuint h, const unsigned char* data);

//*************************************
// SHADER
//*************************************

GLuint debugShader(GLuint shader_program);

void makeAllShaders();

void makeFullbright();
void makeLambert();
void makeLambert1();
void makeLambert2();
void makeLambert3();
void makePhong();
void makePhong1();
void makePhong2();
void makePhong3();

void shadeFullbrightT(GLint* position, GLint* projection, GLint* modelview, GLint* texcoord, GLint* sampler);                             // texture + no shading
void shadeFullbright(GLint* position, GLint* projection, GLint* modelview, GLint* color, GLint* opacity);                                 // solid color + no shading

void shadeLambert(GLint* position, GLint* projection, GLint* modelview, GLint* lightpos, GLint* color, GLint* opacity);                   // solid color + no normals
void shadeLambert1(GLint* position, GLint* projection, GLint* modelview, GLint* lightpos, GLint* normal, GLint* color, GLint* opacity);   // solid color + normals
void shadeLambert2(GLint* position, GLint* projection, GLint* modelview, GLint* lightpos, GLint* color, GLint* opacity);                  // colors + no normals
void shadeLambert3(GLint* position, GLint* projection, GLint* modelview, GLint* lightpos, GLint* normal, GLint* color, GLint* opacity);   // colors + normals

//*************************************
// UTILITY CODE
//*************************************

GLuint esRand(const GLuint min, const GLuint max)
{
    return (rand()%(max+1-min))+min;
}

GLfloat esRandFloat(const GLfloat min, const GLfloat max)
{
    static GLfloat rrndmax = 1.f/(GLfloat)RAND_MAX;
    return (((GLfloat)rand()) * rrndmax) * (max-min) + min;
}

void esBind(const GLenum target, GLuint* buffer, const void* data, const GLsizeiptr datalen, const GLenum usage)
{
    glGenBuffers(1, buffer);
    glBindBuffer(target, *buffer);
    glBufferData(target, datalen, data, usage);
}

void esRebind(const GLenum target, GLuint* buffer, const void* data, const GLsizeiptr datalen, const GLenum usage)
{
    glBindBuffer(target, *buffer);
    glBufferData(target, datalen, data, usage);
}

void esBindModel(ESModel* model, const GLfloat* vertices, const GLsizei vertlen, const GLushort* indices, const GLsizei indlen)
{
    esBind(GL_ARRAY_BUFFER, &model->vid, vertices, vertlen * sizeof(GLfloat) * 3, GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &model->iid, indices, indlen * sizeof(GLushort), GL_STATIC_DRAW);
}

GLuint esLoadTexture(const GLuint w, const GLuint h, const unsigned char* data)
{
   GLuint textureId;

   // Use tightly packed data
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   // Generate a texture object
   glGenTextures(1, &textureId);

   // Bind the texture object
   glBindTexture(GL_TEXTURE_2D, textureId);

   // Load the texture
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

   // Set the filtering mode
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

   return textureId;
}

GLuint esLoadTextureA(const GLuint w, const GLuint h, const unsigned char* data)
{
   GLuint textureId;
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glGenTextures(1, &textureId);
   glBindTexture(GL_TEXTURE_2D, textureId);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   return textureId;
}

#if defined(GL_DEBUG) && !defined(__MINGW32__)
// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glDebugMessageControl.xhtml
// https://registry.khronos.org/OpenGL-Refpages/es3/html/glDebugMessageControl.xhtml
// OpenGL ES 3.2 or OpenGL 4.3 and above only.
void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
    printf("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
        type, severity, message );
}

void esDebug(const GLuint state)
{
    if(state == 1)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(MessageCallback, 0);
        glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
    }
    else
    {
        glDisable(GL_DEBUG_OUTPUT);
        glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }
}
#endif

GLuint debugShader(GLuint shader_program)
{
    GLint linked;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &linked);
    if(linked == GL_FALSE) 
    {
        GLint infoLen = 0;
        glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &infoLen);
        if(infoLen > 1)
        {
            char* infoLog = malloc(sizeof(char) * infoLen);
            if(infoLog != NULL)
            {
                glGetProgramInfoLog(shader_program, infoLen, NULL, infoLog);
                printf("!!! error linking shader !!!\n%s\n", infoLog);            
                free(infoLog);
            }
        }
        else
        {
            printf("!!! failed to link shader with no returned debug output !!!\n");
        }
        glDeleteProgram(shader_program);
        return linked;
    }
    return linked;
}

//*************************************
// SHADER CODE
//*************************************

const GLchar* vt0 =
    "#version 100\n"
    "uniform mat4 modelview;\n"
    "uniform mat4 projection;\n"
    "attribute vec4 position;\n"
    "attribute vec2 texcoord;\n"
    "varying vec2 vtc;\n"
    "void main()\n"
    "{\n"
        "vtc = texcoord;\n"
        "gl_Position = projection * modelview * position;\n"
    "}\n";

const GLchar* ft0 =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec2 vtc;\n"
    "uniform sampler2D tex;\n"
    "void main()\n"
    "{\n"
        "gl_FragColor = texture2D(tex, vtc);\n"
    "}\n";

//

// fullbright
const GLchar* v0 =
    "#version 100\n"
    "uniform mat4 modelview;\n"
    "uniform mat4 projection;\n"
    "uniform vec3 color;\n"
    "uniform float opacity;\n"
    "attribute vec4 position;\n"
    "varying vec4 fragcolor;\n"
    "void main()\n"
    "{\n"
        "fragcolor = vec4(color, opacity);\n"
        "gl_Position = projection * modelview * position;\n"
    "}\n";

// solid color + no normals
const GLchar* v1 =
    "#version 100\n"
    "uniform mat4 modelview;\n"
    "uniform mat4 projection;\n"
    "uniform vec3 color;\n"
    "uniform float opacity;\n"
    "uniform vec3 lightpos;\n"
    "attribute vec4 position;\n"
    "varying vec4 fragcolor;\n"
    "void main()\n"
    "{\n"
        "vec4 vertPos4 = modelview * position;\n"
        "vec3 vertPos = vertPos4.xyz / vertPos4.w;\n"
        "vec3 vertNorm = normalize(vec3(modelview * vec4(normalize(position.xyz), 0.0)));\n"
        "vec3 lightDir = normalize(lightpos - vertPos);\n"
        "fragcolor = vec4((color * opacity) + max(dot(lightDir, vertNorm), 0.0)*color, opacity);\n"
        "gl_Position = projection * vertPos4;\n"
    "}\n";

// solid color + normal array
const GLchar* v11 =
    "#version 100\n"
    "uniform mat4 modelview;\n"
    "uniform mat4 projection;\n"
    "uniform vec3 color;\n"
    "uniform float opacity;\n"
    "uniform vec3 lightpos;\n"
    "attribute vec4 position;\n"
    "attribute vec3 normal;\n"
    "varying vec4 fragcolor;\n"
    "void main()\n"
    "{\n"
        "vec4 vertPos4 = modelview * position;\n"
        "vec3 vertPos = vertPos4.xyz / vertPos4.w;\n"
        "vec3 vertNorm = normalize(vec3(modelview * vec4(normal, 0.0)));\n"
        "vec3 lightDir = normalize(lightpos - vertPos);\n"
        "fragcolor = vec4((color * opacity) + max(dot(lightDir, vertNorm), 0.0)*color, opacity);\n"
        "gl_Position = projection * vertPos4;\n"
    "}\n";

// color array + no normals
const GLchar* v12 =
    "#version 100\n"
    "uniform mat4 modelview;\n"
    "uniform mat4 projection;\n"
    "uniform float opacity;\n"
    "uniform vec3 lightpos;\n"
    "attribute vec4 position;\n"
    "attribute vec3 color;\n"
    "varying vec4 fragcolor;\n"
    "void main()\n"
    "{\n"
        "vec4 vertPos4 = modelview * position;\n"
        "vec3 vertPos = vertPos4.xyz / vertPos4.w;\n"
        "vec3 vertNorm = normalize(vec3(modelview * vec4(normalize(position.xyz), 0.0)));\n"
        "vec3 lightDir = normalize(lightpos - vertPos);\n"
        "fragcolor = vec4((color * opacity) + max(dot(lightDir, vertNorm), 0.0)*color, opacity);\n"
        "gl_Position = projection * vertPos4;\n"
    "}\n";

// color array + normal array
const GLchar* v13 =
    "#version 100\n"
    "uniform mat4 modelview;\n"
    "uniform mat4 projection;\n"
    "uniform float opacity;\n"
    "uniform vec3 lightpos;\n"
    "attribute vec4 position;\n"
    "attribute vec3 normal;\n"
    "attribute vec3 color;\n"
    "varying vec4 fragcolor;\n"
    "void main()\n"
    "{\n"
        "vec4 vertPos4 = modelview * position;\n"
        "vec3 vertPos = vertPos4.xyz / vertPos4.w;\n"
        "vec3 vertNorm = normalize(vec3(modelview * vec4(normal, 0.0)));\n"
        "vec3 lightDir = normalize(lightpos - vertPos);\n"
        "fragcolor = vec4((color * opacity) + max(dot(lightDir, vertNorm), 0.0)*color, opacity);\n"
        "gl_Position = projection * vertPos4;\n"
    "}\n";

const GLchar* f0 =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec4 fragcolor;\n"
    "void main()\n"
    "{\n"
        "gl_FragColor = fragcolor;\n"
    "}\n";

//

GLuint shdFullbrightT;
GLint  shdFullbrightT_position;
GLint  shdFullbrightT_projection;
GLint  shdFullbrightT_modelview;
GLint  shdFullbrightT_texcoord;
GLint  shdFullbrightT_sampler;
GLuint shdFullbright;
GLint  shdFullbright_position;
GLint  shdFullbright_projection;
GLint  shdFullbright_modelview;
GLint  shdFullbright_color;
GLint  shdFullbright_opacity;
GLuint shdLambert;
GLint  shdLambert_position;
GLint  shdLambert_projection;
GLint  shdLambert_modelview;
GLint  shdLambert_lightpos;
GLint  shdLambert_color;
GLint  shdLambert_opacity;
GLuint shdLambert1;
GLint  shdLambert1_position;
GLint  shdLambert1_projection;
GLint  shdLambert1_modelview;
GLint  shdLambert1_lightpos;
GLint  shdLambert1_color;
GLint  shdLambert1_normal;
GLint  shdLambert1_opacity;
GLuint shdLambert2;
GLint  shdLambert2_position;
GLint  shdLambert2_projection;
GLint  shdLambert2_modelview;
GLint  shdLambert2_lightpos;
GLint  shdLambert2_color;
GLint  shdLambert2_opacity;
GLuint shdLambert3;
GLint  shdLambert3_position;
GLint  shdLambert3_projection;
GLint  shdLambert3_modelview;
GLint  shdLambert3_lightpos;
GLint  shdLambert3_color;
GLint  shdLambert3_normal;
GLint  shdLambert3_opacity;

//

void makeFullbrightT()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vt0, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &ft0, NULL);
    glCompileShader(fragmentShader);

    shdFullbrightT = glCreateProgram();
        glAttachShader(shdFullbrightT, vertexShader);
        glAttachShader(shdFullbrightT, fragmentShader);
    glLinkProgram(shdFullbrightT);

    if(debugShader(shdFullbrightT) == GL_FALSE){return;}

    shdFullbrightT_position   = glGetAttribLocation(shdFullbrightT,  "position");
    shdFullbrightT_texcoord   = glGetAttribLocation(shdFullbrightT,  "texcoord");

    shdFullbrightT_projection = glGetUniformLocation(shdFullbrightT, "projection");
    shdFullbrightT_modelview  = glGetUniformLocation(shdFullbrightT, "modelview");
    shdFullbrightT_sampler    = glGetUniformLocation(shdFullbrightT, "tex");
}

void makeFullbright()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &v0, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &f0, NULL);
    glCompileShader(fragmentShader);

    shdFullbright = glCreateProgram();
        glAttachShader(shdFullbright, vertexShader);
        glAttachShader(shdFullbright, fragmentShader);
    glLinkProgram(shdFullbright);

    if(debugShader(shdFullbright) == GL_FALSE){return;}

    shdFullbright_position = glGetAttribLocation(shdFullbright,    "position");
    
    shdFullbright_projection = glGetUniformLocation(shdFullbright, "projection");
    shdFullbright_modelview = glGetUniformLocation(shdFullbright,  "modelview");
    shdFullbright_color = glGetUniformLocation(shdFullbright,      "color");
    shdFullbright_opacity = glGetUniformLocation(shdFullbright,    "opacity");
}

void makeLambert()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &v1, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &f0, NULL);
    glCompileShader(fragmentShader);

    shdLambert = glCreateProgram();
        glAttachShader(shdLambert, vertexShader);
        glAttachShader(shdLambert, fragmentShader);
    glLinkProgram(shdLambert);

    if(debugShader(shdLambert) == GL_FALSE){return;}

    shdLambert_position = glGetAttribLocation(shdLambert,    "position");
    
    shdLambert_projection = glGetUniformLocation(shdLambert, "projection");
    shdLambert_modelview = glGetUniformLocation(shdLambert,  "modelview");
    shdLambert_lightpos = glGetUniformLocation(shdLambert,   "lightpos");
    shdLambert_color = glGetUniformLocation(shdLambert,      "color");
    shdLambert_opacity = glGetUniformLocation(shdLambert,    "opacity");
}

void makeLambert1()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &v11, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &f0, NULL);
    glCompileShader(fragmentShader);

    shdLambert1 = glCreateProgram();
        glAttachShader(shdLambert1, vertexShader);
        glAttachShader(shdLambert1, fragmentShader);
    glLinkProgram(shdLambert1);

    if(debugShader(shdLambert1) == GL_FALSE){return;}

    shdLambert1_position = glGetAttribLocation(shdLambert1,    "position");
    shdLambert1_normal = glGetAttribLocation(shdLambert1,      "normal");
    
    shdLambert1_projection = glGetUniformLocation(shdLambert1, "projection");
    shdLambert1_modelview = glGetUniformLocation(shdLambert1,  "modelview");
    shdLambert1_lightpos = glGetUniformLocation(shdLambert1,   "lightpos");
    shdLambert1_color = glGetUniformLocation(shdLambert1,      "color");
    shdLambert1_opacity = glGetUniformLocation(shdLambert1,    "opacity");
}

void makeLambert2()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &v12, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &f0, NULL);
    glCompileShader(fragmentShader);

    shdLambert2 = glCreateProgram();
        glAttachShader(shdLambert2, vertexShader);
        glAttachShader(shdLambert2, fragmentShader);
    glLinkProgram(shdLambert2);

    if(debugShader(shdLambert2) == GL_FALSE){return;}

    shdLambert2_position = glGetAttribLocation(shdLambert2,    "position");
    shdLambert2_color = glGetAttribLocation(shdLambert2,       "color");
    
    shdLambert2_projection = glGetUniformLocation(shdLambert2, "projection");
    shdLambert2_modelview = glGetUniformLocation(shdLambert2,  "modelview");
    shdLambert2_lightpos = glGetUniformLocation(shdLambert2,   "lightpos");
    shdLambert2_opacity = glGetUniformLocation(shdLambert2,    "opacity");
}

void makeLambert3()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &v13, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &f0, NULL);
    glCompileShader(fragmentShader);

    shdLambert3 = glCreateProgram();
        glAttachShader(shdLambert3, vertexShader);
        glAttachShader(shdLambert3, fragmentShader);
    glLinkProgram(shdLambert3);

    if(debugShader(shdLambert3) == GL_FALSE){return;}

    shdLambert3_position = glGetAttribLocation(shdLambert3,    "position");
    shdLambert3_normal = glGetAttribLocation(shdLambert3,      "normal");
    shdLambert3_color = glGetAttribLocation(shdLambert3,       "color");
    
    shdLambert3_projection = glGetUniformLocation(shdLambert3, "projection");
    shdLambert3_modelview = glGetUniformLocation(shdLambert3,  "modelview");
    shdLambert3_lightpos = glGetUniformLocation(shdLambert3,   "lightpos");
    shdLambert3_opacity = glGetUniformLocation(shdLambert3,    "opacity");
}

void makeAllShaders()
{
    makeFullbrightT();
    makeFullbright();
    makeLambert();
    makeLambert1();
    makeLambert2();
    makeLambert3();
}

void shadeFullbrightT(GLint* position, GLint* projection, GLint* modelview, GLint* texcoord, GLint* sampler)
{
    *position = shdFullbrightT_position;
    *projection = shdFullbrightT_projection;
    *modelview = shdFullbrightT_modelview;
    *texcoord = shdFullbrightT_texcoord;
    *sampler = shdFullbrightT_sampler;
    glUseProgram(shdFullbrightT);
}

void shadeFullbright(GLint* position, GLint* projection, GLint* modelview, GLint* color, GLint* opacity)
{
    *position = shdFullbright_position;
    *projection = shdFullbright_projection;
    *modelview = shdFullbright_modelview;
    *color = shdFullbright_color;
    *opacity = shdFullbright_opacity;
    glUseProgram(shdFullbright);
}

void shadeLambert(GLint* position, GLint* projection, GLint* modelview, GLint* lightpos, GLint* color, GLint* opacity)
{
    *position = shdLambert_position;
    *projection = shdLambert_projection;
    *modelview = shdLambert_modelview;
    *lightpos = shdLambert_lightpos;
    *color = shdLambert_color;
    *opacity = shdLambert_opacity;
    glUseProgram(shdLambert);
}

void shadeLambert1(GLint* position, GLint* projection, GLint* modelview, GLint* lightpos, GLint* normal, GLint* color, GLint* opacity)
{
    *position = shdLambert1_position;
    *projection = shdLambert1_projection;
    *modelview = shdLambert1_modelview;
    *lightpos = shdLambert1_lightpos;
    *color = shdLambert1_color;
    *normal = shdLambert1_normal;
    *opacity = shdLambert1_opacity;
    glUseProgram(shdLambert1);
}

void shadeLambert2(GLint* position, GLint* projection, GLint* modelview, GLint* lightpos, GLint* color, GLint* opacity)
{
    *position = shdLambert2_position;
    *projection = shdLambert2_projection;
    *modelview = shdLambert2_modelview;
    *lightpos = shdLambert2_lightpos;
    *color = shdLambert2_color;
    *opacity = shdLambert2_opacity;
    glUseProgram(shdLambert2);
}

void shadeLambert3(GLint* position, GLint* projection, GLint* modelview, GLint* lightpos, GLint* normal, GLint* color, GLint* opacity)
{
    *position = shdLambert3_position;
    *projection = shdLambert3_projection;
    *modelview = shdLambert3_modelview;
    *lightpos = shdLambert3_lightpos;
    *color = shdLambert3_color;
    *normal = shdLambert3_normal;
    *opacity = shdLambert3_opacity;
    glUseProgram(shdLambert3);
}

#endif