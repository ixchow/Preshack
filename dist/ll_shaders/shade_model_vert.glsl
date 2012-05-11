void main() {
	vec4 vert = gl_ModelViewProjectionMatrix * gl_Vertex;
	vec3 norm = normalize((gl_ModelViewMatrix * vec4(gl_Normal,0.0)).xyz);

	vec3 color =
		+ abs(gl_Normal.x) * vec3(0.0, 1.0, 0.0)
		+ abs(gl_Normal.y) * vec3(1.0, 0.5, 0.0)
		+ abs(gl_Normal.z) * vec3(0.0, 0.5, 1.0);

	float lit = max(0.0, dot(norm, normalize(vec3(0.5,1.0,-1.5))));
	float lit2 = max(0.0, dot(norm, normalize(vec3(-1.5,0.3,-0.5))));

	gl_FrontColor = gl_BackColor = vec4(color,0.7) *
		mix(
			vec4(0.1, 0.1, 0.2, 1.0),
			vec4(1.0, 0.9, 0.9, 1.0),
			lit+lit2
		);
	gl_Position = vert;
}
