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

/** FreeType 라이브러리로 로드한 glyph metrices(각 글꼴의 크기, 위치, baseline 등)를 파싱할 자료형 정의 */
struct Character
{
  unsigned int TextureID; // FreeType 내부에서 렌더링된 각 glyph 의 텍스쳐 버퍼 ID
  glm::ivec2 Size;        // glyph 크기
  glm::ivec2 Bearing;     // glyph 원점에서 x축, y축 방향으로 각각 떨어진 offset
  unsigned int Advance;   // 현재 glyph 원점에서 다음 glyph 원점까지의 거리 (1/64px 단위로 정의되어 있으므로, 값 사용 시 1px 단위로 변환해야 함.)
};
std::map<GLchar, Character> Characters;

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

  /** OpenGL 전역 상태 설정 */

  // 2D Quad 를 2D View 로(= orthogonal 투영으로 상단에서) 렌더링할 것이므로 불필요한 은면 제거
  glEnable(GL_CULL_FACE);

  // 2D Quad 에서 glyph background 는 투명 처리하기 위해 blending mode 활성화
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  /** Text Rendering 쉐이더 생성 및 투영행렬 계산 */

  // 쉐이더 객체 생성 및 바인딩
  Shader shader("resources/shaders/text.vs", "resources/shaders/text.fs");
  shader.use();

  // orthogonal 투영행렬 계산 및 쉐이더에 전송
  // orthogonal 투영행렬의 left, right, top, bottom 을 아래와 같이 정의하면, vertex position 을 screen space 좌표계로 정의하여 사용할 수 있음.
  // -> 텍스트 위치(= 2D Quad 위치)는 아무래도 screen space 좌표계로 정의하는 게 더 직관적이니까!
  glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));
  shader.setMat4("projection", projection);

  /** FreeType 라이브러리 초기화 */
  FT_Library ft;
  if (FT_Init_FreeType(&ft))
  {
    // FreeType 라이브러리 초기화 실패 -> FreeType 함수들은 에러 발생 시 0 이 아닌 값을 반환.
    std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
    return -1;
  }

  /** FT_Face 인터페이스로 .ttf 파일 로드 */
  FT_Face face;
  if (FT_New_Face(ft, "resources/fonts/Antonio-Bold.ttf", 0, &face))
  {
    // .ttf 파일 로드 실패
    std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
    return -1;
  }
  else
  {
    // .ttf 파일 로드 성공 시 작업들 처리

    // .ttf 파일로부터 렌더링할 glyph 들의 pixel size 설정 -> height 값만 설정하고 width 는 각 glyph 형태에 따라 동적으로 계산하도록 0 으로 지정
    FT_Set_Pixel_Sizes(face, 0, 48);

    // glyph 가 렌더링된 grayscale bitmap 의 텍스쳐 데이터 정렬 단위 변경 (하단 필기 참고)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // 128 개의 ASCII 문자들의 glyph 들을 8-bit grayscale bitmap 텍스쳐 버퍼에 렌더링
    for (unsigned char c = 0; c < 128; c++)
    {
      if (FT_Load_Char(face, c, FT_LOAD_RENDER))
      {
        // 각 ASCII 문자에 해당하는 glyph 로드 실패
        std::cout << "ERROR::FREETYPE: Failed to load Glyph" << std::endl;
        continue;
      }

      // 각 glyph grayscale bitmap 텍스쳐 생성 및 bitmap 데이터 복사
      unsigned int texture;
      glGenTextures(1, &texture);
      glBindTexture(GL_TEXTURE_2D, texture);
      glTexImage2D(
          GL_TEXTURE_2D,
          0,
          GL_RED,
          face->glyph->bitmap.width,
          face->glyph->bitmap.rows, // bitmap 버퍼 줄 수 = bitmap 버퍼 height
          0,
          GL_RED,
          GL_UNSIGNED_BYTE,
          face->glyph->bitmap.buffer);

      // 텍스쳐 파라미터 설정
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      // 로드된 glyph metrices 를 커스텀 자료형으로 파싱
      Character character = {
          texture,
          glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
          glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
          static_cast<unsigned int>(face->glyph->advance.x)};
      // std::map 컨테이너는 std::pair 를 node 로 삼아 key-value 쌍을 추가함. (참고로, std::map 은 red-black tree 기반의 컨테이너)
      Characters.insert(std::pair<char, Character>(c, character));
    }

    // 각 glyph 들의 텍스쳐 생성 완료 후 텍스쳐 바인딩 해제
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  // FreeType 라이브러리 사용 완료 후 리소스 메모리 반납
  FT_Done_Face(face);
  FT_Done_FreeType(ft);

  /** rendering loop */
  while (!glfwWindowShouldClose(window))
  {
    processInput(window);

    // 버퍼 초기화
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Back 버퍼에 렌더링된 최종 이미지를 Front 버퍼에 교체 -> blinking 현상 방지
    glfwSwapBuffers(window);

    // 키보드, 마우스 입력 이벤트 발생 검사 후 등록된 콜백함수 호출 + 이벤트 발생에 따른 GLFWwindow 상태 업데이트
    glfwPollEvents();
  }

  // GLFW 종료 및 메모리 반납
  glfwTerminate();

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

/**
 * glPixelStorei(GL_UNPACK_ALIGNMENT, 1)
 *
 * GL_UNPACK_ALIGNMENT 란, GPU 가 텍스쳐 버퍼에 저장된 데이터들을 한 줄(row) 단위로 읽을 때,
 * (-> 참고로 여기서 텍스쳐 데이터의 한 줄은 텍스쳐 width 길이만큼과 동일함.)
 * 각 줄의 데이터를 몇 byte 단위로 정렬되도록 할 것인지 지정하는 OpenGL 상태값이라고 보면 됨.
 *
 * 이것의 기본값은 4인데,
 * 그 이유는 OpenGL 에서 다루는 대부분의 텍스쳐 포맷은 GL_RGBA 이므로,
 * 텍스쳐 버퍼 한 줄의 크기는 4 bytes(= r, g, b, a 각각 1 byte 씩) 의 배수로 맞아 떨어짐.
 *
 * 이렇게 되면, 텍스쳐 버퍼의 각 줄(= width)을 GPU 로 전송할 때,
 * 데이터가 기본적으로 4 bytes 단위로 정렬된 상태라고 가정하고 데이터를 읽음.
 * 그래서 일반적으로는 기본값을 그대로 적용해서 GPU 로 텍스쳐를 전송하면 아무런 문제가 안됨.
 *
 * 그러나, 이 예제에서 FreeType 라이브러리가 렌더링해주는 glyph 텍스쳐 버퍼의 포맷은
 * GL_RED 타입의 grayscale bitmap 이므로, 텍스쳐 버퍼의 각 줄의 크기는 1 byte 의 배수로 떨어짐.
 *
 * 이럴 경우, GL_UNPACK_ALIGNMENT 상태값을 1로 변경해서
 * GPU 에게 전송하려는 grayscale bitmap 데이터의 각 줄이 1 byte 단위로 정렬된 상태임을 알려야 함.
 *
 * 이렇게 하지 않으면 소위 Segmentation Fault 라고 하는
 * 허용되지 않은 에모리 영역을 침범하는 memory violation 에러가 런타임에 발생할 수 있음.
 */
