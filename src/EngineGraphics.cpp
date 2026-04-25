#include "../include/EngineGraphics.h"

void PimpEx::EngineGraphics::init_graphics_state() {

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  _window = glfwCreateWindow(_windowWidth, _windowHeight, _windowTitle.c_str(),
                             NULL, NULL);
  if (!_window)
    throw std::runtime_error("Window ain't loaded");

  glfwMakeContextCurrent(_window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    throw std::runtime_error("GLADloadproc failed");

  glViewport(0, 0, _windowWidth, _windowHeight);
  glfwSetFramebufferSizeCallback(_window, frameBufferCallback);
  glEnable(GL_DEPTH_TEST);
}
void PimpEx::EngineGraphics::draw_render() {}
bool PimpEx::EngineGraphics::is_init() {}
