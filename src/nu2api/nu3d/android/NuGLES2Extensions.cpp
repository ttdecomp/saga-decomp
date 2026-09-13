#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "nu2api/nu3d/android/NuGLES2Extensions.h"
#include "decomp.h"

// Android stores these entry points as exported data symbols. Host GL libraries
// can export functions with the same names, so this table is target-only.
extern "C" {
    void (*glGetProgramBinaryOES)(GLuint, GLsizei, GLsizei *, GLenum *, void *);
    void (*glProgramBinaryOES)(GLuint, GLenum, const void *, GLint);
    void (*glDiscardFramebufferEXT)(GLenum, GLsizei, const GLenum *);
    void (*glGenVertexArraysOES)(GLsizei, GLuint *);
    void (*glBindVertexArrayOES)(GLuint);
    void (*glDeleteVertexArraysOES)(GLsizei, const GLuint *);
}

SAGA_HOST_WEAK void NuGLES2ExtensionsInit() {
    glGetProgramBinaryOES =
        reinterpret_cast<decltype(glGetProgramBinaryOES)>(eglGetProcAddress("glGetProgramBinaryOES"));
    glProgramBinaryOES = reinterpret_cast<decltype(glProgramBinaryOES)>(eglGetProcAddress("glProgramBinaryOES"));
    glDiscardFramebufferEXT =
        reinterpret_cast<decltype(glDiscardFramebufferEXT)>(eglGetProcAddress("glDiscardFramebufferEXT"));
    glGenVertexArraysOES = reinterpret_cast<decltype(glGenVertexArraysOES)>(eglGetProcAddress("glGenVertexArraysOES"));
    glBindVertexArrayOES = reinterpret_cast<decltype(glBindVertexArrayOES)>(eglGetProcAddress("glBindVertexArrayOES"));
    glDeleteVertexArraysOES =
        reinterpret_cast<decltype(glDeleteVertexArraysOES)>(eglGetProcAddress("glDeleteVertexArraysOES"));
}

extern "C" void glGenVertexArraysOESC(GLsizei count, GLuint *arrays) {
    glGenVertexArraysOES(count, arrays);
}

extern "C" void glDeleteVertexArraysOESC(GLsizei count, const GLuint *arrays) {
    glDeleteVertexArraysOES(count, arrays);
}
