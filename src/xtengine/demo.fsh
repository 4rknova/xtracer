STRINGIFY (

uniform vec2      iResolution;
uniform sampler2D iChannel0;

void main()
{
    gl_FragColor = vec4(texture2D(iChannel0, gl_FragCoord/iResolution).xyz, 1);
}

);
