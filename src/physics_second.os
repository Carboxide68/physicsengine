@compute
#version 460

layout(local_size_x = 1536, local_size_y = 1, local_size_z = 1) in;

struct Node {

    vec4 position;
    vec4 velocity;
    float mass;

    uint locked;

};

struct Connection {

    uint first;
    uint second;
    float natural_length;

};

struct Accumulation {

    vec4 velocity;
    vec4 force;

};

layout(std430, binding=0) restrict buffer Nodes {

    Node nodes[];

} in_nodes;

layout(std430, binding=4) restrict readonly buffer BaseNodes {

    Node nodes[];

} in_base;

layout(std430, binding=5) restrict buffer Combined {

    Accumulation acc[];

} in_acc;

layout(std430, binding=6) restrict buffer Forces {

    coherent vec4 force[];

} in_forces;

layout(std430, binding=1) readonly restrict buffer Connections {

    Connection c[];

} in_cons;

uniform float drag;
uniform vec3 gravity;
uniform float K;
uniform float TS;
uniform uint node_count;

double start;
double end;

vec4 calculate_forces() {

    uint s = in_nodes.nodes.length() * start;
    uint e = in_nodes.nodes.length() * end;

    for (uint i = s; i < e; i++) {
        if (in_nodes.nodes[i].locked != 0) continue;
        const float mass = in_nodes.nodes[i].mass;
        const vec4 velocity = in_nodes.nodes[i].velocity;
        forces.forces[i] = mass * vec4(gravity,0) + -velocity * drag;
    }

    s = in_cons.c.length() * start;
    e = in_cons.c.length() * end;
    for (uint i = s; i < e; i++) {
    
        Connection c = in_cons.c[i];
        const vec4 diff = in_nodes.nodes[c.first].position - in_nodes.nodes[c.second].position;
        const vec4 adjusted_diff = diff - normalize(diff) * c.natural_length;
        const vec4 force = K * adjusted_diff;
        in_forces.force[c.first] += force;
        in_forces.force[c.second] += -force;
    }
}

void euler_integration(float e_TS) {
    
    uint s = in_nodes.nodes.length() * start;
    uint e = in_nodes.nodes.length() * end;

    for (uint i = s; i < e; i++) {
    
        const float mass = in_base.nodes[i].mass;
        in_nodes.nodes[i].position = in_base.nodes[i].position + in_nodes.nodes[i].velocity * e_TS;
        in_nodes.nodes[i].velocity = in_base.nodes[i].velocity + forces.forces[i]/mass * e_TS;

    }
}

void main() {

    //K1
    start = double(gl_LocalInvocationIndex)/double(gl_WorkGroupSize.x);
    end = double(gl_LocalInvocationIndex+1)/double(gl_WorkGroupSize.x);

    calculate_forces();

    barrier();
    uint s = in_nodes.nodes.length() * start;
    uint e = in_nodes.nodes.length() * end;
    for (uint i = s; i < e; i++) {
        in_acc.acc[i] = {in_base.nodes[i].velocity, in_forces.forces[i]};
    }

    //K2
    euler_integration(TS/2.0);

    barrier();
    memoryBarrier();

    calculate_forces();

    barrier();
    for (uint i = s; i < e; i++) {
        in_acc.acc[i].velocity += in_nodes.nodes[i].velocity * 2;
        in_acc.acc[i].force += forces.forces[i] * 2;
    }

    //K3
    euler_integration(TS/2.0);

    barrier();
    memoryBarrier();

    calculate_forces();

    barrier();
    for (uint i = s; i < e; i++) {
        in_acc.acc[i].velocity += in_nodes.nodes[i].velocity * 2;
        in_acc.acc[i].force += forces.forces[i] * 2;
    }
    //K4
    euler_integration(TS);

    barrier();
    memoryBarrier();

    calculate_forces();

    barrier();
    for (uint i = s; i < e; i++) {
        in_acc.acc[i].velocity += in_nodes.nodes[i].velocity;
        in_acc.acc[i].force += forces.forces[i];
    }

    //yn+1
    for (uint i = s; i < e; i++) {
        const float mult = (in_nodes.nodes[i].locked == 0) ? 1.0 : 0.0;
        const float mass = in_nodes.nodes[i].mass;
        in_nodes.nodes[i].position = in_base.nodes[i].position + in_acc.acc[i].velocity * TS/6.0 * mult;
        in_nodes.nodes[i].velocity = in_base.nodes[i].velocity + in_acc.acc[i].force/mass * TS/6.0 * mult;
    }
}

@end
