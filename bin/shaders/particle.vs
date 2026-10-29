
#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in float vertexType;

uniform mat4 mvp;

out float isDark;

void main()
{
    gl_Position = mvp * vec4(vertexPosition, 1.0);
    gl_PointSize = 2.0;
    isDark = vertexType;
}
