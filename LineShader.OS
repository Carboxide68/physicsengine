@vertex
#version 450 core

layout(location = 0) in vec3 in_Point;

uniform mat4 uAssembledMatrix;

void main() {
    gl_Position = uAssembledMatrix * vec4(in_Point, 1.0);
}

@fragment
#version 450 core

uniform vec3 uLineColor;

out vec4 FragColor;

void main() {

    FragColor = vec4(uLineColor, 1.0);

}

@END