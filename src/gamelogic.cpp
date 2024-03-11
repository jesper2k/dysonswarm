
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

int calculateShadows = 0;
int showBox = 0;

float pi = 3.1415926535897926462;
float tau = 2 * pi;

const float timeSpeedup = 0.2;

const float mirrorScale = 0.025;
const int numMirrors = 500;
const float baseRadius = 40;
const float randRadius = 20;
const float maxInclination = 0.1; // of radius


Mirror* mirrors[numMirrors];

double padPositionX = 0;
double padPositionZ = 0;

unsigned int currentKeyFrame = 0;
unsigned int previousKeyFrame = 0;


SceneNode* rootNode;
SceneNode* boxNode;
SceneNode* ballNode;
SceneNode* starNode;
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

bool hasStarted        = false;
bool hasLost           = false;
bool jumpedToNextFrame = false;
bool isPaused          = false;

bool mouseLeftPressed   = false;
bool mouseLeftReleased  = false;
bool mouseRightPressed  = false;
bool mouseRightReleased = false;



// Modify if you want the music to start further on in the track. Measured in seconds.
const float debug_startTime = 0;
double totalElapsedTime = debug_startTime;
double gameElapsedTime = debug_startTime;

double mouseSensitivity = 1.0;
double lastMouseX = windowWidth / 2;
double lastMouseY = windowHeight / 2;
void mouseCallback(GLFWwindow* window, double x, double y) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    double deltaX = x - lastMouseX;
    double deltaY = y - lastMouseY;

    padPositionX -= mouseSensitivity * deltaX / windowWidth;
    padPositionZ -= mouseSensitivity * deltaY / windowHeight;

    if (padPositionX > 1) padPositionX = 1;
    if (padPositionX < 0) padPositionX = 0;
    if (padPositionZ > 1) padPositionZ = 1;
    if (padPositionZ < 0) padPositionZ = 0;

    glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);
}

float random() {
    return (float)(rand() % 10000) / 10000.0;
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
    if (!buffer->loadFromFile("../res/Hall of the Mountain King.ogg")) {
        return;
    }

    options = gameOptions;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetCursorPosCallback(window, mouseCallback);

    shader = new Gloom::Shader();
    shader->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");
    shader->activate();

    // Init rand
    srand(time(0));

    // Loading textures
    PNGImage charMap = loadPNGFile("../res/textures/charmap.png");

    GLuint texID = getTextureID(charMap);
    
    Mesh text = generateTextGeometryBuffer("Hello, world!", 39./29., 29);
    //std::cout << text.indices.size() << std::endl;
    textNode = createSceneNode();
    textNode->vertexArrayObjectID  = generateBuffer(text);
    textNode->VAOIndexCount        = text.indices.size();
    textNode->nodeType = TEXTURE;
    textNode->texID = texID;

    textNode->scale.x = 10;
    textNode->scale.y = 10;

    // Create meshes
    Mesh pad = cube(padDimensions, glm::vec2(30, 40), true);
    Mesh box = cube(boxDimensions, glm::vec2(90), true, true);
    Mesh mirror = cube(boxDimensions, glm::vec2(90), true, true);
    Mesh sphere = generateSphere(1.0, 40, 40);



    rootNode = createSceneNode();

    // Replace this with mesh instantiating later

    for (int i = 0; i < numMirrors; i++) {
        Mirror* newMirror = new Mirror();
        newMirror->position = glm::vec3(0, 0, 0);
        rootNode->children.push_back(newMirror);
        
        newMirror->vertexArrayObjectID = generateBuffer(pad);
        newMirror->VAOIndexCount = pad.indices.size();

        newMirror->radius = baseRadius + randRadius * random();
        newMirror->inclination = newMirror->radius * maxInclination * random();
        newMirror->LAN = tau * random();

        mirrors[i] = newMirror;
    }
    

    // Fill buffers
    unsigned int ballVAO = generateBuffer(sphere);
    unsigned int starVAO = generateBuffer(sphere);
    unsigned int boxVAO  = generateBuffer(box);
    unsigned int padVAO  = generateBuffer(pad);
    unsigned int mirrorVAO  = generateBuffer(pad);

    // Construct scene
    boxNode  = createSceneNode();
    padNode  = createSceneNode();
    ballNode = createSceneNode();
    starNode = createSceneNode();
    
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

    textNode->position = glm::vec3(0, 0, -110);
    lightNode0->position = glm::vec3(0.2, -0.3, -0.3);
    lightNode1->position = glm::vec3(-0.2, -0.3, -0.3);
    lightNode2->position = glm::vec3(-0.6, -0.3, -0.3);
    lightNode3->position = glm::vec3(0.0, 0.0, 0.0);
    
    starNode->position = glm::vec3(0, -20, -100);


    lightNode0->color = glm::vec3(0.8, 0.2, 0.1); // Red
    lightNode1->color = glm::vec3(0.2, 0.8, 0.1); // Green
    lightNode2->color = glm::vec3(0.1, 0.2, 0.8); // Blue
    lightNode2->intensity = 0.3f;
    lightNode2->intensity = 0.3f;
    lightNode2->intensity = 0.3f;

    lightNode3->color = glm::vec3(0.9, 0.5, 0.0); // Hot orange
    lightNode3->intensity = 3.0f;

    if (showBox) {
        rootNode->children.push_back(boxNode);
    }
    //rootNode->children.push_back(padNode);
    //rootNode->children.push_back(ballNode);
    rootNode->children.push_back(starNode);
    //rootNode->children.push_back(textNode);
    //ballNode->children.push_back(lightNode3); // Moving light

    boxNode->vertexArrayObjectID  = boxVAO;
    boxNode->VAOIndexCount        = box.indices.size();

    padNode->vertexArrayObjectID  = padVAO;
    padNode->VAOIndexCount        = pad.indices.size();

    ballNode->vertexArrayObjectID = ballVAO;
    ballNode->VAOIndexCount       = sphere.indices.size();
    starNode->vertexArrayObjectID = ballVAO;
    starNode->VAOIndexCount       = sphere.indices.size();




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
                sound = new sf::Sound();
                sound->setBuffer(*buffer);
                sf::Time startTime = sf::seconds(debug_startTime);
                sound->setPlayingOffset(startTime);
                sound->play();
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
                hasStarted = false;
                currentKeyFrame = 0;
                previousKeyFrame = 0;
            }
        } else if (isPaused) {
            if (mouseRightReleased) {
                isPaused = false;
                if (options.enableMusic) {
                    sound->play();
                }
            }
        } else {
            gameElapsedTime += timeDelta;
            if (mouseRightReleased) {
                isPaused = true;
                if (options.enableMusic) {
                    sound->pause();
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

            if(options.enableAutoplay) {
                padPositionX = 1-(ballPosition.x - ballMinX) / (ballMaxX - ballMinX);
                padPositionZ = 1-(ballPosition.z - ballMinZ) / ((ballMaxZ+cameraWallOffset) - ballMinZ);
            }

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
                        sound->stop();
                        delete sound;
                    }
                }
            }
        }
    }

    glm::mat4 projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 350.f);

    glm::vec3 cameraPosition = glm::vec3(0, 2, -10);

    // Some fancy math to make the camera move in a nice way
    float lookRotation = -0.6 / (1 + exp(-5 * (padPositionX-0.5))) + 0.3;
    glm::mat4 cameraTransform =
                    glm::rotate(0.3f + 0.2f * float(-padPositionZ*padPositionZ), glm::vec3(1, 0, 0)) *
                    glm::rotate(lookRotation, glm::vec3(0, 1, 0)) *
                    glm::translate(-cameraPosition);

    glm::mat4 VP = projection * cameraTransform;


    // Move and rotate various SceneNodes
    boxNode->position = { 0, -10, -80 };

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

    float starSize = 10;
    starNode->position = glm::vec3(0, -20, -100);
    starNode->scale = glm::vec3(starSize, starSize, starSize);
    starNode->rotation = { 0, t/2, 0 };

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

    float boxSize = boxDimensions.y;
    glUniform3f(8, ballPosition.x/180, ballPosition.y/90, ballPosition.z/90);



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
                glUniform1i(64, 0);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            break;
        case TEXTURE:
            
            /*
            if(node->vertexArrayObjectID != -1) {
                glBindVertexArray(node->vertexArrayObjectID);
                glUniform1i(64, 1);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            */
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
