#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <shader/shader.hpp>

#include <iostream>
#include <string>
#include <map>

/** 콜백함수 전방 선언 */

// GLFW 윈도우 resizing 콜백함수
void framebuffer_size_callback(GLFWwindow *window, int width, int height);

// GLFW 윈도우 키 입력 콜백함수
void processInput(GLFWwindow *window);

/** 스크린 해상도 선언 */
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

int main()
{
  // GLFW 초기화 및 윈도우 설정 구성
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

// 현재 운영체제가 macos 일 경우, 미래 버전의 OpenGL 을 사용해서 GLFW 창을 생성하여 버전 호환성 이슈 해결
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // GLFW 윈도우 생성 및 현재 OpenGL 컨텍스트로 등록
  GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Text Rendering", nullptr, nullptr);
  glfwMakeContextCurrent(window);
  if (window == NULL)
  {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  // GLFW 윈도우 resizing 콜백함수 등록
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // GLAD 를 사용하여 OpenGL 표준 API 호출 시 사용할 현재 그래픽 드라이버에 구현된 함수 포인터 런타임 로드
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    // 함수 포인터 로드 실패
    std::cout << "Failed to initialized GLAD" << std::endl;
    return -1;
  }

  return 0;
}

/** 콜백함수 구현부 */

// GLFW 윈도우 키 입력 콜백함수
void processInput(GLFWwindow *window)
{
  // ESC 키 입력 시 렌더링 루프 및 프로그램 종료
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
  {
    glfwSetWindowShouldClose(window, true);
  }
}

// GLFW 윈도우 resizing 콜백함수
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
  glViewport(0, 0, width, height);
}
