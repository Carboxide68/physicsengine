@vertex
#version 460 core

uniform mat4 u_assembled_matrix;

struct Node {

    vec4 position;
    vec4 velocity;
    float mass;

    int connections[30];
    uint locked;

};

struct Connection {

    uint first;
    uint second;
    float normal_length;

};

layout(std430, binding=0) readonly restrict buffer Nodes {

    Node nodes[];

} in_nodes;

layout(std430, binding=1) readonly restrict buffer Connections {

    Connection c[];

} in_cons;

out vec4 in_color;

#define EPSILON 0.001

void main() {

    uint connection = int(gl_VertexID/2);
    const Node node1 = in_nodes.nodes[in_cons.c[connection].first];
    const Node node2 = in_nodes.nodes[in_cons.c[connection].second];
    const float natural_length = in_cons.c[connection].normal_length;

    if (natural_length > distance(node1.position, node2.position) + EPSILON) in_color = vec4(1, 0, 0, 1);
    else in_color = vec4(0, 1, 0, 1);
    
    vec3 in_pos;
    if (gl_VertexID % 2 == 0) in_pos = -node1.position.xyz;
    else in_pos = -node2.position.xyz;
    gl_Position = u_assembled_matrix * vec4(in_pos, 1.0);

}

@fragment
#version 460 core

uniform vec4 u_color;

in vec4 in_color;
out vec4 Color;

void main() {

    Color = in_color;

}

@end
