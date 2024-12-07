#version 330 core

in vec2 TexCoords;

// 색상 출력변수 선언
out vec4 color;

// 각 glyph 가 렌더링된 grayscale bitmap 텍스쳐
uniform sampler2D text;

// 각 glyph 를 렌더링할 텍스트 색상 변수 선언
uniform vec3 textColor;

void main() {
  // grayscale bitmap 텍스쳐로부터 2D Quad 에 적용할 glyph 의 alpha 값 샘플링
  vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);

  // 입력된 텍스트 색상값과 곱하여 최종 glyph 색상 변수 출력
  color = vec4(textColor, 1.0) * sampled;
}
