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

// 주어진 std::string 문자열을 주어진 위치, 크기, 색상으로 렌더링하는 콜백함수
void RenderText(Shader &shader, std::string text, float x, float y, float scale, glm::vec3 color);

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

// 각 glyph 의 grayscale bitmap 을 적용할 2D Quad 의 VAO, VBO 객체 ID 변수 전역 선언 -> RenderText() 콜백함수 내에서 참조해야 하기 때문.
unsigned int VAO, VBO;

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

  /** 2D Quad 의 VAO, VBO 객체 생성 및 설정 */
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  // 2D Quad 는 각 glyph metrices 에 따라 매 프레임마다 정점 데이터가 자주 변경되므로, GL_DYNAMIC_DRAW 모드로 정점 데이터 버퍼의 메모리를 예약함.
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  /** rendering loop */
  while (!glfwWindowShouldClose(window))
  {
    processInput(window);

    // 버퍼 초기화
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 주어진 std::string 컨테이너 문자열을 2D Quad 에 렌더링하는 함수 호출
    RenderText(shader, "This is sample text", 25.0f, 25.0f, 1.0f, glm::vec3(0.5f, 0.8f, 0.2f));
    RenderText(shader, "(C) LearnOpenGL.com", 540.0f, 570.0f, 0.5f, glm::vec3(0.3f, 0.7f, 0.9f));

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

// 주어진 std::string 문자열을 주어진 위치, 크기, 색상으로 렌더링하는 콜백함수
void RenderText(Shader &shader, std::string text, float x, float y, float scale, glm::vec3 color)
{
  // 주어진 Shader 객체 바인딩 및 색상값 전송
  shader.use();
  shader.setVec3("textColor", color);

  // grayscale bitmap 텍스쳐(glyph 텍스쳐)를 바인딩할 texture unit 활성화 및 전송
  glActiveTexture(0);
  // 예제코드 원본에서는 0번 texture unit(기본값) 이다보니 쉐이더 전송 코드는 생략한 것으로 보임. (but, 가독성 및 확장성을 위해 명시적으로 초기화 권장)
  shader.setInt("text", 0);

  // glyph 텍스쳐를 적용할 2D Quad 정점 데이터 VAO 객체 바인딩
  glBindVertexArray(VAO);

  /** 주어진 문자열 컨테이너 std::string 을 순회하며 각 문자에 대응되는 glyph 를 2D Quad 에 렌더링  */
  // std::string 컨테이너를 순회하는 '읽기 전용' 이터레이터 선언
  std::string::const_iterator c;
  /**
   * 읽기 전용 이터레이터 c 에는 연속 메모리 블록으로 저장되는 std::string 컨테이너 내부의
   * 각 char 타입의 문자들이 저장된 메모리 블록 주소값이 담겨 있음.
   *
   * 따라서, *c (= de-referencing)을 통해 해당 메모리에 저장된 실제 char 타입 문자에 접근 가능.
   *
   * 이때, for 문 내에서 c++ 연산자를 사용하면 현재 가리키고 있는 메모리 주소로부터
   * 포인터가 가리키는 데이터 타입(= char)의 크기(= 1 byte)만큼 더해서 다음 메모리 블록에 저장된
   * char 타입 문자를 가리키는 주소값을 얻을 수 있음.
   *
   * -> 즉, 연속 메모리 블록 형태로 저장된 std::string 컨테이너의 각 char 타입 값을
   * 하나씩 순차적으로 접근할 수 있도록 for 문을 구성한 것.
   */
  for (c = text.begin(); c != text.end(); c++)
  {
    // 현재 순회 중인 char 타입 문자에 대응되는 glyph metrices 를 가져옴
    Character ch = Characters[*c];

    // 현재 문자를 렌더링할 glyph 의 위치(= 2D Quad 의 좌하단 정점의 좌표값) 계산
    /**
     * 참고로, 로컬 변수 x, y 에는 현재 glyph 원점(origin)이 저장되어 있음.
     * (LearnOpenGL Glyph Metrics 이미지 참고)
     *
     * 여기에 Bearing 값을 더해 glyph 를 원점에서 얼만큼 떨어트릴 지 결정함.
     * 이때, g, j, p, j 처럼 glyph 일부가 baseline(즉, glyph 원점이 포함된 수평선) 하단에 내려오는 글꼴의 경우,
     * Bearing.y 값이 Size.y 값보다 작게 계산되고,
     *
     * X, Z, Y 처럼 glyph 가 정확히 baseline 위에 안착하는 글꼴의 경우 Bearing.y == Size.y 로 계산되어
     * 글꼴에 따라 glyph 위치값(= 2D Quad 의 좌하단 정점의 좌표)을 baseline 밑으로 내리도록 계산함.
     */
    float xpos = x + ch.Bearing.x * scale;
    float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

    // 현재 문자를 렌더링할 glyph 의 크기(= 2D Quad 의 width, height) 계산
    float w = ch.Size.x * scale;
    float h = ch.Size.y * scale;

    // glyph 의 위치(= 2D Quad 좌하단 정점)와 크기(= 2D Quad 의 width, height)를 가지고 2D Quad 정점 데이터 계산
    float vertices[6][4] = {
        // position      // uv
        {xpos, ypos + h, 0.0f, 0.0f},
        {xpos, ypos, 0.0f, 1.0f},
        {xpos + w, ypos, 1.0f, 1.0f},
        {xpos, ypos + h, 0.0f, 0.0f},
        {xpos + w, ypos, 1.0f, 1.0f},
        {xpos + w, ypos + h, 1.0f, 0.0f},
    };

    // 2D Quad 에 적용할 grayscale bitmap 텍스쳐 버퍼 바인딩
    glBindTexture(GL_TEXTURE_2D, ch.TextureID);

    // 재계산된 2D Quad 정점 데이터를 VBO 객체에 덮어쓰기
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // 2D Quad draw call
    glDrawArrays(GL_TRIANGLES, 0, 6);

    /**
     * 현재 glyph 원점에서 Advance 만큼 떨어진 다음 glyph 원점의 x 좌표값 계산
     *
     * 이 때, FreeType 라이브러리의 Advance 값은 1/64 px 단위로 계산되기 때문에,
     * 이것을 1px 단위로 변환해서 사용해야 함.
     *
     * 이를 위해 1/2^6(= 1/64)제곱값을 구해서 Advance 에 곱할 수도 있으나,
     * >> 6, 즉, right bit shift 연산을 6번 수행하면 1/2 를 6제곱하는 것과 동일함.
     *
     * 심지어, bit shift 연산이 거듭제곱보다 더 빠르기 때문에, 성능 최적화에 유리함.
     */
    x += (ch.Advance >> 6) * scale;
  }

  // std::string 컨테이너에 저장된 모든 문자열 렌더링 완료 후, 텍스쳐 및 VAO 객체 바인딩 해제
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
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
