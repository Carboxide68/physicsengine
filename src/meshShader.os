@vertex
#version 460

layout(location=0) in vec3 in_pos;
layout(location=1) in vec3 in_normal;

uniform mat4 u_model_matrix;
uniform mat4 u_assembled_matrix;

struct Node {

    vec4 position;
    vec4 velocity;
    float mass;

    uint locked;

};

layout(std430, binding=0) readonly restrict buffer Nodes {

    Node nodes[];

} in_nodes;


out vec3 Normal;
out vec3 FragPos;
out uint Locked;

void main() {

    vec4 position = in_nodes.nodes[gl_InstanceID].position;

    mat4 model_matrix = u_model_matrix;
    model_matrix[3] = vec4(-position.xyz, 1);

    vec4 new_pos = model_matrix * vec4(in_pos, 1);
    gl_Position = u_assembled_matrix * new_pos; 
	Normal = mat3(transpose(inverse(model_matrix))) * in_normal;
    FragPos = (new_pos).xyz;
    Locked = in_nodes.nodes[gl_InstanceID].locked;
    //vec4 new_pos = u_model_matrix * vec4(in_pos, 1);
}

@fragment
#version 460

struct Material {

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float specE;

};

struct Light {

    vec3 color;
    vec3 position;

};

const Light light = {

    {1.0,1.0,1.0},
    {0.0,0.0,0.0},

};

const Material material = {

    {0.2, 0.0, 0.0},
    {0.5, 0.0, 0.0},
    {1.0, 1.0, 1.0},
    32.0,
};

in vec3 Normal;
in vec3 FragPos;
in flat uint Locked;

out vec4 Color;

uniform vec3 u_player_pos;

void main() {

    
    vec3 norm = normalize(Normal);

    vec3 lightDir = normalize(light.position - FragPos);
    vec3 ambient = material.ambient;
    if (Locked != 0) ambient = vec3(0.1, 0.1, 0.1);
    float diff = dot(norm, lightDir);
    vec3 mat_diff = material.diffuse;
    if (Locked != 0) mat_diff = vec3(0.5, 0.5, 0.5);
    vec3 diffuse = light.color * (abs(diff) * mat_diff);

    // specular
    vec3 viewDir = normalize(u_player_pos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(abs(dot(norm, halfwayDir)), material.specE);
    vec3 specular = light.color * (material.specular * spec);

    vec3 result;
    if (diff < 0)
        result = ambient;
    else
        result = ambient + specular + diffuse;

    Color = vec4(result, 1);
    //Color = vec4(normalize(FragPos), 1);
}

@end
