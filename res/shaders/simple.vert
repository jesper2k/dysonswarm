#version 430 core


// From VAOs
in layout(location = 0) vec3 position;
in layout(location = 1) vec3 normal_in;
in layout(location = 2) vec2 textureCoordinates_in;

uniform layout(location = 3) mat4 MVP;
uniform layout(location = 4) mat4 model_matrix;
uniform layout(location = 5) int is_instanced;
uniform layout(location = 6) mat3 normal_matrix;
uniform vec3 instanceOffset[1000];



// Outputs match simple.frag input
out layout(location = 0) vec3 normal_out;
out layout(location = 1) vec2 textureCoordinates_out;
out layout(location = 2) vec3 model_out;


void main()
{
    model_out = normalize((model_matrix * vec4(position, 1.0)).xyz);
    normal_out = normalize(normal_matrix * normal_in);

    textureCoordinates_out = textureCoordinates_in;
    if (is_instanced == 1) {
        // How do i rotate this a little bit? (based on x-offset + some randomness)
        gl_Position = MVP * vec4(position + instanceOffset[gl_InstanceID], 1.0f);
    } else {
        gl_Position = MVP * vec4(position, 1.0f);
    }
}
