# define GLAD_GL_IMPLEMENTATION
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <stdlib.h>
#include <stddef.h>
#include <string>

#include "Shader.h"

static void cleanup()
{
	// terminate GLFW
	glfwTerminate();
	std::cout << "Terminalte GLFW" << std::endl;
}

void error_callback(int error, const char* description)
{
	std::cerr << "GLFW Error: " << error << description << std::endl;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

/**
 * @param shader	The ID of shader object to which the source is assigned
 * @param filePath	The path to the shade source file.
 * @return true		If the file was successfully reda and registered
 * @return false	If the file could not be opend.
*/
//bool setShaderSourceFromFile(GLuint shader, const std::string& filePath)
//{
//	std::ifstream file(filePath); // open file
//	if (!file.is_open())
//	{
//		std::cerr << "Error: could not open shader file" << std::endl;
//		return false;
//	}
//
//	// Load file contents
//	std::stringstream buffer;
//	buffer << file.rdbuf();
//	std::string sourceCode = buffer.str();
//
//	// set pointer for OpenGL
//	const GLchar* sourcePtr = sourceCode.c_str();
//
//	// excute registering to OpenGL
//	glShaderSource(shader, 1, &sourcePtr, nullptr);
//
//	return true;
//}


int main(void)
{
	if (!glfwInit()) return 1;

	atexit(&cleanup);
	glfwSetErrorCallback(error_callback);

	// set GLFW version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* const window(glfwCreateWindow(800, 600, "title", NULL, NULL));
	if (window == NULL)
	{
		std::cout << "Failed to initialize GLFW window" << std::endl;
		return -1;
	}
	glfwMakeContextCurrent(window); // select a window to be drawn on 

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	//glfwSetWindowPos(window, 640, 100);
	glViewport(0, 0, 800, 600);

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

	Shader ourShader("shader.vert", "shader.frag");

	// set vertices positions
	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f
	};

	unsigned int indices[] = {
		0, 1, 2
	};

	unsigned int VBO, VAO, IBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &IBO);
	// bind the Vertex Array Object first. then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	int stride = 6 * sizeof(float);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	glfwSetKeyCallback(window, key_callback);


	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT); // paint background color seted by function glClearColor()

		ourShader.use();
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
		
		glfwSwapBuffers(window); // change a buffer moniterd now and a buffer being painted in back.

		glfwPollEvents(); //  chack event queue. if there are any stack, process stock events. else, skip
		//glfwWaitEvents(); // if detect any events, process next loop. ex) move a mouse, press the close wedget and so on
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

}