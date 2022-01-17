#ifndef USE_GPU_SHADER_CREATE_INFO
uniform mat4 ModelViewProjectionMatrix;

in vec2 pos;
in float size;
in vec4 color;
out vec4 finalColor;
#endif

void main()
{
  gl_Position = ModelViewProjectionMatrix * vec4(pos, 0.0, 1.0);
  gl_PointSize = size;
  finalColor = color;
}
