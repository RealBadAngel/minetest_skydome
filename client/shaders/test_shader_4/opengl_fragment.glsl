uniform sampler2D glow;
uniform sampler2D color;
uniform vec3 sunpos;
varying vec3 vertex;

void main()
{
    vec3 V = normalize(vertex)*0.995;
    vec3 L = sunpos;

    // Compute the proximity of this fragment to the sun.

    float vl = dot(V, L);
	float sun = smoothstep(0.0, 1.0, (vl-0.970)*10.0);
	sun += smoothstep(0.0, 1.0, (vl-0.9947)*6000.0);
	//sun = clamp(sun,0.0,1.0);

    // Look up the sky color and glow colors.

    vec4 Kc = texture2D(color, vec2((L.z + 1.0) / 2.0, V.z));
    vec4 Kg = texture2D(glow,  vec2((L.z + 1.0) / 2.0, vl))*vl;

    // Combine the color and glow giving the pixel value.

	gl_FragColor = vec4(Kc.rgb + Kg.rgb * Kg.a / 2.0, Kc.a)+vec4(Kg.rgb*sun,0.15);
	//gl_FragColor = vec4(sun);
}
