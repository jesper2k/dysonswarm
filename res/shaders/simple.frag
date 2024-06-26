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

// Uniform/static value taken from updateFrame function
uniform layout(location = 5) int is_instanced;
uniform layout(location = 7) float ambient;
uniform layout(location = 8) vec3 ball_pos;
uniform layout(location = 9) vec3 fresnelColor;
//uniform layout(location = 10) int calculateShadows;
uniform layout(location = 11) int is_fresnel;
uniform layout(location = 12) vec3 cameraPosition;
uniform layout(location = 13) float time;
uniform layout(location = 14) int is_dyson;
uniform layout(location = 15) float star_size;
uniform layout(location = 16) int is_animation;

uniform layout(location = 25) float debug_value_1;
uniform layout(location = 26) float debug_value_2;


float ball_radius = 0.1;

// Texture
layout(binding = 1) uniform sampler2D tex;
layout(binding = 2) uniform sampler2D screen_tex;


// Helper functions
float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float dither(vec2 uv) { return (rand(uv)*2.0-1.0) / 256.0; }
vec3 reject(vec3 from, vec3 onto) { return from - onto*dot(from, onto)/dot(onto, onto); }

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
float dyson_transparency;

float fresnel() {
    surface_to_camera = normalize(cameraPosition/star_size - position);
    return clamp(1.0 - abs(1.0*dot(-surface_to_camera, normalize(normal))), 0, 1);
}

vec4 calculateFragmentColor(Light light) {
    
    // Distance from fragment/pixel position to light
    d = length(light_pos - position);
    attenuation = clamp(1/(10*pow(d, 2)), 0, 100); 
    

    light_pos = light.position;
    surface_to_light = normalize(light_pos - position);
    diffuse_intensity = light.intensity * attenuation * dot(surface_to_light, normal);
    
    diffuse_color = light.color;
    if (is_dyson == 1) {
        // Dyson material is more rough than swarm mirrors. This also creates a glow around the star
        shininess = 1.0;
        if (diffuse_intensity < 0) {
            diffuse_color += vec3(2.0, -1.0, -1.0);
        } else {
            diffuse_color += vec3(2.0, 0.2, 0.0);
        }
        diffuse_intensity = abs(diffuse_intensity);
    } else {
        shininess = 10.0;
        // Only the inside of the dyson sphere gets bright, but the swarm mirrors are reflective
        diffuse_intensity = abs(diffuse_intensity);
    }
    
    surface_to_camera = normalize(cameraPosition - position);
    reflected_light = reflect(-surface_to_light, normal);
    specular_intensity = light.intensity * attenuation * clamp(pow(clamp(abs(dot(surface_to_camera, reflected_light)), 0, 1), shininess), 0, 1); // Sorry
    specular_color = vec3(1,1,1);
              
    // Phong with dither, attenuation, and shadows
    return vec4((diffuse_intensity * diffuse_color // Diffuse
              + 1.0 * specular_intensity * specular_color) // Specular
              * vec3(1, 1, 1), 1.0);
    
}


vec4 postProcess(vec4 color_in) {

    //vec4 color_out = vec4(color_in.x, color_in.y, color_in.z, color_in.w);

    // Failed experiment with chromatic abberation
    /** /
    float red_offset   =  0.005;
    float green_offset =  0.003;
    float blue_offset  =  0.000;

    float transparency = color_in.w;

    vec2 tex_size  = textureSize(screen_tex, 0).xy;
    vec2 tex_coord = gl_FragCoord.xy / tex_size;
    
    vec2 direction = tex_coord - tex_size/2;
    
    color_out.x  = texture(screen_tex, tex_coord + (direction * vec2(red_offset  ))).x;
    color_out.y  = texture(screen_tex, tex_coord + (direction * vec2(green_offset))).y;
    color_out.zw = texture(screen_tex, tex_coord + (direction * vec2(blue_offset ))).xw;
    /**/
    
    return color_in;
}

void main()
{
    /** /
    color = vec4(1, 0.5, 1, 1);
    return;
    /**/

    if (is_fresnel == 1) {
        float f = fresnel();
        color = vec4(5.0 * f * fresnelColor, f);
        //color = vec4(f * vec3(1.0, 1.0, 1.0), 1.0); // Black white debug fresnel
        
        color = postProcess(color);
        return;
    }
        
    if (is_textured == 1 || is_dyson == 1) {

        color = texture(tex, textureCoordinates);
        if (is_dyson == 0) {

            // Optional preprocessing of textures for smooth "black->transparency" gradient
            /** /
            if (color.w < 0.99) {
                color.w = (color.x + color.y + color.z)/3;
            }
            /**/
            color = postProcess(color);
            return;
        }
        dyson_transparency = color.w;
        color = texture(tex, textureCoordinates*60);

        // Animation if i want to add that
        /** /
        if (is_animation == 1) {
            color = vec4(0, 1, 0, 1);
            color = texture(tex, vec2(
                mod(textureCoordinates.x + 0.0 * time, 1),
                mod(textureCoordinates.y + 0.5 * time, 1)));
        } else {
        }
        /**/
        
    }

    dither_intensity = dither(textureCoordinates);
    
    vec4 totalColor = vec4((dither_intensity + ambient) * vec3(0, 0, 1), 1);

    for (int i = 0; i < 4; i++) {
        totalColor += calculateFragmentColor(lights[i]);
    }

    
    if (is_dyson == 1) {
        //totalColor += vec4(0.0, 0.0, 0.0, 1.0);
        color = vec4(0.5*totalColor.xyz + color.xyz + vec3(1, -0.05, 0.05), dyson_transparency);
        color = postProcess(color);
        return;
    }
    
    color = postProcess(totalColor);
    //color = totalColor; 
}
    