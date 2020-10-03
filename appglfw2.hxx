#pragma once

#define GL_GLEXT_PROTOTYPES
#include <gl3w/GL/gl3w.h>

#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdio>
#include <cfloat>
#include <cstring>

struct vec3 {
  float x, y, z;

  vec3 operator-() const noexcept {
    return { -x, -y, -z };
  }

  friend vec3 operator+(vec3 a, vec3 b) noexcept {
    return { a.x + b.x, a.y + b.y, a.z + b.z };
  }
  friend vec3 operator-(vec3 a, vec3 b) noexcept {
    return a + -b;
  }
  friend vec3 operator*(float a, vec3 b) noexcept {
    return { a * b.x, a * b.y, a * b.z };
  }
};

struct vec4 {
  float x, y, z, w;
};

struct mat4 {
  union {
    float data[4][4];
    vec4 columns[4];
  };

  friend mat4 operator*(const mat4& a, const mat4& b) noexcept {
    mat4 m { };
    for(int col = 0; col < 4; ++col)
      for(int row = 0; row < 4; ++row)
        for(int i = 0; i < 4; ++i)
          m.data[col][row] += a.data[i][row] * b.data[col][i];
    return m;
  }
};

float radians(float degrees) {
  return M_PIf32 / 180 * degrees;
}

float clamp(float x, float min, float max) {
  return x < min ? x : x > max ? max : x;
}

inline vec3 cross(vec3 a, vec3 b) {
  return { 
    a.y * b.z - a.z * b.y, 
    a.z * b.x - a.x * b.z,
    a.x * b.y - a.y * b.x
  };
}

inline float dot(vec3 a, vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline vec3 normalize(vec3 v) {
  float len = sqrtf(dot(v, v));
  return { v.x / len, v.y / len, v.z / len };
}

inline mat4 make_perspective(float fov, float ar, float near, float far) {
  float f = 1 / tan(fov / 2);

  if(FLT_MAX == far) {
    return mat4 {
      f / ar, 0, 0,                      0,
      0,      f, 0,                      0,
      0,      0, -1,                    -1,    
      0,      0, -2 * near,              0
    }; 

  } else {
    float range = near - far;
    return mat4 {
      f / ar, 0, 0,                      0,
      0,      f, 0,                      0,
      0,      0, (far + near) / range,  -1,    
      0,      0, 2 * far * near / range, 0
    }; 
  }
}

inline mat4 make_lookat(vec3 eye, vec3 at, vec3 up) {
  vec3 zaxis = normalize(eye - at);
  vec3 xaxis = normalize(cross(up, zaxis));
  vec3 yaxis = cross(zaxis, xaxis);
  return mat4 {
    xaxis.x,          yaxis.x,          zaxis.x,          0,
    xaxis.y,          yaxis.y,          zaxis.y,          0,
    xaxis.z,          yaxis.z,          zaxis.z,          0,
    -dot(xaxis, eye), -dot(yaxis, eye), -dot(zaxis, eye), 1
  };
}

inline mat4 make_scale(vec3 scale) {
  return mat4 {
    scale.x, 0, 0, 0,
    0, scale.y, 0, 0,
    0, 0, scale.z, 0,
    0, 0, 0,       1
  };
}

inline mat4 make_translate(vec3 translate) {
  return mat4 {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    translate.x, translate.y, translate.z, 1
  };
}

inline mat4 make_rotateX(float angle) {
  float s = sin(angle);
  float c = cos(angle);
  return mat4 {
    1,  0,  0,  0,
    0,  c, -s,  0,
    0,  s,  c,  0,
    0,  0,  0,  1
  };
}

inline mat4 make_rotateY(float angle) {
  float s = sin(angle);
  float c = cos(angle);
  return mat4 {
    c,  0, -s,  0,
    0,  1,  0,  0,
    s,  0,  c,  0,
    0,  0,  0,  1
  };
}

inline mat4 make_rotateZ(float angle) {
  float s = sin(angle);
  float c = cos(angle);
  return mat4 {
    c, -s,  0,  0,
    s,  c,  0,  0,
    0,  0,  1,  0,
    0,  0,  0,  1
  };
}

inline mat4 make_rotate(vec3 angles) {
  mat4 x = make_rotateX(angles.x);
  mat4 y = make_rotateY(angles.y);
  mat4 z = make_rotateZ(angles.z);
  return z * y * x;
}

////////////////////////////////////////////////////////////////////////////////

struct camera_t {
  vec3 origin = vec3();
  float pitch = 0;
  float yaw = 0; 
  float distance = 10.f;

  // Perspective terms.
  float fov = radians(60.f);
  float near = .5;
  float far = FLT_MAX;

  void adjust(float pitch2, float yaw2, float d2);

  vec3 get_eye() const noexcept;
  mat4 get_view() const noexcept;
  mat4 get_perspective(int width, int height) const noexcept;

  mat4 get_xform(int width, int height) const noexcept;
};

inline void camera_t::adjust(float pitch2, float yaw2, float d2) {
  pitch = clamp(pitch + pitch2, -radians(80.f), radians(80.f));
  yaw = fmod(yaw + yaw2, 2 * M_PI);
  distance = exp(log(distance) + d2);
}

inline vec3 camera_t::get_eye() const noexcept {
  return vec3 {
    sinf(yaw) * cosf(pitch) * distance,
                sinf(pitch) * distance,
    cosf(yaw) * cosf(pitch) * distance
  };
}

inline mat4 camera_t::get_view() const noexcept {
  return make_lookat(get_eye(), origin, vec3 { 0, 1, 0 });
}

inline mat4 camera_t::get_perspective(int width, int height) const noexcept {
  float ar = (float)width / height;
  return make_perspective(fov, ar, near, far);
}

inline mat4 camera_t::get_xform(int width, int height) const noexcept {
  return get_perspective(width, height) * get_view();
}

////////////////////////////////////////////////////////////////////////////////

class app_t {
public:
  app_t(const char* name, int width = 800, int height = 600);
  void loop();
  virtual void display() { }

  virtual void pos_callback(int xpos, int ypos) { }
  virtual void size_callback(int width, int height) { }
  virtual void close_callback() { }
  virtual void refresh_callback() { }
  virtual void focus_callback(int focused) { }
  virtual void framebuffer_callback(int width, int height);

  virtual void cursor_callback(double xpos, double ypos);
  virtual void button_callback(int button, int action, int mods);

  virtual void key_callback(int key, int scancode, int action, int mods) { }

  virtual void debug_callback(GLenum source, GLenum type, GLuint id, 
    GLenum severity, GLsizei length, const GLchar* message);

protected:
  GLFWwindow* window = nullptr;
  camera_t camera { };
  int captured = false;
  double last_x, last_y;

private:
  static void _pos_callback(GLFWwindow* window, int xpos, int ypos);
  static void _size_callback(GLFWwindow* window, int width, int height);
  static void _close_callback(GLFWwindow* window);
  static void _refresh_callback(GLFWwindow* window);
  static void _focus_callback(GLFWwindow* window, int focused);
  static void _framebuffer_callback(GLFWwindow* window, int width, int height);

  static void _cursor_callback(GLFWwindow* window, double xpos, double ypos);
  static void _button_callback(GLFWwindow* window, int button, int action,
    int mods);

  static void _key_callback(GLFWwindow* window, int key, int scancode, 
    int action, int mods);

  static void _debug_callback(GLenum source, GLenum type, GLuint id, 
    GLenum severity, GLsizei length, const GLchar* message, 
    const void* user_param);

  void register_callbacks();
};

app_t::app_t(const char* name, int width, int height) {
  glfwWindowHint(GLFW_DOUBLEBUFFER, 1);
  glfwWindowHint(GLFW_DEPTH_BITS, 24);
  glfwWindowHint(GLFW_STENCIL_BITS, 8);
  glfwWindowHint(GLFW_SAMPLES, 4); // HQ 4x multisample.
  glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);

  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

  window = glfwCreateWindow(width, height, name, nullptr, nullptr);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  register_callbacks();

  gl3wInit();
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(_debug_callback, this);

  glfwGetWindowSize(window, &width, &height);
  glViewport(0, 0, width, height);


//  IMGUI_CHECKVERSION();
//  ImGui::CreateContext();
//  // ImGuiIO& io = ImGui::GetIO(); (void)io;
//
//  ImGui_ImplGlfw_InitForOpenGL(window, true);
//  ImGui_ImplOpenGL3_Init("#version 460");
}

void app_t::register_callbacks() {
  glfwSetWindowUserPointer(window, this);
  glfwSetWindowPosCallback(window, _pos_callback);
  glfwSetWindowSizeCallback(window, _size_callback);
  glfwSetWindowCloseCallback(window, _close_callback);
  glfwSetWindowRefreshCallback(window, _refresh_callback);
  glfwSetWindowFocusCallback(window, _focus_callback);
  glfwSetFramebufferSizeCallback(window, _framebuffer_callback);

  glfwSetCursorPosCallback(window, _cursor_callback);
  glfwSetMouseButtonCallback(window, _button_callback);

  glfwSetKeyCallback(window, _key_callback);
}

void app_t::loop() {
  while(!glfwWindowShouldClose(window)) {
    display();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}

void app_t::framebuffer_callback(int width, int height) {
  glViewport(0, 0, width, height);
}

void app_t::cursor_callback(double xpos, double ypos) {
  if(captured) {
    double dx = xpos - last_x;
    double dy = ypos - last_y;

    if(GLFW_PRESS == glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)) {
      camera.adjust(0, 0, dy / 100.f);

    } else {
      camera.adjust(-dy / 100.f, dx / 100.f, 0);
    }

    last_x = xpos;
    last_y = ypos;
  }
}

void app_t::button_callback(int button, int action, int mods) {
  bool is_release = 
    GLFW_RELEASE == glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) && 
    GLFW_RELEASE == glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);

  if(!is_release && !captured) {
    glfwGetCursorPos(window, &last_x, &last_y);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    captured = true;
    
  } else if(is_release && captured) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    captured = false;
  }
}

void app_t::debug_callback(GLenum source, GLenum type, GLuint id, 
  GLenum severity, GLsizei length, const GLchar* message) { 

  printf("OpenGL: %s\n", message);
  if(GL_DEBUG_SEVERITY_HIGH == severity ||
    GL_DEBUG_SEVERITY_MEDIUM == severity)
    exit(1);
}

void app_t::_pos_callback(GLFWwindow* window, int xpos, int ypos) {
  app_t* app = static_cast<app_t*>(glfwGetWindowUserPointer(window));
  app->pos_callback(xpos, ypos);
}
void app_t::_size_callback(GLFWwindow* window, int width, int height) {
  app_t* app = static_cast<app_t*>(glfwGetWindowUserPointer(window));
  app->size_callback(width, height);
}
void app_t::_close_callback(GLFWwindow* window) {
  app_t* app = static_cast<app_t*>(glfwGetWindowUserPointer(window));
  app->close_callback();
}
void app_t::_refresh_callback(GLFWwindow* window) {
  app_t* app = static_cast<app_t*>(glfwGetWindowUserPointer(window));
  app->refresh_callback();
}
void app_t::_focus_callback(GLFWwindow* window, int focused) {
  app_t* app = static_cast<app_t*>(glfwGetWindowUserPointer(window));
  app->focus_callback(focused);
}
void app_t::_framebuffer_callback(GLFWwindow* window, int width, int height) {
  app_t* app = static_cast<app_t*>(glfwGetWindowUserPointer(window));
  app->framebuffer_callback(width, height);
}

void app_t::_cursor_callback(GLFWwindow* window, double xpos, double ypos) {
  app_t* app = static_cast<app_t*>(glfwGetWindowUserPointer(window));
  app->cursor_callback(xpos, ypos);
}
void app_t::_button_callback(GLFWwindow* window, int button, int action,
  int mods) { 
  app_t* app = static_cast<app_t*>(glfwGetWindowUserPointer(window));
  app->button_callback(button, action, mods);
}

void app_t::_key_callback(GLFWwindow* window, int key, int scancode, 
  int action, int mods) { 
  app_t* app = static_cast<app_t*>(glfwGetWindowUserPointer(window));
  app->key_callback(key, scancode, action, mods);
}

void app_t::_debug_callback(GLenum source, GLenum type, GLuint id, 
  GLenum severity, GLsizei length, const GLchar* message, 
  const void* user_param) {

  app_t* app = (app_t*)user_param;
  app->debug_callback(source, type, id, severity, length, message);
}

////////////////////////////////////////////////////////////////////////////////