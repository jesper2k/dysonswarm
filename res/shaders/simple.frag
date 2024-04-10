#version 430 core


// Task 3
struct Light {
    vec3 position;
    vec3 color;
    float intensity;
};

// Light sources
uniform Light lights[4];


// Inputs match simple.vert outputs
in layout(location = 0) vec3 normal;
in layout(location = 1) vec2 textureCoordinates;
in layout(location = 2) vec3 position;

out layout(location = 0) vec4 color;


uniform layout(location = 64) int is_textured;
uniform layout(location = 11) int is_fresnel;

// Uniform/static value taken from updateFrame function
uniform layout(location = 5) int is_instanced;
uniform layout(location = 7) float ambient;
uniform layout(location = 8) vec3 ball_pos;
//uniform layout(location = 10) int calculateShadows;
float ball_radius = 0.1;

// Texture
layout(binding = 1) uniform sampler2D tex;


// Helper functions
float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float dither(vec2 uv) { return (rand(uv)*2.0-1.0) / 256.0; }
vec3 reject(vec3 from, vec3 onto) { return from - onto*dot(from, onto)/dot(onto, onto); }

/*
float shadow;
vec3 shadowDebug;

float isShadow(vec3 rejection, vec3 light_pos, vec3 position) {

    vec3 shadowDebug = vec3(0, 0, 0);
    float shadow = 0;

    // If fragment to light passes through ball (direct shadow)
    if (length(rejection) < ball_radius) { 
        //shadowDebug += vec3(1, 0, 0);

        // If fragment is on the correct side of the light
        if (dot(light_pos - position, light_pos - ball_pos) > 0) {
            //shadowDebug += vec3(0, 0, 1);
            shadow = clamp(0.9+ball_radius-10*length(rejection), 0, 1); // Don't mind the magic numbers :^)
        }
    }

    // If fragment is behind ball (no shadow on the ball itself)
    if (length(light_pos - position) < length(light_pos - ball_pos)) {
        //shadowDebug += vec3(0, 1, 0);
        shadow = 0;
    }

    return shadow;
}
*/

vec3 surface_to_light;
vec3 surface_to_camera;
vec3 diffuse_color;
vec3 reflected_light;
vec3 specular_color;
vec3 rejection;
vec3 light_pos;

float d;
float attenuation;
float diffuse_intensity;
float shininess;
float specular_intensity;
float dither_intensity;

float fresnel() {
    surface_to_light = normalize(light_pos - position);
    surface_to_camera = normalize(vec3(0, 0, 0) - position);
    reflected_light = reflect(-surface_to_light, normal);
    return clamp(0.5 - 0.5* dot(surface_to_camera, reflected_light), 0, 1);
}

vec4 calculateFragmentColor(Light light) {
    light_pos = light.position;
    
    surface_to_light = normalize(light_pos - position);

    // Distance from fragment/pixel position to light
    d = length(light_pos - position);
    attenuation = clamp(1/(10*pow(d, 2)), 0, 100); 

    diffuse_intensity = light.intensity * attenuation * dot(surface_to_light, normal);
    diffuse_color = light.color;
    
    shininess = 18.0;
    surface_to_camera = normalize(vec3(0, 0, 0) - position);
    reflected_light = reflect(-surface_to_light, normal);
    specular_intensity = light.intensity * attenuation * clamp(pow(clamp(dot(surface_to_camera, reflected_light), 0, 1), shininess), 0, 1); // Sorry
    specular_color = vec3(1, 1, 1);

    // Shadow
    /*
    if (calculateShadows == 1) {
        rejection = reject(light_pos - position, normalize(ball_pos - position));
        shadow = isShadow(rejection, light_pos, position);
    } else {
        shadow = 0;
    }
    */

    // Shadow doesn't work correctly :(
    // I'm guessing it's due to a wacky perspective projection that's not done correctly
    

    // Lines for testing
    //color = vec4(0.5 * (rejection) + 0.5 + ambient, 1.0);
    //color = vec4(lights[0].color, 1.0);
    
    // Phong with dither, attenuation, and shadows
    return vec4((diffuse_intensity * diffuse_color // Diffuse
              + 0.5 * specular_intensity * specular_color) // Specular
              * (1/*-shadow*/) * vec3(1, 1, 1), 1);
    
}

void main()
{
    if (is_fresnel == 1) {
        color = vec4(2.0 * fresnel() * vec3(1.0, 0.7, 0.0), 0.5 * fresnel());
        return;
    }
        
    if (is_textured == 1) {
        
        color = texture(tex, textureCoordinates);

        //color = vec4(1.0, 0.0, 0.0, 1.0);
        //color = vec4(textureCoordinates.x, textureCoordinates.y, 0.0, 1.0);
        return;
    }
    


    dither_intensity = dither(textureCoordinates);
    vec4 totalColor = vec4((dither_intensity + ambient) * vec3(1, 1, 1), 1);
    

    for (int i = 0; i < 4; i++) {
        totalColor += calculateFragmentColor(lights[i]);
    }

    color = totalColor; 

}
    