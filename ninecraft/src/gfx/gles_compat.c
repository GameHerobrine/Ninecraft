#include <ninecraft/gfx/gles_compat.h>
#include <GL/glext.h>

void gl_alpha_func(GLenum func, GLclampf ref) {
    glAlphaFunc(func, ref);
}

void gl_bind_buffer(GLenum target, GLuint buffer) {
    glBindBuffer(target, buffer);
}

void gl_bind_texture(GLenum target, GLuint texture) {
    glBindTexture(target, texture);
}

void gl_blend_func(GLenum sfactor, GLenum dfactor) {
    glBlendFunc(sfactor, dfactor);
}

void gl_buffer_data(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
    glBufferData(target, size, data, usage);
}

void gl_clear(GLbitfield mask) {
    glClear(mask);
}

void gl_clear_color(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    glClearColor(red, green, blue, alpha);
}

void gl_color_4_f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    glColor4f(red, green, blue, alpha);
}

void gl_color_mask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    glColorMask(red, green, blue, alpha);
}

void gl_color_pointer(GLint size, GLenum type, GLsizei stride, const void *pointer) {
    glColorPointer(size, type, stride, pointer);
}

void gl_cull_face(GLenum mode) {
    glCullFace(mode);
}

void gl_delete_buffers(GLsizei n,const GLuint *buffers) {
    glDeleteBuffers(n, buffers);
}

void gl_delete_textures(GLsizei n, const GLuint *textures) {
    glDeleteTextures(n, textures);
}

void gl_depth_func(GLenum func) {
    glDepthFunc(func);
}

void gl_depth_mask(GLboolean flag) {
    glDepthMask(flag);
}

void gl_depth_range_f(GLclampf zNear, GLclampf zFar) {
    glDepthRange((GLclampd)zNear, (GLclampd)zFar);
}

void gl_disable(GLenum cap) {
    glDisable(cap);
}

void gl_disable_client_state(GLenum array) {
    glDisableClientState(array);
}

void gl_draw_arrays(GLenum mode, GLint first, GLsizei count) {
    glDrawArrays(mode, first, count);
}

void gl_enable(GLenum cap) {
    glEnable(cap);
}

void gl_enable_client_state(GLenum array) {
    glEnableClientState(array);
}

void gl_fog_f(GLenum pname, GLfloat param) {
    glFogf(pname, param);
}

void gl_fog_f_v(GLenum pname, const GLfloat *params) {
    glFogfv(pname, params);
}

void gl_fog_x(GLenum pname, GLfixed param) {
    glFogi(pname, param);
}

void gl_gen_textures(GLsizei n, GLuint *textures) {
    glGenTextures(n, textures);
}

void gl_get_float_v(GLenum pname, GLfloat *params) {
    glGetFloatv(pname, params);
}

const GLubyte *gl_get_string(GLenum name) {
    return glGetString(name);
}

void gl_hint(GLenum target, GLenum mode) {
    glHint(target, mode);
}

void gl_line_width(GLfloat width) {
    glLineWidth(width);
}

void gl_load_identity() {
    glLoadIdentity();
}

void gl_matrix_mode(GLenum mode) {
    glMatrixMode(mode);
}

void gl_mult_matrix_f(const GLfloat *m) {
    glMultMatrixf(m);
}

void gl_normal_3_f(GLfloat nx, GLfloat ny, GLfloat nz) {
    glNormal3f(nx, ny, nz);
}

void gl_ortho_f(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar) {
    glOrtho((GLdouble)left, (GLdouble)right, (GLdouble)bottom, (GLdouble)top, (GLdouble)zNear, (GLdouble)zFar);
}

void gl_polygon_offset(GLfloat factor, GLfloat units) {
    glPolygonOffset(factor, units);
}

void gl_pop_matrix() {
    glPopMatrix();
}

void gl_push_matrix() {
    glPushMatrix();
}

void gl_read_pixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) {
    glReadPixels(x, y, width, height, format, type, pixels);
}

void gl_rotate_f(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    glRotatef(angle, x, y, z);
}

void gl_scale_f(GLfloat x, GLfloat y, GLfloat z) {
    glScalef(x, y, z);
}

void gl_scissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    glScissor(x, y, width, height);
}

void gl_shade_model(GLenum mode) {
    glShadeModel(mode);
}

void gl_tex_coord_pointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    glTexCoordPointer(size, type, stride, pointer);
}

void gl_tex_image_2_d(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLint format, GLenum type, const GLvoid *pixels) {
    glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

void gl_tex_parameter_i(GLenum target, GLenum pname, GLint param) {
    glTexParameteri(target, pname, param);
}

void gl_tex_sub_image_2_d(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
    glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void gl_translate_f(GLfloat x, GLfloat y, GLfloat z) {
    glTranslatef(x, y, z);
}

void gl_vertex_pointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    glVertexPointer(size, type, stride, pointer);
}

void gl_viewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    glViewport(x, y, width, height);
}