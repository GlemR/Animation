#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<stb/stb_image.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>

#include"Texture.h"
#include"shaderClass.h"
#include"VAO.h"
#include"VBO.h"
#include"EBO.h"
#include"Camera.h"
#include"Mesh.h"
#include"ModelLoader.h"
#include"AABB.h"



const unsigned int width = 1920;
const unsigned int height = 1080;
bool enableCollision = true;
bool showAABBs = false;
bool fleshlight = true; // Toggle for fleshlight effect
float fov = 70.0f; // Field of view for the camera

// Nathan animation variables
float nathanWalkSpeed = 2.0f; // Units per second
glm::vec3 nathanStartPos = glm::vec3(3.6f, 1.0f, -45.8f);
glm::vec3 nathanEndPos = glm::vec3(29.6f, 1.0f, -45.8f);
glm::vec3 nathanCurrentPos = nathanStartPos;
bool nathanMovingToEnd = true; // true = moving to end position, false = moving to start
float nathanLastTime = 0.0f;

// Vertices coordinates
Vertex vertices[] =
{ //               COORDINATES           /            COLORS          /           NORMALS         /       TEXTURE COORDINATES    //
	Vertex{glm::vec3(-1.0f, 0.0f,  1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(0.0f, 0.0f)},
	Vertex{glm::vec3(-1.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(0.0f, 1.0f)},
	Vertex{glm::vec3(1.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 1.0f)},
	Vertex{glm::vec3(1.0f, 0.0f,  1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 0.0f)}
};

// Indices for vertices order
GLuint indices[] =
{
	0, 1, 2,
	0, 2, 3
};

Vertex lightVertices[] =
{ //     COORDINATES     //
	Vertex{glm::vec3(-0.1f, -0.1f,  0.1f)},
	Vertex{glm::vec3(-0.1f, -0.1f, -0.1f)},
	Vertex{glm::vec3(0.1f, -0.1f, -0.1f)},
	Vertex{glm::vec3(0.1f, -0.1f,  0.1f)},
	Vertex{glm::vec3(-0.1f,  0.1f,  0.1f)},
	Vertex{glm::vec3(-0.1f,  0.1f, -0.1f)},
	Vertex{glm::vec3(0.1f,  0.1f, -0.1f)},
	Vertex{glm::vec3(0.1f,  0.1f,  0.1f)}
};

GLuint lightIndices[] =
{
	0, 1, 2,
	0, 2, 3,
	0, 4, 7,
	0, 7, 3,
	3, 7, 6,
	3, 6, 2,
	2, 6, 5,
	2, 5, 1,
	1, 5, 4,
	1, 4, 0,
	4, 5, 6,
	4, 6, 7
};

// Function to update Nathan's position
void updateNathanPosition(float currentTime) {
	float deltaTime = currentTime - nathanLastTime;
	nathanLastTime = currentTime;

	if (nathanMovingToEnd) {
		// Move towards end position
		glm::vec3 direction = glm::normalize(nathanEndPos - nathanCurrentPos);
		nathanCurrentPos += direction * nathanWalkSpeed * deltaTime;

		// Check if we've reached or passed the end position
		if (glm::distance(nathanCurrentPos, nathanEndPos) < 0.1f) {
			nathanCurrentPos = nathanEndPos;
			nathanMovingToEnd = false;
		}
	}
	else {
		// Move towards start position
		glm::vec3 direction = glm::normalize(nathanStartPos - nathanCurrentPos);
		nathanCurrentPos += direction * nathanWalkSpeed * deltaTime;

		// Check if we've reached or passed the start position
		if (glm::distance(nathanCurrentPos, nathanStartPos) < 0.1f) {
			nathanCurrentPos = nathanStartPos;
			nathanMovingToEnd = true;
		}
	}
}

int main()
{
	// Initialize GLFW
	glfwInit();

	// Tell GLFW what version of OpenGL we are using 
	// In this case we are using OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Tell GLFW we are using the CORE profile
	// So that means we only have the modern functions
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create a GLFWwindow object of 800 by 800 pixels, naming it "YoutubeOpenGL"
	GLFWwindow* window = glfwCreateWindow(width, height, "main", NULL, NULL);
	// Error check if the window fails to create
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	// Introduce the window into the current context
	glfwMakeContextCurrent(window);

	// Load GLAD so it configures OpenGL
	gladLoadGL();
	// REQUEST A HIGHER PRECISION DEPTH BUFFER
	glfwWindowHint(GLFW_DEPTH_BITS, 24);
	// Specify the viewport of OpenGL in the Window
	// In this case the viewport goes from x = 0, y = 0, to x = 800, y = 800
	glViewport(0, 0, width, height);






	// Generates Shader object using shaders default.vert and default.frag
	Shader shaderProgram("src/default.vert", "src/default.frag");

	Model* schoolModel = nullptr;
	try {
		schoolModel = new Model("models/MapSchool.fbx");
		std::cout << "School model loaded successfully!" << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "Failed to load school model: " << e.what() << std::endl;
		// Continue without the model
	}

	Model* nathanModel = nullptr;
	try {
		nathanModel = new Model("models/nathan.fbx");
		std::cout << "Nathan model loaded successfully!" << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "Failed to load nathan model: " << e.what() << std::endl;
		// Continue without the model
	}


	glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	glm::vec4 lightColor2 = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
	glm::vec3 lightPos = glm::vec3(0.0f, 0.5f, 0.5f);
	glm::vec3 lightPos2 = glm::vec3(-1.615f, 3.0f, 2.894f);
	glm::mat4 lightModel = glm::mat4(1.0f);
	glm::mat4 lightModel2 = glm::mat4(1.0f);
	glm::vec3 spotDirection2 = glm::vec3(0.0f, 1.0f, 0.0f); // Example: pointing up
	glm::vec3 lampPos = glm::vec3(-1.615f, 4.4f, 2.894f); // Example position inside your L-shaped room
	glUniform3f(glGetUniformLocation(shaderProgram.ID, "lampPos"), lampPos.x, lampPos.y, lampPos.z);
	lightModel = glm::translate(lightModel, lightPos);
	lightModel2 = glm::translate(lightModel2, lightPos2);


	

	shaderProgram.Activate();
	glUniform4f(glGetUniformLocation(shaderProgram.ID, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
	glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
	glUniform4f(glGetUniformLocation(shaderProgram.ID, "lightColor2"), lightColor2.x, lightColor2.y, lightColor2.z, lightColor2.w);
	glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightPos2"), lightPos2.x, lightPos2.y, lightPos2.z);
	glUniform3f(glGetUniformLocation(shaderProgram.ID, "spotDirection2"), spotDirection2.x, spotDirection2.y, spotDirection2.z);

	// Set up texture uniforms in the shader
	// This tells the shader which texture units to use for each texture type
	glUniform1i(glGetUniformLocation(shaderProgram.ID, "diffuse0"), 0);
	glUniform1i(glGetUniformLocation(shaderProgram.ID, "specular0"), 1);
	glUniform1i(glGetUniformLocation(shaderProgram.ID, "normal0"), 2);


	// School model transformation
	glm::mat4 schoolModelMatrix = glm::mat4(1.0f);
	schoolModelMatrix = glm::translate(schoolModelMatrix, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec3 scaleVec(2.0f, 2.0f, 2.0f);
	schoolModelMatrix = glm::scale(schoolModelMatrix, scaleVec);

	// Initialize Nathan's starting time
	nathanLastTime = static_cast<float>(glfwGetTime());

	// Compute world triangles for each mesh in the school model
	for (auto& mesh : schoolModel->meshes) {
		mesh.ComputeWorldTriangles(schoolModelMatrix);
	}

	// Enables the Depth Buffer
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	//POLYGON OFFSET TO REDUCE Z - FIGHTING
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0f, 1.0f);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);


	// Creates camera object
	Camera camera(width, height, glm::vec3(6.62f, 2.5f, 4.19f));

	static bool prevF1 = false, prevF2 = false, prevF = false;;

	Shader aabbShader("src/aabb.vert", "src/aabb.frag");


	// Main while loop
	while (!glfwWindowShouldClose(window))
	{

		// Get current time for animation
		float currentTime = static_cast<float>(glfwGetTime());

		// Update Nathan's position
		updateNathanPosition(currentTime);

		// Specify the color of the background
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		// Clean the back buffer and depth buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		{
			fov += 0.5f; // Increase FOV
		}
		if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
		{
			fov -= 0.5f; // Increase FOV
		}
		// Handles camera inputs
		camera.Inputs(window, schoolModel->meshes, schoolModelMatrix, enableCollision);
		// Updates and exports the camera matrix to the Vertex Shader
		camera.updateMatrix(fov, 0.1f, 50.0f);

		shaderProgram.Activate();
		// Set spotlight position to camera position
		glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightPos"),
					camera.Position.x, camera.Position.y, camera.Position.z);
		// Set spotlight direction to camera forward vector
		glUniform3f(glGetUniformLocation(shaderProgram.ID, "spotDirection"),
					camera.Orientation.x, camera.Orientation.y, camera.Orientation.z);
		// ---------------------------------------------------------------------
		float timeValue = static_cast<float>(glfwGetTime());
		glUniform1f(glGetUniformLocation(shaderProgram.ID, "time"), timeValue);
		// Handle toggling of collision and AABB visibility

		bool currF1 = glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS;
		bool currF2 = glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS;
		bool currF = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
		if (currF1 && !prevF1) enableCollision = !enableCollision;
		if (currF2 && !prevF2) showAABBs = !showAABBs;
		if (currF && !prevF) fleshlight = !fleshlight;
		prevF1 = currF1; prevF2 = currF2; prevF = currF;
		glUniform1i(glGetUniformLocation(shaderProgram.ID, "isOn"), fleshlight ? 1 : 0);

		std::cout << camera.Position.x << " " << camera.Position.y << " " << camera.Position.z << std::endl;
		// Draw the school model if it loaded successfully
		if (schoolModel != nullptr) {
			// Update model matrix for the school
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram.ID, "model"), 1, GL_FALSE, glm::value_ptr(schoolModelMatrix));

			// Draw the school model using your existing Draw method
			schoolModel->Draw(shaderProgram, camera);
		}

		// Draw the nathan model if it loaded successfully
		if (nathanModel != nullptr) {
			// Create Nathan's model matrix with updated position
			glm::mat4 nathanModelMatrix = glm::mat4(1.0f);
			nathanModelMatrix = glm::translate(nathanModelMatrix, nathanCurrentPos);
			nathanModelMatrix = glm::scale(nathanModelMatrix, glm::vec3(0.0088f, 0.0088f, 0.0088f));

			// Rotate Nathan to face the direction he's walking
			if (nathanMovingToEnd) {
				// Face walking direction (90 degrees)
				nathanModelMatrix = glm::rotate(nathanModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			}
			else {
				// Face walking direction (270 degrees)
				nathanModelMatrix = glm::rotate(nathanModelMatrix, glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			}

			// Update model matrix for nathan
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram.ID, "model"), 1, GL_FALSE, glm::value_ptr(nathanModelMatrix));

			// Draw the school model using your existing Draw method
			nathanModel->Draw(shaderProgram, camera);
		}

		if (showAABBs) {
			for (auto& mesh : schoolModel->meshes) {
				mesh.DrawAABB(schoolModelMatrix, aabbShader, camera);
			}
		}

		// Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		// Take care of all GLFW events
		glfwPollEvents();
	}



	// Delete all the objects we've created
	shaderProgram.Delete();
	aabbShader.Delete();

	// Delete the school model
	if (schoolModel != nullptr) {
		delete schoolModel;
	}

	// Delete nathan model
	if (nathanModel != nullptr) {
		delete nathanModel;
	}

	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	return 0;
}
