#version 430 core

// From VAOs
in layout(location = 0) vec3 position;
in layout(location = 1) vec3 normal_in;
in layout(location = 2) vec2 textureCoordinates_in;

out layout(location = 100) vec3 position_out;
out layout(location = 101) vec3 normal_out;
out layout(location = 102) vec2 textureCoordinates_out;

void main() {
    normal_out = normal_in;
    textureCoordinates_out = textureCoordinates_in;
    position_out = position;
}