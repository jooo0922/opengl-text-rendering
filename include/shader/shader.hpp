#ifndef SHADER_HPP
#define SHADER_HPP

/*
  #ifndef ~ #endif 전처리기는
  헤더파일의 헤더가드를 처리하여 중복 include 방지해 줌!
*/

#include <glad/glad.h> // OpenGL 함수를 초기화하기 위한 헤더
#include <string>      // std::string
#include <fstream>     // 파일 입출력을 위한 헤더
#include <sstream>     // 문자열 스트림
#include <iostream>    // 콘솔 입출력을 위한 헤더
#include <glm/glm.hpp> // glm 라이브러리

/*
  Shader 클래스

  쉐이더 파일 파싱, 컴파일, 컴파일 에러 예외처리,
  쉐이더 프로그램 생성 및 관리, uniform 변수에 데이터 전송 등

  쉐이더와 관련된 모든 작업들을 관리하는 클래스!

  즉, 기존 쉐이더 관련 코드들을 별도의 클래스로 추출하는
  리팩토링을 했다고 보면 됨!
*/
class Shader
{
public:
  unsigned int ID; // 생성된 ShaderProgram의 참조 ID

  // Shader 클래스 생성자
  Shader(const GLchar *vertexPath, const GLchar *fragmentPath);

  // Shader 클래스 소멸자
  ~Shader();

  // ShaderProgram 객체 활성화
  void use();

  // 유니폼 변수 관련 유틸리티
  void setBool(const std::string &name, bool value) const;
  void setInt(const std::string &name, int value) const;
  void setFloat(const std::string &name, float value) const;
  void setVec2(const std::string &name, const glm::vec2 &value) const;
  void setVec2(const std::string &name, float x, float y) const;
  void setVec3(const std::string &name, const glm::vec3 &value) const;
  void setVec3(const std::string &name, float x, float y, float z) const;
  void setVec4(const std::string &name, const glm::vec4 &value) const;
  void setVec4(const std::string &name, float x, float y, float z, float w) const;
  void setMat2(const std::string &name, const glm::mat2 &mat) const;
  void setMat3(const std::string &name, const glm::mat3 &mat) const;
  void setMat4(const std::string &name, const glm::mat4 &mat) const;

private:
  // 쉐이더 객체 및 쉐이더 프로그램 객체의 컴파일 및 링킹 에러 대응
  void checkCompileErrors(unsigned int shader, std::string type);
};

#endif // SHADER_HPP
