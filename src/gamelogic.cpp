
/*
TODO:
Perspective projection wackery
Improve ugly code for manually setting each light position, color, and intensity

Resources:
- Instanced rendering for lots of mirrors: https://learnopengl.com/Advanced-OpenGL/Instancing




*/

#include <chrono>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <SFML/Audio/SoundBuffer.hpp>
#include <utilities/shader.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <utilities/timeutils.h>
#include <utilities/mesh.h>
#include <utilities/shapes.h>
#include <utilities/glutils.h>
#include <SFML/Audio/Sound.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fmt/format.h>
#include "gamelogic.h"
#include "sceneGraph.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "utilities/imageLoader.hpp"
#include "utilities/glfont.h"

enum KeyFrameAction {
    BOTTOM, TOP
};

#include <timestamps.h>
#include <cstdlib>
#include <ctime>
#include <random>

int calculateShadows = 0;

float pi = 3.1415926535897926462;
float tau = 2 * pi;

bool mute = true;

// Scene setup
int scene = 0;
int numSecneProperties = 6; // Manually updated
SceneConfig sceneConfigs[3] = {
    {
        /* Mirror objects                */ 400,
        /* Instances per object          */ 1000,
        /* Mirror size                   */ 0.002,
        /* Star texture filename         */ "sun_col.png",
        /* Mirror model filename         */ "hex.obj",
        /* Fresnel color                 */ glm::vec3(0.9, 0.5, 0.1),
    },
    {
        300,
        1000,
        0.002,
        "neutronstar.png",
        "hex.obj",
        glm::vec3(0.6, 0.2, 1.0),
    },
    {
        500,
        1000,
        0.005,
        "sun_col.png",
        "hex.obj",
        glm::vec3(1.0, 0.4, 0.0),
    },
};



float starSize = 10.0; // Config todo
const float timeSpeedup = 0.25; // 0.03

float mirrorScale = sceneConfigs[scene].mirrorSize;
int numMirrors = sceneConfigs[scene].numMirrors;
const int maxNumMirrors = 500; // Config todo
const float baseRadius = 50; // Config todo
const float randRadius = 60;// Config todo
const float maxInclination = 0.15; // of radius
glm::vec3 fresnelColor = sceneConfigs[scene].fresnelColor;

glm::vec3 cameraPosition = glm::vec3(0, 20, 100);

Mirror* mirrors[maxNumMirrors];
const int numMagNodes = 4;
SceneNode* magNodes[numMagNodes];

SceneNode dysonLayer1;
SceneNode dysonLayer2;
SceneNode dysonLayer3;

int instances = sceneConfigs[scene].instances;
const float instanceRandomSize = 500.0f;// Config todo
glm::vec3 instanceOffset[1000];

double padPositionX = 0;
double padPositionZ = 0;

unsigned int currentKeyFrame = 0;
unsigned int previousKeyFrame = 0;

std::default_random_engine generator;
std::normal_distribution<double> normal(0.0, 1.0);

float random() {
    return (float)(rand() % 10000) / 10000.0;
}

SceneNode* rootNode;
SceneNode* hiddenNode;
SceneNode* boxNode;
SceneNode* ballNode;
SceneNode* starNode;
SceneNode* glowNode; // Fresnel for star
SceneNode* glowNode2; // Fresnel for star
SceneNode* glowNode3; // Fresnel for star
SceneNode* arcNode;
SceneNode* magNode;
SceneNode* textNode;
SceneNode* dysonNode1;
SceneNode* dysonNode2;
SceneNode* dysonNode3;

PointLight* lightNode0;
PointLight* lightNode1;
PointLight* lightNode2;
PointLight* lightNode3;
PointLight* lights[4];

double ballRadius = 3.0f;


// These are heap allocated, because they should not be initialised at the start of the program
sf::SoundBuffer* buffer1; // DSP main theme
sf::SoundBuffer* buffer2; // Mountain King
Gloom::Shader* shader;
sf::Sound* sound1;
sf::Sound* sound2;

const glm::vec3 boxDimensions(180, 90, 90);
const glm::vec3 padDimensions(30, 3, 40);
const glm::vec3 MirrorDimensions(3, 10, 10);

glm::vec3 ballPosition(0, ballRadius + padDimensions.y, boxDimensions.z / 2);
glm::vec3 ballDirection(1, 1, 0.2f);

CommandLineOptions options;

bool hasStarted        = true;
bool hasLost           = false;
bool jumpedToNextFrame = false;
bool isPaused          = false;

bool mouseLeftPressed   = false;
bool mouseLeftReleased  = false;
bool mouseRightPressed  = false;
bool mouseRightReleased = false;

const int numKeys = 16; // Manually updated
KeyValue keyDown[numKeys] = {
    { GLFW_KEY_1, false }, // Scene 1
    { GLFW_KEY_2, false }, // Scene 2
    { GLFW_KEY_3, false }, // Scene 3
    { GLFW_KEY_W, false }, // Move forward
    { GLFW_KEY_A, false }, // Move left
    { GLFW_KEY_S, false }, // Move Back
    { GLFW_KEY_D, false }, // Move right
    { GLFW_KEY_Q, false }, // Move down
    { GLFW_KEY_E, false }, // Move up
    { GLFW_KEY_LEFT_SHIFT, false }, // Move faster
    { GLFW_KEY_LEFT_CONTROL, false }, // Move slower
    { GLFW_KEY_B, false }, // Toggle bloom
    { GLFW_KEY_UP, false }, // Increase debug value 1
    { GLFW_KEY_DOWN, false }, // Decrease debug value 1
    { GLFW_KEY_LEFT, false }, // Decrease debug value 2
    { GLFW_KEY_RIGHT, false }, // Increase debug value 2
};

void updateScene(int sceneID) {
    for (int i = 0; i < numSecneProperties; i++) {
        
    }
}
// Modify if you want the music to start further on in the track. Measured in seconds.

float debugValue1 = 0.0f;
float debugValue2 = 0.0f;

const float debug_startTime = 0.0f;
double gameElapsedTime = debug_startTime;

double mouseSensitivity = 1.0;
bool firstMouseData = true;
double lastMouseX = windowWidth / 2;
double lastMouseY = windowHeight / 2;
float lookDirectionX = 0.0;
float lookDirectionY = 0.0;

float movementSpeed = 1.0;
float movementSpeedAmplified = 3.0;

void mouseCallback(GLFWwindow* window, double x, double y) {
    // Viewport setup
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    // Relative offset
    double deltaX = x - lastMouseX;
    double deltaY = y - lastMouseY;
    
    lookDirectionX += 0.01 * mouseSensitivity * deltaX;
    lookDirectionY += 0.01 * mouseSensitivity * deltaY;

    lookDirectionX = fmod(lookDirectionX, tau);

    // Limit view to 90 degrees up and down to prevent wack stuff
    if (lookDirectionY > tau/4) lookDirectionY = tau/4;//tau/4;
    if (lookDirectionY < -tau/4) lookDirectionY = -tau/4;//-tau/4; 

    // Skip first data from mouse, because the delta values are wrong
    if (firstMouseData == true) {
        firstMouseData = false;
        
        lookDirectionX = 0.0;
        lookDirectionY = 0.0;
    }

    glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);
}

// Keypress status handler
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    for (int i = 0; i < numKeys; i++) {
        if (key == keyDown[i].key && action == GLFW_PRESS)   keyDown[i].value = true;
        if (key == keyDown[i].key && action == GLFW_RELEASE) keyDown[i].value = false;
    }
    
}

bool isKeyDown(int keyCode) {
    for (int i = 0; i < numKeys; i++) {
        if (keyDown[i].key == keyCode && keyDown[i].value == true) {
            return true;
        }
    }
    return false;
}



unsigned int getTextureID(PNGImage texture) {
    unsigned int texID;
    
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, texture.pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    return texID;
}

void initGame(GLFWwindow* window, CommandLineOptions gameOptions) {
    buffer1 = new sf::SoundBuffer();
    buffer2 = new sf::SoundBuffer();
    if (!buffer1->loadFromFile("../res/DSP_main_theme.ogg")) {
        return;
    }
    if (!buffer2->loadFromFile("../res/Mountain_King.ogg")) {
        return;
    }
    
    // DSP main theme
    sound1 = new sf::Sound();
    sound1->setBuffer(*buffer1);
    sf::Time startTime = sf::seconds(debug_startTime);
    sound1->setPlayingOffset(startTime);
    sound1->setVolume(50);
    sound1->setLoop(true);

    // Sun bubbling sound, only when close to star
    sound2 = new sf::Sound();
    sound2->setBuffer(*buffer2);
    sound2->setPlayingOffset(startTime);
    sound2->setVolume(50);
    sound2->setLoop(true);

    if (!mute)    sound1->play();
    if (!mute)    sound2->play();

    options = gameOptions;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetKeyCallback(window, keyCallback);

    shader = new Gloom::Shader();
    shader->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");
    shader->activate();

    // Init rand
    srand(time(0));

    // Create meshes
    Mesh pad = cube(padDimensions, glm::vec2(30, 40), true);
    Mesh box = cube(boxDimensions, glm::vec2(90), true, true);
    Mesh sphere = generateSphere(1.0, 40, 40);
    Mesh text = generateTextGeometryBuffer("[" + std::to_string((int)(numMirrors * instances)) + "]", 39./29., 29);

    
    //glDisable(GL_CULL_FACE);  
    /*
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    */
    glUniform3f(9, fresnelColor.x, fresnelColor.y, fresnelColor.z);
    
    //Mesh mirrorModel = loadObj("../res/models/hex2sided.obj");
    Mesh mirrorModel = loadObj("../res/models/" + sceneConfigs[scene].mirrorModel);
    Mesh model = mirrorModel;
    Mesh dysonLayer1model = loadObj("../res/models/dysonLayer1.obj");
    Mesh dysonLayer2model = loadObj("../res/models/dysonLayer2.obj");
    Mesh dysonLayer3model = loadObj("../res/models/dysonLayer3.obj");

    std::cout << "spis meg py\n3" << std::endl;

    PNGImage charMap = loadPNGFile("../res/textures/charmap.png");
    GLuint textTexID = getTextureID(charMap);
    PNGImage skybox = loadPNGFile("../res/textures/skybox.png");
    GLuint skyboxTexID = getTextureID(skybox);
    PNGImage magnetsTexture = loadPNGFile("../res/textures/magnets_sprite_weak.png");
    GLuint magnetTexID = getTextureID(magnetsTexture);
    PNGImage UVTexture = loadPNGFile("../res/textures/uv.png");
    GLuint uvTexD = getTextureID(UVTexture);

    
    //PNGImage sun_col = loadPNGFile("../res/textures/sun_col.png");
    PNGImage starTex = loadPNGFile("../res/textures/" + sceneConfigs[scene].starTextureFile);
    GLuint starTexID = getTextureID(starTex);
    PNGImage prominenceTex = loadPNGFile("../res/textures/prominence.png");
    GLuint prominenceTexID = getTextureID(prominenceTex);

    textNode = createSceneNode();
    textNode->vertexArrayObjectID  = generateBuffer(text);
    textNode->VAOIndexCount        = text.indices.size();
    textNode->nodeType = TEXTURE;
    textNode->textureType = COLOR;
    textNode->texID = textTexID;

    // Text disabled
    textNode->scale.x = 0;
    textNode->scale.y = 0;

    // The root of all nodes
    rootNode = createSceneNode();

    // And a node for all the ones we don't want to see
    hiddenNode = createSceneNode();


    starNode = createSceneNode();
    starNode->vertexArrayObjectID  = generateBuffer(sphere);
    starNode->VAOIndexCount        = sphere.indices.size();
    starNode->nodeType = TEXTURE;
    starNode->textureType = COLOR;
    starNode->texID = starTexID;

    glowNode = createSceneNode();
    glowNode->vertexArrayObjectID  = generateBuffer(sphere);
    glowNode->VAOIndexCount        = sphere.indices.size();
    glowNode->nodeType = FRESNEL;
    
    glowNode2 = createSceneNode();
    glowNode2->vertexArrayObjectID  = generateBuffer(sphere);
    glowNode2->VAOIndexCount        = sphere.indices.size();
    glowNode2->nodeType = FRESNEL;
    
    glowNode3 = createSceneNode();
    glowNode3->vertexArrayObjectID  = generateBuffer(sphere);
    glowNode3->VAOIndexCount        = sphere.indices.size();
    glowNode3->nodeType = FRESNEL;



    // Replace this with mesh instantiating later

    int modelVAOID = generateBuffer(model);
    for (int i = 0; i < numMirrors; i++) {
        Mirror* newMirror = new Mirror();
        newMirror->position = glm::vec3(0, 0, 0);
        rootNode->children.push_back(newMirror);
        
        newMirror->vertexArrayObjectID = modelVAOID;
        newMirror->VAOIndexCount = model.indices.size();

        newMirror->radius = baseRadius + randRadius * random();
        newMirror->inclination = newMirror->radius * maxInclination * random();
        newMirror->LAN = tau * random();

        mirrors[i] = newMirror;
    }

    // Setting up the offet array
    for (int i = 0; i < instances; i++) {
        float IRS = instanceRandomSize;
        instanceOffset[i] = {IRS * 5 * normal(generator), IRS * normal(generator), IRS * normal(generator)};
        glUniform3fv(shader->getUniformFromName("instanceOffset[" +  std::to_string(i) + "]"), 1, glm::value_ptr(instanceOffset[i]));
    }


    // Define nodes
    boxNode  = createSceneNode();
    ballNode = createSceneNode();
    
    // Light source nodes
    lightNode0 = new PointLight();
    lightNode1 = new PointLight();
    lightNode2 = new PointLight();
    lightNode3 = new PointLight();
    PointLight* lights[] = {lightNode0, lightNode1, lightNode2, lightNode3} ;

    lightNode0->nodeType = POINT_LIGHT;
    lightNode1->nodeType = POINT_LIGHT;
    lightNode2->nodeType = POINT_LIGHT;
    lightNode3->nodeType = POINT_LIGHT;

    
    rootNode->position = glm::vec3(0, 0, 0);

    
    starNode->scale = glm::vec3(starSize, starSize, starSize);
    starNode->position = glm::vec3(0, 0, 0);


    // Config todo
    glowNode->position = glm::vec3(0, 0, 0);
    glowNode->scale = glm::vec3(1.01, 1.01, 1.01);
    glowNode2->position = glm::vec3(0, 0, 0);
    glowNode2->scale = glm::vec3(1.05, 1.05, 1.05);
    glowNode3->position = glm::vec3(0, 0, 0);
    glowNode3->scale = glm::vec3(1.15, 1.15, 1.15);

    
    

    // Config todo
    textNode->position = glm::vec3(-60, 0, -150);
    
    starNode->position = glm::vec3(0, 0, 0);
    glowNode->position = glm::vec3(0, 0, 0);
    glowNode->scale = glm::vec3(1.01, 1.01, 1.01);
    glowNode2->position = glm::vec3(0, 0, 0);
    glowNode2->scale = glm::vec3(1.05, 1.05, 1.05);
    glowNode3->position = glm::vec3(0, 0, 0);
    glowNode3->scale = glm::vec3(1.15, 1.15, 1.15);

   
    /*
    // Neutron star blue lights
    lightNode0->color = glm::vec3(0.2, 0.2, 0.9); // Red
    lightNode1->color = glm::vec3(0.2, 0.8, 0.9); // Green
    lightNode2->color = glm::vec3(0.2, 0.3, 0.9); // Intense red
    */
    
    // Default star yellow/orange lights
    lightNode0->color = glm::vec3(0.8, 0.2, 0.0); // Red
    lightNode1->color = glm::vec3(0.9, 0.4, 0.0); // Orange
    lightNode2->color = glm::vec3(0.9, 0.3, 0.0); // Intense red

    lightNode0->position = glm::vec3(0.4, 0, -0.3);
    lightNode1->position = glm::vec3(-0.4, 0, -0.3);
    lightNode2->position = glm::vec3(0.0, 0, 0.0);

    lightNode0->intensity = 0.3f;
    lightNode1->intensity = 0.3f;
    lightNode2->intensity = 10.3f;

    lightNode3->position = cameraPosition + glm::vec3(0, -0.5, 0);
    lightNode3->color = glm::vec3(0.9, 0.5, 0.0); // Hot orange
    lightNode3->intensity = 6.0f;
    
    
    unsigned int boxVAO  = generateBuffer(box);
    float skyboxScale = 500.0;
    boxNode->position = { 0, 0, 0 };
    boxNode->scale = glm::vec3(skyboxScale, skyboxScale, skyboxScale);
    boxNode->vertexArrayObjectID  = boxVAO;
    boxNode->VAOIndexCount        = box.indices.size();
    boxNode->nodeType = TEXTURE;
    boxNode->textureType = COLOR;
    boxNode->texID = skyboxTexID;

    // Solar prominence/arc from surface
    unsigned int padVAO  = generateBuffer(pad);
    arcNode = createSceneNode();
    arcNode->vertexArrayObjectID  = padVAO;
    arcNode->VAOIndexCount = pad.indices.size();
    arcNode->position = starSize/10 * glm::vec3(-1.0, 0.0, 0.0);
    arcNode->scale = 1.0f * glm::vec3(1/padDimensions.x, 0.001f/padDimensions.y, 1/padDimensions.z);
    arcNode->nodeType = TEXTURE;
    arcNode->textureType = COLOR;
    arcNode->texID = prominenceTexID;

    // Magnetic field lines
    /*
    for (int i = 0; i < numMagNodes; i++) {
        magNodes[i] = createSceneNode();
        magNodes[i]->vertexArrayObjectID  = padVAO;
        magNodes[i]->VAOIndexCount = pad.indices.size();
        magNodes[i]->position = starSize/10 * glm::vec3(0.0, 0.0, 0.0);
        magNodes[i]->scale = 10.0f * glm::vec3(1.5f/padDimensions.x, 0.001f/padDimensions.y, 1/padDimensions.z);
        magNodes[i]->nodeType = TEXTURE;
        magNodes[i]->textureType = COLOR;
        magNodes[i]->texID = magnetTexID;

        // Only difference is y-rotation relative to star
        magNodes[i]->rotation = glm::vec3(tau/4 + debugValue1, i * tau/(2 * numMagNodes), -tau/4);

        starNode->children.push_back(magNodes[i]);
    }
    */
    dysonNode1 = createSceneNode();
    dysonNode2 = createSceneNode();
    dysonNode3 = createSceneNode();

    unsigned int dysonLayer1VAO = generateBuffer(dysonLayer1model);
    unsigned int dysonLayer2VAO = generateBuffer(dysonLayer2model);
    unsigned int dysonLayer3VAO = generateBuffer(dysonLayer3model);

    dysonNode1->vertexArrayObjectID  = dysonLayer1VAO;
    dysonNode2->vertexArrayObjectID  = dysonLayer2VAO;
    dysonNode3->vertexArrayObjectID  = dysonLayer3VAO;

    dysonNode1->VAOIndexCount = dysonLayer1model.indices.size();
    dysonNode2->VAOIndexCount = dysonLayer2model.indices.size();
    dysonNode3->VAOIndexCount = dysonLayer3model.indices.size();

    float dysonShellRadius = 3.0f;
    dysonNode1->scale = 1.0f * glm::vec3(1, 1, 1) * dysonShellRadius;
    dysonNode2->scale = 1.1f * glm::vec3(1, 1, 1) * dysonShellRadius;
    dysonNode3->scale = 1.2f * glm::vec3(1, 1, 1) * dysonShellRadius;

    dysonNode1->nodeType = TEXTURE; dysonNode1->texID = uvTexD;
    dysonNode2->nodeType = TEXTURE; dysonNode2->texID = uvTexD;
    dysonNode3->nodeType = TEXTURE; dysonNode3->texID = uvTexD;


    // Doing color = vec4(0.2, 0.3, 0.9, 1.0) -> vec4(0.2, 0.3, 0.9, 0.9) is just a blend mode
    // How 2 blend tho??


    // Construct scene graph
    rootNode->children.push_back(boxNode);
    rootNode->children.push_back(starNode);
    rootNode->children.push_back(textNode);
    starNode->children.push_back(glowNode);
    starNode->children.push_back(glowNode2);
    starNode->children.push_back(arcNode);
    starNode->children.push_back(dysonNode1);
    starNode->children.push_back(dysonNode2);
    starNode->children.push_back(dysonNode3);


    getTimeDeltaSeconds();
    
    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;

    std::cout << "Ready. Click to start!" << std::endl;
}




void updateFrame(GLFWwindow* window) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    double timeDelta = getTimeDeltaSeconds();
    gameElapsedTime += timeDelta;
    //std::cout << gameElapsedTime << std::endl; // Debug time


    if (isKeyDown(GLFW_KEY_1)) scene = 0;
    if (isKeyDown(GLFW_KEY_2)) scene = 1;
    if (isKeyDown(GLFW_KEY_3)) scene = 2;
    
    // Debug interaction
    float debugValueSensitivity = 0.01f;
    if (isKeyDown(GLFW_KEY_UP)) debugValue1    += debugValueSensitivity;
    if (isKeyDown(GLFW_KEY_DOWN)) debugValue1  += -debugValueSensitivity;
    if (isKeyDown(GLFW_KEY_LEFT)) debugValue2  += -debugValueSensitivity;
    if (isKeyDown(GLFW_KEY_RIGHT)) debugValue2 += debugValueSensitivity;
    if (isKeyDown(GLFW_KEY_UP) ||
        isKeyDown(GLFW_KEY_UP) ||
        isKeyDown(GLFW_KEY_UP) ||
        isKeyDown(GLFW_KEY_RIGHT)) {
        /*    
        std::cout << 
        "Debug value 1: " << debugValue1 << std::endl << 
        "Debug value 2: " << debugValue2 << std::endl;
        */
    }
    
    // Reset debug values
    if (isKeyDown(GLFW_KEY_R)) {
        debugValue1 = 0.0f;
        debugValue2 = 0.0f;
    }

    
    glm::mat4 projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 100000.f);

    // It's a vec4 because it needs to be rotated with 4x4 matrices
    glm::vec4 movementVector = glm::vec4(0.0, 0.0, 0.0, 0.0);
    if (isKeyDown(GLFW_KEY_W)) movementVector.z += -1.0;
    if (isKeyDown(GLFW_KEY_S)) movementVector.z += 1.0;
    if (isKeyDown(GLFW_KEY_A)) movementVector.x += -1.0;
    if (isKeyDown(GLFW_KEY_D)) movementVector.x += 1.0;
    if (isKeyDown(GLFW_KEY_Q)) movementVector.y += -1.0;
    if (isKeyDown(GLFW_KEY_E)) movementVector.y += 1.0;

    if (glm::length(movementVector) > 0.1) {
        movementVector = glm::normalize(movementVector);
    }
    
    glm::vec4 cameraDirection = glm::vec4(0.0, 0.0, 0.0, 0.0);
    movementVector = glm::rotate(-lookDirectionY, glm::vec3(1, 0, 0)) * movementVector;
    movementVector = glm::rotate(-lookDirectionX, glm::vec3(0, 1, 0)) * movementVector;
    //movementVector *= -1; // I dunno why
    
    float speed;
    if (isKeyDown(GLFW_KEY_LEFT_SHIFT)) {
        speed = movementSpeedAmplified;
    } else if (isKeyDown(GLFW_KEY_LEFT_CONTROL)) {
        speed = 1/glm::pow(movementSpeedAmplified, 2);
    } else {
        speed = movementSpeed;
    }

    cameraPosition += (glm::vec3) movementVector * speed;

    glm::mat4 cameraTransform =
                    glm::rotate(lookDirectionY, glm::vec3(1, 0, 0)) *
                    glm::rotate(lookDirectionX, glm::vec3(0, 1, 0)) *
                    glm::translate(-cameraPosition);

    glm::mat4 VP = projection * cameraTransform;


    // Move and rotate various SceneNodes
    
    

    updateNodeTransformations(rootNode, glm::identity<glm::mat4>(), VP);

    // Star
    float t = gameElapsedTime * timeSpeedup; // Adjustable simulated time
    starNode->rotation = { 0, t/4, 0 }; // Config todo
    
    // surfance prominence
    arcNode->rotation = { gameElapsedTime/3 + tau/2, tau/4, -tau/4 };



    lightNode2->position.x = 0.5-padPositionX;
    lightNode2->position.y = -0.3;
    lightNode2->position.z = 0.3-padPositionZ;

    lightNode3->position = glm::vec3(0.0, 0.0, 0.0); // + starNode->position

    float distanceToStar = glm::length(cameraPosition - starNode->position);
    // Will play from within 3 star radii, fading in from 3 radii and increasing inwards
    float starVolume = glm::pow(2.71, -(distanceToStar-starSize)/starSize);
    sound2->setVolume(glm::clamp(100.0*starVolume, 0.0, 100.0));

    for (int i = 0; i < numMirrors; i++) {

        float inc = mirrors[i]->inclination;
        float LAN = mirrors[i]->LAN; // Longitude of the ascending node (Inclination rotation offset)
        float r = mirrors[i]->radius; // Orbital radius

        float offset = tau * i/numMirrors; // Mean anomaly (Offset from first mirror)
        float o = std::pow(r/baseRadius, -1.5) * t + offset; // Orbit position, how far in the circular orbit each mirror is

        mirrors[i]->position = starNode->position + glm::vec3(
            r * glm::sin(o),
            inc * glm::sin(o + LAN),
            r * glm::cos(o)
        );

        mirrors[i]->scale = mirrorScale * glm::vec3(1, 1, 1);
        mirrors[i]->rotation = { tau/4, o, 0 };

    }


    // --- Shader stuff for lighting --- //

    // Passing a uniform to the vertex and fragment shaders
    glUniform3f(0, 0.5, 1.0, 1.5);

    float ambient = 0.05;
    glUniform1f(7, ambient);
    
    glUniform1i(10, calculateShadows);

    glUniform3f(8, ballPosition.x/180, ballPosition.y/90, ballPosition.z/90);

    glUniform3f(12, cameraPosition.x, cameraPosition.y, cameraPosition.z);
    
    glUniform1f(13, gameElapsedTime);

    //glm::vec3 test = glm::vec3(0.0, 2.0, 0.0);
    //glUniform3f(12, test.x, test.y, test.z);

    // Setting light values
    
    /*
    for (int i = 0; i < 4; i++) {
        std::string identifierString;
        GLint location;

        // Color
        identifierString = fmt::format("lights[{}].color", i);
        location = shader->getUniformFromName(identifierString);
        glUniform3fv(location, 1, glm::value_ptr(lightNode0->color));

        // Position
        identifierString = fmt::format("lights[{}].position", i);
        location = shader->getUniformFromName(identifierString);
        glUniform3fv(location, 1, glm::value_ptr(lightNode0->position));

        // Intensity
        identifierString = fmt::format("lights[{}].intensity", i);
        location = shader->getUniformFromName(identifierString);
        glUniform1f(location, lightNode0->intensity);
    }
    */
    // This is not pretty, but fmt does not want to cooperate with me, and has forced my hand
    
    glUniform3fv(shader->getUniformFromName("lights[0].color"),     1, glm::value_ptr(lightNode0->color));
    glUniform3fv(shader->getUniformFromName("lights[0].position"),  1, glm::value_ptr(lightNode0->position));
    glUniform1f( shader->getUniformFromName("lights[0].intensity"),                   lightNode0->intensity);


    glUniform3fv(shader->getUniformFromName("lights[1].color"),     1, glm::value_ptr(lightNode1->color));
    glUniform3fv(shader->getUniformFromName("lights[1].position"),  1, glm::value_ptr(lightNode1->position));
    glUniform1f( shader->getUniformFromName("lights[1].intensity"),                   lightNode1->intensity);

    glUniform3fv(shader->getUniformFromName("lights[2].color"),     1, glm::value_ptr(lightNode2->color));
    glUniform3fv(shader->getUniformFromName("lights[2].position"),  1, glm::value_ptr(lightNode2->position));
    glUniform1f( shader->getUniformFromName("lights[2].intensity"),                   lightNode2->intensity);

    glUniform3fv(shader->getUniformFromName("lights[3].color"),     1, glm::value_ptr(lightNode3->color));
    glUniform3fv(shader->getUniformFromName("lights[3].position"),  1, glm::value_ptr(lightNode3->position));
    glUniform1f( shader->getUniformFromName("lights[3].intensity"),                   lightNode3->intensity);


    

}


void updateNodeTransformations(SceneNode* node, glm::mat4 modelTransformationThusFar, glm::mat4 VP) {
    
    glm::mat4 transformationMatrix =
              glm::translate(node->position)
            * glm::translate(node->referencePoint)
                * glm::rotate(node->rotation.y, glm::vec3(0,1,0))
                * glm::rotate(node->rotation.x, glm::vec3(1,0,0))
                * glm::rotate(node->rotation.z, glm::vec3(0,0,1))
                * glm::scale(node->scale)
            * glm::translate(-node->referencePoint);

    // Model Matrix
    node->currentTransformationMatrix = VP * modelTransformationThusFar * transformationMatrix;
    node->modelMatrix =                      modelTransformationThusFar * transformationMatrix;

    switch(node->nodeType) {
        case GEOMETRY: break;
        case INSTANCED: break;
        case POINT_LIGHT: break;
    }

    for(SceneNode* child : node->children) {
        updateNodeTransformations(child, node->modelMatrix, VP);
    }
}

void renderNode(SceneNode* node) {
    glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));
    glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(node->modelMatrix));

    // Normal matrix
    // Object specific lighting
    glm::mat4 inverseModelMatrix = glm::inverse(node->modelMatrix);
    glm::mat3 inverseModelMatrix3f = glm::mat3(inverseModelMatrix);
    glUniformMatrix3fv(6, 1, GL_TRUE, glm::value_ptr(inverseModelMatrix3f));


    switch(node->nodeType) {
        case GEOMETRY:
            if(node->vertexArrayObjectID != -1) {
                glBindVertexArray(node->vertexArrayObjectID);
                glUniform1i(64, 0); // is_textured = false
                glUniform1i(5, 0); // is_instanced = false
                glUniform1i(11, 0); // is_fresnel = false
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            break;

        case INSTANCED:
            if(node->vertexArrayObjectID != -1) {
                //glEnable(GL_BLEND);
                //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glBindVertexArray(node->vertexArrayObjectID);
                glUniform1i(64, 0); // is_textured = false
                glUniform1i(5, 1); // is_instanced = true
                glUniform1i(11, 0); // is_fresnel = false
                glDrawElementsInstanced(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, 0, instances);
                //glDisable(GL_BLEND);
                //glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
            }
            break;

            
        case TEXTURE:
            if(node->vertexArrayObjectID != -1) {
                // 1366x768
                glBindTextureUnit(1, node->texID);

                glBindVertexArray(node->vertexArrayObjectID);
                glUniform1i(64, 1); // is_textured = true
                glUniform1i(5, 0); // is_instanced = false
                glUniform1i(11, 0); // is_fresnel = false
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
        
        case FRESNEL:
            if(node->vertexArrayObjectID != -1) {
                // 1366x768
                glBindTextureUnit(1, node->texID);

                glBindVertexArray(node->vertexArrayObjectID);
                glUniform1i(64, 0); // is_textured = false
                glUniform1i(5, 0); // is_instanced = false
                glUniform1i(11, 1); // is_fresnel = true
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            

            break;
        case POINT_LIGHT: break;
    }

    for(SceneNode* child : node->children) {
        renderNode(child);
    }
}

void renderFrame(GLFWwindow* window) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    renderNode(rootNode);
}
