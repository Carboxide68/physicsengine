@compute
#version 460


layout(local_size_x = 2, local_size_y = 1, local_size_z = 1) in;

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
    float natural_length;

};

layout(std430, binding=0) readonly restrict buffer Nodes {

    Node nodes[];

} in_nodes;

layout(std430, binding=1) readonly restrict buffer Connections {

    Connection c[];

} cons_in;


layout(std430, binding=2) restrict buffer Energy {

    float energies[3];

} enrg_out;

uniform float K;
uniform vec3 gravity;

void calculateVelocityEnergy() {
    const uint invid = gl_LocalInvocationIndex;

    const uint len = in_nodes.nodes.length();
    float velocity_energy = 0;
    float height_energy = 0;

    for (uint i = 0; i < len; i++) {
        if (in_nodes.nodes[i].locked == 1) continue;
        const vec3 v = in_nodes.nodes[i].velocity.xyz;
        const vec3 y = in_nodes.nodes[i].position.xyz;
        velocity_energy += dot(v, v) * in_nodes.nodes[i].mass/2.0;
        height_energy += dot(y, -gravity) * in_nodes.nodes[i].mass;
    }

    enrg_out.energies[0] = velocity_energy;
    enrg_out.energies[2] = height_energy;
}

void calculateConnectionEnergy() {
    const uint invid = gl_LocalInvocationIndex;
    
    const uint len = cons_in.c.length();
    float energy = 0;

    for (uint i = 0; i < len; i++) {
        const uint c1 = cons_in.c[i].first;
        const uint c2 = cons_in.c[i].second;
        const Node n1 = in_nodes.nodes[c1];
        const Node n2 = in_nodes.nodes[c2];
        const vec3 diff = n1.position.xyz - n2.position.xyz;
        const float lengthdiff = length(diff) - cons_in.c[i].natural_length;

        energy += K * lengthdiff*lengthdiff/2.0;
    }
    enrg_out.energies[1] = energy;
}

void main() {

    const uint invid = gl_LocalInvocationIndex;
    if (invid == 0) calculateVelocityEnergy();
    else if (invid == 1) calculateConnectionEnergy();
}

