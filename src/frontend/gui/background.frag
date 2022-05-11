STRINGIFY (

uniform vec2  iWindowResolution;
const float iTime = 25.154;

float hash(vec3 p) {
    return fract(sin(dot(p,vec3(127.1,311.7, 321.4)))*43758.5423);
}

float noise(in vec3 p)
{
	p.z += 0.25*(cos(iTime + p.x) + sin(iTime + p.y));
    vec3 i = floor(p);
	vec3 f = fract(p); 
	f *= f * (3.-2.*f);
    vec2 c = vec2(0,1);
    return mix(
		mix(mix(hash(i + c.xxx), hash(i + c.yxx),f.x),
			mix(hash(i + c.xyx), hash(i + c.yyx),f.x),
			f.y),
		mix(mix(hash(i + c.xxy), hash(i + c.yxy),f.x),
			mix(hash(i + c.xyy), hash(i + c.yyy),f.x),
			f.y),
		f.z);
}

float fbm(in vec3 p)
{
	return .5000 * noise(1. * p)
	 	 + .2500 * noise(2. * p)
		 + .1250 * noise(4. * p);
		 + .0625 * noise(8. * p);
}

void main()
{	   
    vec2 uv = vec2(iWindowResolution.y - gl_FragCoord.y, gl_FragCoord.x) * 0.0005;
    vec3  p = 1.5 * vec3(uv.x,uv.y,uv.x*uv.y);
    vec3 col = fbm(p) * mix(vec3(.4,.42,.4), vec3(.4,.4,.55), p);
    col += 0.3 * hash(uv.yyx * (cos(iTime) + sin(iTime))) * .1;
	col *= smoothstep(0.01, 3.5, iTime);
    
    gl_FragColor = vec4(col, 1);
}

);
