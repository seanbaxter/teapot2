#include "appglfw2.hxx"
#include "teapot.h"
#include <vector>

std::vector<char> read_file(const char* filename) {
  FILE* f = fopen(filename, "r");

  // Should really use stat here.
  fseek(f, 0, SEEK_END);
  size_t length = ftell(f);
  fseek(f, 0, SEEK_SET);

  std::vector<char> v;
  v.resize(length);

  // Read the data.
  fread(v.data(), sizeof(char), length, f);

  // Close the file.
  fclose(f);

  // Return the file.
  return v;
}

struct myapp_t : app_t {
  myapp_t();

  void display() override;
  void key_callback(int key, int scancode, int action, int mods) override;

  GLuint programs[3];
  GLuint vao;

  // Indicate the current program.
  int current = 0;
  vec4 tess_terms[3];
  vec3 tess_limits[3];
};

myapp_t::myapp_t() : app_t("Tessellation sample") {
  // Load the SPIV module.
  std::vector<char> data = read_file("teapot.spv");

  // Load and compile the shaders.
  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  GLuint ts0 = glCreateShader(GL_TESS_CONTROL_SHADER);
  GLuint ts1 = glCreateShader(GL_TESS_CONTROL_SHADER);
  GLuint ts2 = glCreateShader(GL_TESS_CONTROL_SHADER);
  GLuint es = glCreateShader(GL_TESS_EVALUATION_SHADER);
  GLuint gs = glCreateShader(GL_GEOMETRY_SHADER);
  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  GLuint shaders[] { vs, ts0, ts1, ts2, es, gs, fs };

  glShaderBinary(7, shaders, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB,
    data.data(), data.size());

  glSpecializeShader(vs, "_Z11vert_shaderv", 0, nullptr, nullptr);
  glSpecializeShader(ts0, "_Z11tesc_shaderIXadL_Z15tess_level_evenDv3_fS0_EEEvv", 0, 
    nullptr, nullptr);
  glSpecializeShader(ts1, "_Z11tesc_shaderIXadL_Z19tess_level_distanceDv3_fS0_EEEvv", 0, 
    nullptr, nullptr);
  glSpecializeShader(ts2, "_Z11tesc_shaderIXadL_Z15tess_level_edgeDv3_fS0_EEEvv", 0, 
    nullptr, nullptr);
  glSpecializeShader(es, "_Z11tese_shaderv", 0, nullptr, nullptr);
  glSpecializeShader(gs, "_Z11geom_shaderv", 0, nullptr, nullptr);  
  glSpecializeShader(fs, "_Z11frag_shaderv", 0, nullptr, nullptr);

  GLuint tesc[3] { ts0, ts1, ts2 };
  for(int i = 0; i < 3; ++i) { 
    programs[i] = glCreateProgram();
    glAttachShader(programs[i], vs);
    glAttachShader(programs[i], tesc[i]);
    glAttachShader(programs[i], es);
    glAttachShader(programs[i], gs);
    glAttachShader(programs[i], fs);
    glLinkProgram(programs[i]);
  }

  // Initialize the VBO with vertices.
  GLuint vbo;
  glCreateBuffers(1, &vbo);
  glNamedBufferStorage(vbo, sizeof(TeapotVertices), TeapotVertices, 0);
  printf("Created VBO\n");

  // Initialize the IBO with indices.
  GLuint ibo;
  glCreateBuffers(1, &ibo);
  glNamedBufferStorage(ibo, sizeof(TeapotIndices), TeapotIndices, 0);
  printf("Created IBO\n");

  // Create the VAO and select the VBO and IBO buffers.
  glCreateVertexArrays(1, &vao);
  glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(vec3));
  glVertexArrayElementBuffer(vao, ibo);

  glEnableVertexArrayAttrib(vao, 0);
  glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, false, 0);
  glVertexArrayAttribBinding(vao, 0, 0);

  // Set the tessellation terms.

  // Flat level.
  // Start at tess level 1. Range between 1 and 10.
  tess_terms[0].x = 1;
  tess_limits[0] = vec3 { .1f, 1, 20 };

  // Distance based.
  // Start at tess level 1. Range between 1 and 12.
  tess_terms[1] = vec4 { 1, .05f, 5.0f };
  tess_limits[1] = vec3 { .1f, 1, 12 };

  // Edge based.
  tess_terms[2].x = 200;
  tess_limits[2] = vec3 { 1, 1, 400 };
}

void myapp_t::display() {
  const float bg[4] { .5f, .5f, .5f, 1 };
  glClearBufferfv(GL_COLOR, 0, bg);

  // Setup the device.
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glClear(GL_DEPTH_BUFFER_BIT);

  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);

  glUseProgram(programs[current]);
  glBindVertexArray(vao);

  vec3 eye = camera.get_eye();
  mat4 view = camera.get_view();

  int width, height;
  glfwGetWindowSize(window, &width, &height);

  tess_terms[current].w = width;

  vec4 color_scale { 1, 1, 1, 1 };

  glUniform4fv(0, 1, &tess_terms[current].x);
  glUniform3fv(1, 1, &eye.x);

  // Translate the teapot to the center of the coordinate system.
  vec3 min_vec { TeapotMinX, TeapotMinY, TeapotMinZ };
  vec3 max_vec { TeapotMaxX, TeapotMaxY, TeapotMaxZ };
  mat4 translate = make_translate(-.5f * (min_vec + max_vec));

  // Put the teapot in the y-is-up orientation.
  mat4 rotateX = make_rotateX(radians(90.f));

  mat4 perspective = camera.get_perspective(width, height);
  mat4 clip = perspective * view * rotateX * translate;
  glUniformMatrix4fv(2, 1, false, &clip.data[0][0]);
  glUniform4fv(3, 1, &color_scale.x);

  // Draw the wireframe.
  glDepthFunc(GL_LEQUAL);
  glLineWidth(1.0f);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glPolygonOffset(1, -10);
  color_scale = vec4 { 0, 0, 0, 1 };
  glUniform4fv(3, 1, &color_scale.x);

  // Draw all patches.
  glPatchParameteri(GL_PATCH_VERTICES, 16);
  glDrawElements(GL_PATCHES, NumTeapotIndices, GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);
  glUseProgram(0);
}

void myapp_t::key_callback(int key, int scancode, int action, int mods) {
  switch(key) {
    case GLFW_KEY_UP: {
      vec3 limits = tess_limits[current];
      tess_terms[current].x = clamp(tess_terms[current].x + limits.x, 
        limits.y, limits.z);
      break;
    }

    case GLFW_KEY_DOWN: {
      vec3 limits = tess_limits[current];
      tess_terms[current].x = clamp(tess_terms[current].x - limits.x, 
        limits.y, limits.z);
      break;
    }

    case GLFW_KEY_1:
      current = 0;
      break;

    case GLFW_KEY_2:
      current = 1;
      break;

    case GLFW_KEY_3:
      current = 2;
      break;

    default:
      break;
  }
}

int main() {
  glfwInit();
  myapp_t app;
  app.loop();

  return 0;
}