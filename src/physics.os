@compute
#version 460

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

const uint CONNECTION_COUNT = 30;

struct Node {

    vec4 position;
    vec4 velocity;
    float mass;

    int connections[CONNECTION_COUNT];
    uint locked;

};

struct Connection {

    uint first;
    uint second;
    float natural_length;

};

layout(std430, binding=0) restrict buffer Nodes {

    Node nodes[];

} in_nodes;

layout(std430, binding=1) readonly restrict buffer Connections {

    Connection c[];

} cons_in;

uniform float drag;
uniform vec3 gravity;
uniform float K;
uniform float TS;

Node base_node; 
Node derivative_node;
float mass;
uint globalInvocationIndex;
#define node (in_nodes.nodes[globalInvocationIndex])

vec4 calculate_forces(Node n) {

    vec4 result = vec4(0, 0, 0, 0);
    if (n.locked != 0) return result;
    
    result += -n.velocity * drag;
    result += mass * vec4(gravity, 0);

    uint head = 0;
    while (head < CONNECTION_COUNT) {
        if (node.connections[head] == int(-1)) break;
        const Connection c = cons_in.c[node.connections[head]];
        const vec4 diff = in_nodes.nodes[c.first].position - in_nodes.nodes[c.second].position;
        const vec4 adjusted_diff = diff - normalize(diff) * c.natural_length;
        const vec4 force = K * adjusted_diff;
        if (c.first == globalInvocationIndex) {result += -force;}
        else {result += force;}
        head++;
    }

    return result;

}

void euler_integration(vec4 force, float e_TS) {
    
    derivative_node.position = base_node.position + derivative_node.velocity * e_TS;
    derivative_node.velocity = base_node.velocity + force/mass * e_TS;

}

void main2() {

    base_node = node;
    derivative_node = base_node;
    vec4 force = vec4(0, -9.8, 0, 0);

    //K1
    force = calculate_forces(node);

    //K2
    euler_integration(force, TS);
    node.position = derivative_node.position;
    node.velocity = derivative_node.velocity;
}

void main() {

    globalInvocationIndex = gl_LocalInvocationIndex + gl_WorkGroupID.z * gl_NumWorkGroups.x * gl_NumWorkGroups.y + gl_WorkGroupID.y * gl_NumWorkGroups.x + gl_WorkGroupID.x;

    base_node.position = node.position;
    base_node.velocity = node.velocity;
    derivative_node.position = base_node.position;
    derivative_node.velocity = base_node.velocity;
    mass = node.mass;
    vec4 force;

    vec4 combined_force = vec4(0);
    vec4 combined_velocity = vec4(0);

    //K1
    force = calculate_forces(node);

    combined_velocity += derivative_node.velocity;
    combined_force += force;

    //K2
    euler_integration(force, TS/2.0);

    barrier();
    node.position = derivative_node.position;
    barrier();
    memoryBarrier();
    force = calculate_forces(node);

    combined_force += force * 2.0;
    combined_velocity += derivative_node.velocity * 2.0;

    //K3

    euler_integration(force, TS/2.0);

    barrier();
    node.position = derivative_node.position;
    barrier();
    memoryBarrier();
    force = calculate_forces(node);

    combined_velocity += derivative_node.velocity * 2.0;
    combined_force += force * 2.0;

    //K4

    euler_integration(force, TS);

    barrier();
    node.position = derivative_node.position;
    barrier();
    memoryBarrier();
    force = calculate_forces(node);

    combined_velocity += derivative_node.velocity;
    combined_force += force;

    //yn+1
    node.position = base_node.position + combined_velocity * TS/6.0;
    node.velocity = base_node.velocity + combined_force/mass * TS/6.0;
}

@end
