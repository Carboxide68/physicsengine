@compute
#version 460

layout(local_size_x = 1536, local_size_y = 1, local_size_z = 1) in;

struct Node {

    vec4 position;
    vec4 velocity;
    float mass;

    uint locked;
    uint padding[2];

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

layout(std430, binding=4) restrict buffer BaseNodes {

    Node nodes[];

} in_base;

layout(std430, binding=5) restrict buffer Combined {

    Accumulation acc[];

} in_acc;

layout(std430, binding=6) coherent restrict buffer Forces {

    vec4 force[];

} in_forces;

layout(std430, binding=1) readonly restrict buffer Connections {

    Connection c[];

} in_cons;

uniform float drag;
uniform vec3 gravity;
uniform float K;
uniform float TS;

const int INT_MIN = -2147483648;
const int INT_MAX = 2147483647;
const float EPSILON2 = 0.000000001;

double start;
double end;

int QuantizeFloat(float value, float scope) {

    return int(value/scope * double(INT_MAX));

}

float DeQuantize(int value, float scope) {

    return float(double(value)/double(INT_MAX) * scope);

}

void addForce(uint i, vec4 force, float scope) {

    in_forces.force[i] += force;

}

vec4 readForce(uint i, float scope) {

    return in_forces.force[i];

}
/*
void addForce(uint i, vec4 force, float scope) {

    atomicAdd(in_forces.force[i][0], QuantizeFloat(force[0], scope));
    atomicAdd(in_forces.force[i][1], QuantizeFloat(force[1], scope));
    atomicAdd(in_forces.force[i][2], QuantizeFloat(force[2], scope));
    atomicAdd(in_forces.force[i][3], QuantizeFloat(force[3], scope));

}

vec4 readForce(uint i, float scope) {

    vec4 f;
    ivec4 force = in_forces.force[i];
    f[0] = DeQuantize(force[0], scope);
    f[1] = DeQuantize(force[1], scope);
    f[2] = DeQuantize(force[2], scope);
    f[3] = DeQuantize(force[3], scope);
    return f;

}
*/
void calculate_forces() {

    uint s = uint(double(in_nodes.nodes.length()) * start);
    uint e = uint(double(in_nodes.nodes.length()) * end);

    for (uint i = s; i < e; i++) {
        const float mass = in_nodes.nodes[i].mass;
        const vec4 velocity = in_nodes.nodes[i].velocity;
        const vec4 force = mass * vec4(gravity, 0) + -velocity * drag;
        in_forces.force[i] = force;
    }
    barrier();
    memoryBarrier();

    s = uint(double(in_cons.c.length()) * start);
    e = uint(double(in_cons.c.length()) * end);
    for (uint i = s; i < e; i++) {
        Connection c = in_cons.c[i];
        const vec4 diff = in_nodes.nodes[c.first].position - in_nodes.nodes[c.second].position;
        const vec4 adjusted_diff = diff - normalize(diff) * c.natural_length;
        const vec4 force = K * adjusted_diff;
        in_forces.force[c.first] += -force;
        in_forces.force[c.second] += force;
    }
}

void euler_integration(float e_TS) {
    
    uint s = uint(double(in_nodes.nodes.length()) * start);
    uint e = uint(double(in_nodes.nodes.length()) * end);

    for (uint i = s; i < e; i++) {
        if (in_nodes.nodes[i].locked != 0) continue;
        const float mass = in_nodes.nodes[i].mass;
        const vec4 acceleration = in_forces.force[i]/mass;
        const vec4 velocity = in_nodes.nodes[i].velocity;
        in_nodes.nodes[i].position = in_base.nodes[i].position + velocity * e_TS;
        in_nodes.nodes[i].velocity = in_base.nodes[i].velocity + acceleration * e_TS;
    }
}

void main() {

    start = double(gl_LocalInvocationIndex)/double(gl_WorkGroupSize.x);
    end = double(gl_LocalInvocationIndex+1)/double(gl_WorkGroupSize.x);
    uint s = uint(double(in_nodes.nodes.length()) * start);
    uint e = uint(double(in_nodes.nodes.length()) * end);

    for (uint i = s; i < e; i++) {
        in_base.nodes[i].position = in_nodes.nodes[i].position;
        in_base.nodes[i].velocity = in_nodes.nodes[i].velocity;
    }

    //K1
    calculate_forces();

    barrier();
    for (uint i = s; i < e; i++) {
        const float mass = in_nodes.nodes[i].mass;
        const vec4 force = in_forces.force[i];
        in_acc.acc[i].velocity = in_base.nodes[i].velocity;
        in_acc.acc[i].force = force;
    }

    //K2
    euler_integration(TS/2.0);

    barrier();
    memoryBarrier();

    calculate_forces();

    barrier();
    for (uint i = s; i < e; i++) {
        const float mass = in_nodes.nodes[i].mass;
        const vec4 force = in_forces.force[i];
        in_acc.acc[i].velocity += in_nodes.nodes[i].velocity * 2.0;
        in_acc.acc[i].force += force * 2.0;
    }

    //K3
    euler_integration(TS/2.0);

    barrier();
    memoryBarrier();

    calculate_forces();

    barrier();
    for (uint i = s; i < e; i++) {
        const float mass = in_nodes.nodes[i].mass;
        const vec4 force = in_forces.force[i];
        in_acc.acc[i].velocity += in_nodes.nodes[i].velocity * 2.0;
        in_acc.acc[i].force += force * 2.0;
    }
    //K4
    euler_integration(TS);

    barrier();
    memoryBarrier();

    calculate_forces();

    barrier();
    //yn+1
    for (uint i = s; i < e; i++) {
        const float mass = in_nodes.nodes[i].mass;
        const vec4 force = in_forces.force[i] + in_acc.acc[i].force;
        const vec4 vel = in_nodes.nodes[i].velocity + in_acc.acc[i].velocity;
        const float mult = (in_nodes.nodes[i].locked == 0) ? 1.0 : 0.0;
        in_nodes.nodes[i].position = in_base.nodes[i].position + vel * TS/6.0 * mult;
        in_nodes.nodes[i].velocity = in_base.nodes[i].velocity + force/mass * TS/6.0 * mult;
    }
}

@end
