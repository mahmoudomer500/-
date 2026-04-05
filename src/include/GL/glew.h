#ifndef DUMMY_GLEW_H
#define DUMMY_GLEW_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stddef.h>

typedef unsigned int GLuint;
typedef unsigned int GLenum;
typedef int GLint;
typedef int GLsizei;
typedef float GLfloat;
typedef double GLdouble;
typedef unsigned char GLubyte;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef char GLchar;
typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;

#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006
#define GL_QUADS 0x0007
#define GL_LINES 0x0001
#define GL_LINE_STRIP 0x0003
#define GL_LINE_LOOP 0x0002
#define GL_POINTS 0x0000

#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84

#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893

#define GL_FLOAT 0x1406
#define GL_UNSIGNED_INT 0x1405
#define GL_UNSIGNED_BYTE 0x1401
#define GL_FALSE 0
#define GL_TRUE 1

#define GL_BLEND 0x0BE2
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_DEPTH_TEST 0x0B71
#define GL_CULL_FACE 0x0B44

#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100

#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_REPEAT 0x2901
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_LINEAR 0x2601
#define GL_RGBA 0x1908
#define GL_RGB 0x1907
#define GL_VERSION 0x1F02
#define GL_EXTENSIONS 0x1F03
#define GL_NUM_EXTENSIONS 0x821D
#define GL_DEBUG_OUTPUT 0x92E0
#define GL_BACK 0x0405
#define GL_CCW 0x0901
#define GL_MULTISAMPLE 0x809D

#define GLEW_OK 0
#define GLEW_VERSION_3_3 1

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001

#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

// Mock functions
typedef void (APIENTRYP PFNGLDEBUGMESSAGECALLBACKPROC)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
inline void glDebugMessageCallback(PFNGLDEBUGMESSAGECALLBACKPROC, const void*) {}
inline void glCullFace(GLenum) {}
inline void glFrontFace(GLenum) {}
inline void glViewport(GLint, GLint, GLsizei, GLsizei) {}
inline void glClearColor(GLfloat, GLfloat, GLfloat, GLfloat) {}
inline void glClear(GLbitfield) {}
inline void glEnable(GLenum) {}
inline void glDisable(GLenum) {}
inline void glBlendFunc(GLenum, GLenum) {}
inline void glUseProgram(GLuint) {}
inline void glUniformMatrix4fv(GLint, GLsizei, GLboolean, const GLfloat*) {}
inline void glUniform1f(GLint, GLfloat) {}
inline void glUniform2fv(GLint, GLsizei, const GLfloat*) {}
inline void glUniform3fv(GLint, GLsizei, const GLfloat*) {}
inline void glUniform4fv(GLint, GLsizei, const GLfloat*) {}
inline void glUniform1i(GLint, GLint) {}
inline GLint glGetUniformLocation(GLuint, const GLchar*) { return 0; }
inline void glGenVertexArrays(GLsizei, GLuint*) {}
inline void glBindVertexArray(GLuint) {}
inline void glGenBuffers(GLsizei, GLuint*) {}
inline void glBindBuffer(GLenum, GLuint) {}
inline void glBufferData(GLenum, GLsizeiptr, const void*, GLenum) {}
inline void glVertexAttribPointer(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*) {}
inline void glEnableVertexAttribArray(GLuint) {}
inline void glDisableVertexAttribArray(GLuint) {}
inline void glLineWidth(GLfloat) {}
inline void glDrawArrays(GLenum, GLint, GLsizei) {}
inline void glDrawElements(GLenum, GLsizei, GLenum, const void*) {}
inline GLuint glCreateShader(GLenum) { return 1; }
inline void glShaderSource(GLuint, GLsizei, const GLchar**, const GLint*) {}
inline void glCompileShader(GLuint) {}
inline void glGetShaderiv(GLuint, GLenum, GLint* v) { if(v) *v = 1; }
inline void glGetShaderInfoLog(GLuint, GLsizei, GLsizei*, GLchar*) {}
inline void glDeleteShader(GLuint) {}
inline GLuint glCreateProgram() { return 1; }
inline void glAttachShader(GLuint, GLuint) {}
inline void glLinkProgram(GLuint) {}
inline void glGetProgramiv(GLuint, GLenum, GLint* v) { if(v) *v = 1; }
inline void glGetProgramInfoLog(GLuint, GLsizei, GLsizei*, GLchar*) {}
inline void glDeleteProgram(GLuint) {}
inline void glDeleteBuffers(GLsizei, const GLuint*) {}
inline void glDeleteVertexArrays(GLsizei, const GLuint*) {}
inline void glGenTextures(GLsizei, GLuint*) {}
inline void glBindTexture(GLenum, GLuint) {}
inline void glTexParameteri(GLenum, GLenum, GLint) {}
inline void glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*) {}
inline void glGenerateMipmap(GLenum) {}
inline void glDeleteTextures(GLsizei, const GLuint*) {}
inline void glActiveTexture(GLenum) {}
inline void glTexSubImage2D(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void*) {}
inline void glVertexAttribDivisor(GLuint, GLuint) {}
inline void glGetIntegerv(GLenum, GLint*) {}
inline const GLubyte* glGetStringi(GLenum, GLuint) { return (const GLubyte*)"Mock"; }
inline void glDrawElementsInstanced(GLenum, GLsizei, GLenum, const void*, GLsizei) {}
inline const GLubyte* glGetString(GLenum) { return (const GLubyte*)"Mock"; }

typedef HGLRC (WINAPI * PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hDC, HGLRC hShareContext, const int *attribList);
extern PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

inline GLenum glewInit() { return GLEW_OK; }
inline const GLubyte* glewGetErrorString(GLenum) { return (const GLubyte*)"No error"; }
inline GLboolean glewIsSupported(const char*) { return 1; }

#endif // DUMMY_GLEW_H