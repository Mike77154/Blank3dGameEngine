#ifndef B3D_TEST_GL_H
#define B3D_TEST_GL_H

typedef unsigned int GLenum;
typedef unsigned int GLbitfield;
typedef unsigned char GLubyte;
typedef int GLint;
typedef int GLsizei;
typedef float GLfloat;
typedef double GLdouble;

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
#define GL_COLOR_MATERIAL 0x0B57U
#define GL_POSITION 0x1203U
#define GL_RGB 0x1907U
#define GL_RGBA 0x1908U
#define GL_UNSIGNED_BYTE 0x1401U
#define GL_UNPACK_ALIGNMENT 0x0CF5U
#define GL_SCISSOR_TEST 0x0C11U

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
void glLightfv(GLenum, GLenum, const GLfloat *);
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

#endif
