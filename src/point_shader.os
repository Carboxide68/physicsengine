@vertex
#version 460

layout(location=0) in vec3 in_pos;
layout(location=1) in vec3 in_normal;

uniform mat4 u_assembled_matrix;
uniform float u_node_size;

struct Node {

    vec4 position;
    vec4 velocity;
    float mass;

    int connections[30];
    uint locked;

};

layout(std430, binding=0) readonly restrict buffer Nodes {

    Node nodes[];

} in_nodes;


out uint Locked;

void main() {

    vec4 position = u_assembled_matrix * vec4(-in_nodes.nodes[gl_VertexID].position.xyz, 1.0);

    gl_Position = position; 
    gl_PointSize = u_node_size/position.z * 150;
    Locked = in_nodes.nodes[gl_VertexID].locked;
}

@fragment
#version 460

in flat uint Locked;

out vec4 Color;

void main() {
    vec3 result;
    if (Locked != 0)
        result = vec3(0.5, 0.5, 0.5);
    else
        result = vec3(0.5, 0.0, 0.0);

    Color = vec4(result, 1);
}

@end
