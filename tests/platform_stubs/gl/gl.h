#ifndef B3D_TEST_GL_H
#define B3D_TEST_GL_H

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef unsigned int GLbitfield;
typedef unsigned char GLubyte;
typedef unsigned char GLboolean;
typedef int GLint;
typedef int GLsizei;
typedef float GLfloat;
typedef double GLdouble;

#define GL_FALSE 0U
#define GL_TRUE 1U
#define GL_TRIANGLES 0x0004U
#define GL_QUADS 0x0007U
#define GL_QUAD_STRIP 0x0008U
#define GL_LINES 0x0001U
#define GL_PROJECTION 0x1701U
#define GL_MODELVIEW 0x1700U
#define GL_DEPTH_TEST 0x0B71U
#define GL_BLEND 0x0BE2U
#define GL_SRC_ALPHA 0x0302U
#define GL_ONE_MINUS_SRC_ALPHA 0x0303U
#define GL_COLOR_BUFFER_BIT 0x00004000UL
#define GL_DEPTH_BUFFER_BIT 0x00000100UL
#define GL_LIGHTING 0x0B50U
#define GL_LIGHT0 0x4000U
#define GL_LIGHT1 0x4001U
#define GL_LIGHT2 0x4002U
#define GL_AMBIENT 0x1200U
#define GL_DIFFUSE 0x1201U
#define GL_SPECULAR 0x1202U
#define GL_CONSTANT_ATTENUATION 0x1207U
#define GL_LINEAR_ATTENUATION 0x1208U
#define GL_QUADRATIC_ATTENUATION 0x1209U
#define GL_COLOR_MATERIAL 0x0B57U
#define GL_POSITION 0x1203U
#define GL_RGB 0x1907U
#define GL_RGBA 0x1908U
#define GL_UNSIGNED_BYTE 0x1401U
#define GL_UNPACK_ALIGNMENT 0x0CF5U
#define GL_SCISSOR_TEST 0x0C11U
#define GL_TEXTURE_2D 0x0DE1U
#define GL_TEXTURE_MIN_FILTER 0x2801U
#define GL_TEXTURE_MAG_FILTER 0x2800U
#define GL_TEXTURE_WRAP_S 0x2802U
#define GL_TEXTURE_WRAP_T 0x2803U
#define GL_NEAREST 0x2600U
#define GL_LINEAR 0x2601U
#define GL_CLAMP 0x2900U
#define GL_ONE 1U
#define GL_ZERO 0U
#define GL_DST_COLOR 0x0306U
#define GL_CULL_FACE 0x0B44U
#define GL_CULL_FACE_MODE 0x0B45U
#define GL_FRONT 0x0404U
#define GL_BACK 0x0405U
#define GL_ALWAYS 0x0207U
#define GL_EQUAL 0x0202U
#define GL_LEQUAL 0x0203U
#define GL_DEPTH_FUNC 0x0B74U
#define GL_DEPTH_WRITEMASK 0x0B72U
#define GL_TEXTURE_BINDING_2D 0x8069U
#define GL_BLEND_SRC 0x0BE1U
#define GL_BLEND_DST 0x0BE0U

void glBegin(GLenum);
void glEnd(void);
void glColor3ub(GLubyte, GLubyte, GLubyte);
void glColor4ub(GLubyte, GLubyte, GLubyte, GLubyte);
void glNormal3f(GLfloat, GLfloat, GLfloat);
void glVertex2i(GLint, GLint);
void glVertex3f(GLfloat, GLfloat, GLfloat);
void glVertex3i(GLint, GLint, GLint);
void glLoadMatrixf(const GLfloat *);
void glMultMatrixf(const GLfloat *);
void glViewport(GLint, GLint, GLsizei, GLsizei);
void glScissor(GLint, GLint, GLsizei, GLsizei);
void glClearColor(GLfloat, GLfloat, GLfloat, GLfloat);
void glClear(GLbitfield);
void glEnable(GLenum);
void glDisable(GLenum);
void glDepthMask(GLboolean);
void glDepthFunc(GLenum);
void glCullFace(GLenum);
GLboolean glIsEnabled(GLenum);
void glGetBooleanv(GLenum, GLboolean *);
void glGetIntegerv(GLenum, GLint *);
void glLightfv(GLenum, GLenum, const GLfloat *);
void glLightf(GLenum, GLenum, GLfloat);
void glMatrixMode(GLenum);
void glPushMatrix(void);
void glPopMatrix(void);
void glLoadIdentity(void);
void glOrtho(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
void glBlendFunc(GLenum, GLenum);
void glRasterPos2i(GLint, GLint);
void glPixelZoom(GLfloat, GLfloat);
void glPixelStorei(GLenum, GLint);
void glDrawPixels(GLsizei, GLsizei, GLenum, GLenum, const void *);
void glGenTextures(GLsizei, GLuint *);
void glDeleteTextures(GLsizei, const GLuint *);
void glBindTexture(GLenum, GLuint);
void glTexParameteri(GLenum, GLenum, GLint);
void glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *);
void glTexCoord2f(GLfloat, GLfloat);
void glCopyTexSubImage2D(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);

#endif
