#version 330 core

in vec4 frag_pos;
in vec3 n;
in vec2 tex;

//uniform vec3 lightPosition;
uniform sampler2D sampler;

uniform vec3 light_pos; 
uniform vec3 view_pos; 
uniform vec3 light_col;

uniform int light;

out vec4 color;

void main() {
	vec4 object_col = texture(sampler, tex);
	vec4 result;
	vec3 pos = vec3(frag_pos);

	if (light > 0) {
		// ambient
		float ambient_strength = 0.1;
		vec3 ambient = ambient_strength * light_col;
  	
		// diffuse 
		vec3 norm = normalize(n);
		vec3 light_vec = normalize(light_pos - pos);
		float diff = max(dot(norm, light_vec), 0.0);
		vec3 diffuse = diff * light_col;
    
		// specular
		float specular_strength = 0.5;
		vec3 view_dir = normalize(view_pos - pos);
		vec3 reflect_dir = reflect(-light_vec, norm);  
		float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
		vec3 specular = specular_strength * spec * light_col;  
        
		result = vec4(ambient + diffuse + specular, 0.0) * object_col;
	} else {
		result = object_col;
	}
	
    color = result;
}
