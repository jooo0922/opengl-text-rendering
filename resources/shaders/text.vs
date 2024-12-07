#version 330 core

// 이 예제에서는 정점 pos, uv 데이터를 단일 attribute 에 담아서 전달받을 것임.
// -> why? glyph 를 렌더링할 2D Quad 의 정점 위치는 vec2 만으로도 충분히 정의 가능하므로, vec4 에 pos, uv 를 한꺼번에 담을 수 있음.
layout(location = 0) in vec4 vertex;

// uv 보간 출력 변수 선언
out vec2 TexCoords;

// orthogonal 투영행렬 변수 선언
uniform mat4 projection;

void main() {
  // text rendering 시 카메라를 사용하지 않으므로 정점 pos 에 투영행렬을 바로 곱해서 변환함.
  // 이때, 정점 pos(= vertex.xy) 는 screen space 기준으로 정의된 좌표값이며, orthogonal 투영행렬은 screen space 좌표값을 그대로 사용 가능하도록 계산된 상태임.
  gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
  TexCoords = vertex.zw;
}
