
#version 330

in float isDark;
uniform int showDarkMatter;

out vec4 finalColor;

void main()
{
    if (showDarkMatter == 0 && isDark > 0.5)
        discard;

    vec2 p = gl_PointCoord * 2.0 - 1.0;
    if (dot(p, p) > 1.0)
        discard;

    if (isDark > 0.5)
        finalColor = vec4(0.35, 0.2, 0.8, 1.0);
    else
        finalColor = vec4(1.0, 1.0, 1.0, 1.0);
}
