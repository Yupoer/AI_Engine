#version 330 core
layout (location = 6) in vec3 aPos;
layout (location = 7) in vec3 aColor;
layout (location = 8) in vec2 aTexCoord;
layout (location = 9) in vec3 aNormal;


out vec2 TexCoord;

out vec3 Normal;
out vec3 FragPos;

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projMat;

void main() {
    gl_Position =  projMat * viewMat * modelMat * vec4(aPos.xyz, 1.0);
    
    
	TexCoord = aTexCoord;

    FragPos = (modelMat * vec4(aPos.xyz, 1.0)).xyz;
    Normal = mat3(transpose(inverse(modelMat))) * aNormal;
}