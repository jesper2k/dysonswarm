#pragma once

#include <utilities/window.hpp>
#include <utilities/imageLoader.hpp>
#include "sceneGraph.hpp"

void updateNodeTransformations(SceneNode* node, glm::mat4 transformationThusFar, glm::mat4 VP);
void initGame(GLFWwindow* window, CommandLineOptions options);
void initScene();
void updateFrame(GLFWwindow* window);
void renderFrame(GLFWwindow* window);
unsigned int getTextureID(PNGImage texture);