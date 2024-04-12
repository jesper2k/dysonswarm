
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


// Scene setup
int scene = 1;
SceneConfig sceneConfigs[3] = {
    {
        /* Mirror objects                */ 50,
        /* Instances per object          */ 10,
        /* Mirror size                   */ 0.020,
        /* Star texture filename         */ "sun_col.png",
        /* Mirror model filename         */ "hex.obj",
        /* Fresnel color                 */ glm::vec3(0.9, 0.5, 0.1),
    },
    {
        500,
        200,
        0.003,
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



const float timeSpeedup = 0.25; // 0.03

float mirrorScale = sceneConfigs[scene].mirrorSize;
int numMirrors = sceneConfigs[scene].numMirrors;
const int maxNumMirrors = 500; // Config todo
const float baseRadius = 40; // Config todo
const float randRadius = 60;// Config todo
const float maxInclination = 0.15; // of radius
glm::vec3 fresnelColor = sceneConfigs[scene].fresnelColor;

glm::vec3 cameraPosition = glm::vec3(0, 20, 100);

Mirror* mirrors[maxNumMirrors];

int instances = sceneConfigs[scene].instances;
const float instanceRandomSize = 400.0f;// Config todo
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
SceneNode* boxNode;
SceneNode* ballNode;
SceneNode* starNode;
SceneNode* glowNode; // Fresnel for star
SceneNode* glowNode2; // Fresnel for star
SceneNode* glowNode3; // Fresnel for star
SceneNode* padNode;
SceneNode* textNode;

PointLight* lightNode0;
PointLight* lightNode1;
PointLight* lightNode2;
PointLight* lightNode3;
PointLight* lights[4];

double ballRadius = 3.0f;


// These are heap allocated, because they should not be initialised at the start of the program
sf::SoundBuffer* buffer;
Gloom::Shader* shader;
sf::Sound* sound;

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

const int numKeys = 11;
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
    { GLFW_KEY_B, false }, // Toggle bloom
};

// Modify if you want the music to start further on in the track. Measured in seconds.
const float debug_startTime = 0;
double totalElapsedTime = debug_startTime;
double gameElapsedTime = debug_startTime;

double mouseSensitivity = 1.0;
double lastMouseX = windowWidth / 2;
double lastMouseY = windowHeight / 2;
float lookDirectionX = 0.0;
float lookDirectionY = 0.0;

float movementSpeed = 1.0;
float movementSpeedAmplified = 3.0;

void mouseCallback(GLFWwindow* window, double x, double y) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    double deltaX = x - lastMouseX;
    double deltaY = y - lastMouseY;
    
    lookDirectionX += 0.01 * mouseSensitivity * deltaX;
    lookDirectionY += 0.01 * mouseSensitivity * deltaY;


    lookDirectionX = fmod(lookDirectionX, tau);

    // Limit view to 90 degrees up and down to prevent wack stuff
    if (lookDirectionY > tau/4) lookDirectionY = tau/4;//tau/4;
    if (lookDirectionY < -tau/4) lookDirectionY = -tau/4;//-tau/4; 



    padPositionX -= mouseSensitivity * deltaX / windowWidth;
    padPositionZ -= mouseSensitivity * deltaY / windowHeight;

    /*
    if (padPositionX > 1) padPositionX = 1;
    if (padPositionX < 0) padPositionX = 0;
    if (padPositionZ > 1) padPositionZ = 1;
    if (padPositionZ < 0) padPositionZ = 0;
    */

    glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);
}

// Keypress status handler
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    /*
    // Simple example
    if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        std::cout << "key E pressed" << std::endl;
    }
    */

    
    for (int i = 0; i < numKeys; i++) {
        if (key == keyDown[i].key && action == GLFW_PRESS)   {
            keyDown[i].value = true;
            //std::cout << std::to_string(keyDown[i].key) + " : " << keyDown[i].value << std::endl;
        }
        if (key == keyDown[i].key && action == GLFW_RELEASE) {
            keyDown[i].value = false;
            //std::cout << std::to_string(keyDown[i].key) + " : " << keyDown[i].value << std::endl;
        }
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
    buffer = new sf::SoundBuffer();
    if (!buffer->loadFromFile("../res/DSP_main_theme.ogg")) {
        return;
    }
    
    sound = new sf::Sound();
    sound->setBuffer(*buffer);
    //sf::Time startTime = sf::seconds(debug_startTime);
    //sound->setPlayingOffset(startTime);
    //sound->play();


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
    Mesh mirror = cube(padDimensions, glm::vec2(30, 40), true);
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

    std::cout << "spis meg py" << std::endl;

    PNGImage charMap = loadPNGFile("../res/textures/charmap.png");
    GLuint textTexID = getTextureID(charMap);
    PNGImage skybox = loadPNGFile("../res/textures/skybox.png");
    GLuint skyboxTexID = getTextureID(skybox);

    
    //PNGImage sun_col = loadPNGFile("../res/textures/sun_col.png");
    PNGImage starTex = loadPNGFile("../res/textures/" + sceneConfigs[scene].starTextureFile);
    GLuint starTexID = getTextureID(starTex);

    textNode = createSceneNode();
    textNode->vertexArrayObjectID  = generateBuffer(text);
    textNode->VAOIndexCount        = text.indices.size();
    textNode->nodeType = TEXTURE;
    textNode->textureType = COLOR;
    textNode->texID = textTexID;

    // Text disabled
    textNode->scale.x = 0;
    textNode->scale.y = 0;

    //PNGImage sunTexture = loadPNGFile("../res/textures/8k_sun.png");
    //GLuint sunTexID = getTextureID(sunTexture);
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

    rootNode = createSceneNode();

    // Replace this with mesh instantiating later

    for (int i = 0; i < numMirrors; i++) {
        Mirror* newMirror = new Mirror();
        newMirror->position = glm::vec3(0, 0, 0);
        rootNode->children.push_back(newMirror);
        
        newMirror->vertexArrayObjectID = generateBuffer(model);
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

    // Fill buffers
    unsigned int boxVAO  = generateBuffer(box);
    unsigned int padVAO  = generateBuffer(mirror);

    // Construct scene
    boxNode  = createSceneNode();
    padNode  = createSceneNode();
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

    // Config todo
    textNode->position = glm::vec3(-60, 0, -150);
    lightNode0->position = glm::vec3(0.2, -0.3, -0.3);
    lightNode1->position = glm::vec3(-0.2, -0.3, -0.3);
    lightNode2->position = glm::vec3(-0.6, -0.3, -0.3);
    
    rootNode->position = glm::vec3(0, 0, 0);

    float starSize = 10; // Config todo
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
    /*
    lightNode0->color = glm::vec3(0.2, 0.2, 0.9); // Red
    lightNode1->color = glm::vec3(0.2, 0.8, 0.9); // Green
    lightNode2->color = glm::vec3(0.2, 0.3, 0.9); // Intense red
    */

    lightNode0->color = glm::vec3(0.8, 0.2, 0.1); // Red
    lightNode1->color = glm::vec3(0.2, 0.8, 0.1); // Green
    lightNode2->color = glm::vec3(0.9, 0.3, 0.0); // Intense red

    lightNode0->intensity = 5.0f;
    lightNode1->intensity = 0.0f;
    lightNode2->intensity = 0.0f;

    lightNode3->position = cameraPosition + glm::vec3(0, -0.5, 0);
    lightNode3->color = glm::vec3(0.8, 0.8, 1.0); // Hot orange
    lightNode3->intensity = 0.0f;

    
    rootNode->children.push_back(boxNode);
    rootNode->children.push_back(starNode);
    rootNode->children.push_back(textNode);
    starNode->children.push_back(glowNode);
    starNode->children.push_back(glowNode2);
    //starNode->children.push_back(glowNode3);

    float skyboxScale = 500.0;
    boxNode->position = { 0, 0, 0 };
    boxNode->scale = glm::vec3(skyboxScale, skyboxScale, skyboxScale);
    boxNode->vertexArrayObjectID  = boxVAO;
    boxNode->VAOIndexCount        = box.indices.size();
    boxNode->nodeType = TEXTURE;
    boxNode->textureType = COLOR;
    boxNode->texID = skyboxTexID;

    padNode->vertexArrayObjectID  = padVAO;
    padNode->VAOIndexCount        = mirror.indices.size();


    getTimeDeltaSeconds();
    
    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;

    std::cout << "Ready. Click to start!" << std::endl;
}




void updateFrame(GLFWwindow* window) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    double timeDelta = getTimeDeltaSeconds();

    const float ballBottomY = boxNode->position.y - (boxDimensions.y/2) + ballRadius + padDimensions.y;
    const float ballTopY    = boxNode->position.y + (boxDimensions.y/2) - ballRadius;
    const float BallVerticalTravelDistance = ballTopY - ballBottomY;

    const float cameraWallOffset = 30; // Arbitrary addition to prevent ball from going too much into camera

    const float ballMinX = boxNode->position.x - (boxDimensions.x/2) + ballRadius;
    const float ballMaxX = boxNode->position.x + (boxDimensions.x/2) - ballRadius;
    const float ballMinZ = boxNode->position.z - (boxDimensions.z/2) + ballRadius;
    const float ballMaxZ = boxNode->position.z + (boxDimensions.z/2) - ballRadius - cameraWallOffset;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1)) {
        mouseLeftPressed = true;
        mouseLeftReleased = false;
    } else {
        mouseLeftReleased = mouseLeftPressed;
        mouseLeftPressed = false;
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2)) {
        mouseRightPressed = true;
        mouseRightReleased = false;
    } else {
        mouseRightReleased = mouseRightPressed;
        mouseRightPressed = false;
    }

    if(!hasStarted) {
        if (mouseLeftPressed) {
            if (options.enableMusic) {
                /*
                sound = new sf::Sound();
                sound->setBuffer(*buffer);
                sf::Time startTime = sf::seconds(debug_startTime);
                sound->setPlayingOffset(startTime);
                sound->play();
                */
            }
            totalElapsedTime = debug_startTime;
            gameElapsedTime = debug_startTime;
            hasStarted = true;
        }

        ballPosition.x = ballMinX + (1 - padPositionX) * (ballMaxX - ballMinX);
        ballPosition.y = ballBottomY;
        ballPosition.z = ballMinZ + (1 - padPositionZ) * ((ballMaxZ+cameraWallOffset) - ballMinZ);
    } else {
        totalElapsedTime += timeDelta;
        
        if(hasLost) {
            if (mouseLeftReleased) {
                hasLost = false;
                //hasStarted = false;
                currentKeyFrame = 0;
                previousKeyFrame = 0;
            }
        } else if (isPaused) {
            if (mouseRightReleased) {
                isPaused = false;
                if (options.enableMusic) {
                    //sound->play();
                }
            }
        } else {
            gameElapsedTime += timeDelta;
            if (mouseRightReleased) {
                //isPaused = true;
                if (options.enableMusic) {
                    //sound->pause();
                }
            }
            // Get the timing for the beat of the song
            for (unsigned int i = currentKeyFrame; i < keyFrameTimeStamps.size(); i++) {
                if (gameElapsedTime < keyFrameTimeStamps.at(i)) {
                    continue;
                }
                currentKeyFrame = i;
            }

            jumpedToNextFrame = currentKeyFrame != previousKeyFrame;
            previousKeyFrame = currentKeyFrame;

            double frameStart = keyFrameTimeStamps.at(currentKeyFrame);
            double frameEnd = keyFrameTimeStamps.at(currentKeyFrame + 1); // Assumes last keyframe at infinity

            double elapsedTimeInFrame = gameElapsedTime - frameStart;
            double frameDuration = frameEnd - frameStart;
            double fractionFrameComplete = elapsedTimeInFrame / frameDuration;

            double ballYCoord;

            KeyFrameAction currentOrigin = keyFrameDirections.at(currentKeyFrame);
            KeyFrameAction currentDestination = keyFrameDirections.at(currentKeyFrame + 1);

            // Synchronize ball with music
            if (currentOrigin == BOTTOM && currentDestination == BOTTOM) {
                ballYCoord = ballBottomY;
            } else if (currentOrigin == TOP && currentDestination == TOP) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance;
            } else if (currentDestination == BOTTOM) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance * (1 - fractionFrameComplete);
            } else if (currentDestination == TOP) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance * fractionFrameComplete;
            }

            // Make ball move
            const float ballSpeed = 60.0f;
            ballPosition.x += timeDelta * ballSpeed * ballDirection.x;
            ballPosition.y = ballYCoord;
            ballPosition.z += timeDelta * ballSpeed * ballDirection.z;

            // Make ball bounce
            if (ballPosition.x < ballMinX) {
                ballPosition.x = ballMinX;
                ballDirection.x *= -1;
            } else if (ballPosition.x > ballMaxX) {
                ballPosition.x = ballMaxX;
                ballDirection.x *= -1;
            }
            if (ballPosition.z < ballMinZ) {
                ballPosition.z = ballMinZ;
                ballDirection.z *= -1;
            } else if (ballPosition.z > ballMaxZ) {
                ballPosition.z = ballMaxZ;
                ballDirection.z *= -1;
            }

            /*
            if(options.enableAutoplay) {
                padPositionX = 1-(ballPosition.x - ballMinX) / (ballMaxX - ballMinX);
                padPositionZ = 1-(ballPosition.z - ballMinZ) / ((ballMaxZ+cameraWallOffset) - ballMinZ);
            }
            */

            // Check if the ball is hitting the pad when the ball is at the bottom.
            // If not, you just lost the game! (hehe)
            if (jumpedToNextFrame && currentOrigin == BOTTOM && currentDestination == TOP) {
                double padLeftX  = boxNode->position.x - (boxDimensions.x/2) + (1 - padPositionX) * (boxDimensions.x - padDimensions.x);
                double padRightX = padLeftX + padDimensions.x;
                double padFrontZ = boxNode->position.z - (boxDimensions.z/2) + (1 - padPositionZ) * (boxDimensions.z - padDimensions.z);
                double padBackZ  = padFrontZ + padDimensions.z;

                if (   ballPosition.x < padLeftX
                    || ballPosition.x > padRightX
                    || ballPosition.z < padFrontZ
                    || ballPosition.z > padBackZ
                ) {
                    hasLost = true;
                    if (options.enableMusic) {
                        /*
                        sound->stop();
                        delete sound;
                        */
                    }
                }
            }
        }
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
    
    movementVector = glm::rotate(-lookDirectionY, glm::vec3(1, 0, 0)) * movementVector;
    movementVector = glm::rotate(-lookDirectionX, glm::vec3(0, 1, 0)) * movementVector;
    //movementVector *= -1; // I dunno why
    
    float speed;
    if (isKeyDown(GLFW_KEY_LEFT_SHIFT)) {
        speed = movementSpeedAmplified;
    } else {
        speed = movementSpeed;
    }

    cameraPosition += (glm::vec3) movementVector * speed;


    // Some fancy math to make the camera move in a nice way
    //float lookRotation = -0.6 / (1 + exp(-5 * (padPositionX-0.5))) + 0.3;
    glm::mat4 cameraTransform =
                    glm::rotate(lookDirectionY, glm::vec3(1, 0, 0)) *
                    glm::rotate(lookDirectionX, glm::vec3(0, 1, 0)) *
                    glm::translate(-cameraPosition);

    glm::mat4 VP = projection * cameraTransform;


    // Move and rotate various SceneNodes
    ballNode->position = ballPosition;
    ballNode->scale = glm::vec3(ballRadius);
    ballNode->rotation = { 0, totalElapsedTime*2, 0 };
    
    padNode->position  = {
        boxNode->position.x - (boxDimensions.x/2) + (padDimensions.x/2) + (1 - padPositionX) * (boxDimensions.x - padDimensions.x),
        boxNode->position.y - (boxDimensions.y/2) + (padDimensions.y/2),
        boxNode->position.z - (boxDimensions.z/2) + (padDimensions.z/2) + (1 - padPositionZ) * (boxDimensions.z - padDimensions.z)
    };

    updateNodeTransformations(rootNode, glm::identity<glm::mat4>(), VP);

    // Star
    float t = totalElapsedTime * timeSpeedup; // Adjustable simulated time
    starNode->rotation = { 0, t * 10, 0 }; // Config todo

    lightNode2->position.x = 0.5-padPositionX;
    lightNode2->position.y = -0.3;
    lightNode2->position.z = 0.3-padPositionZ;

    lightNode3->position = glm::vec3(0.0, 0.0, 0.0); // + starNode->position
    
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
                glBindVertexArray(node->vertexArrayObjectID);
                glUniform1i(64, 0); // is_textured = false
                glUniform1i(5, 1); // is_instanced = true
                glUniform1i(11, 0); // is_fresnel = false
                glDrawElementsInstanced(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, 0, instances);
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
