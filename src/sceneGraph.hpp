#pragma once

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stack>
#include <vector>
#include <cstdio>
#include <stdbool.h>
#include <cstdlib> 
#include <ctime> 
#include <chrono>
#include <fstream>

enum SceneNodeType {
	GEOMETRY, POINT_LIGHT, TEXTURE, INSTANCED, FRESNEL, DYSON, ANIMATION
};

enum TextureType {
	COLOR, NORMAL, ROUGHNESS,
};

struct SceneConfig {
    float starSize;
    int numMirrors;
    int instances; // Total meshes will be numMirrors * instances
    float mirrorSize;
    std::string starTextureFile;
    std::string mirrorModel;
    glm::vec3 fresnelColor;
    float swarmMinRadius;
    float swarmMaxRadius;
    float swarmOrbitSpeed;
    float swarmInclination;
};

struct KeyValue {
    int key;
    bool value;
};

struct SceneNode {
	SceneNode() {
		position = glm::vec3(0, 0, 0);
		rotation = glm::vec3(0, 0, 0);
		scale = glm::vec3(1, 1, 1);
		RGBA = glm::vec4(0, 0, 0, 0);

        referencePoint = glm::vec3(0, 0, 0);
        vertexArrayObjectID = -1;
        VAOIndexCount = 0;

        nodeType = GEOMETRY;

	}

	// A list of all children that belong to this node.
	// For instance, in case of the scene graph of a human body shown in the assignment text, the "Upper Torso" node would contain the "Left Arm", "Right Arm", "Head" and "Lower Torso" nodes in its list of children.
	std::vector<SceneNode*> children;
	
	// The node's position and rotation relative to its parent
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
	glm::vec4 RGBA;
    glm::mat4 modelMatrix;

	// A transformation matrix representing the transformation of the node's location relative to its parent. This matrix is updated every frame.
	glm::mat4 currentTransformationMatrix;

	// The location of the node's reference point
	glm::vec3 referencePoint;

	// The ID of the VAO containing the "appearance" of this SceneNode.
	int vertexArrayObjectID;
	unsigned int VAOIndexCount;

	// Node type is used to determine how to handle the contents of a node
	SceneNodeType nodeType;
    TextureType textureType;
    
    unsigned int texID;
    unsigned int normalTexID;
};


struct PointLight : SceneNode {
    
	PointLight() : SceneNode() {
        intensity = 1.0;
        color = glm::vec3(1, 1, 1);
    }
    
    double intensity;
    glm::vec3 color;
};


struct Mirror : SceneNode {
    
	Mirror() : SceneNode() {
        intensity = 1.0;
        color = glm::vec3(1, 1, 1);
        normal = glm::vec3(1, 0, 0); // Not working
        nodeType = INSTANCED;
    }
    
    double intensity;
    glm::vec3 color;
    glm::vec3 normal;

    float radius;
    float LAN;
    float inclination;
};


SceneNode* createSceneNode();
void addChild(SceneNode* parent, SceneNode* child);
void printNode(SceneNode* node);
int totalChildren(SceneNode* parent);

// For more details, see SceneGraph.cpp.