#include <stdio.h>
#include <GLFW/glfw3.h>

void error_cb(int errno, const char *description) {
	fprintf(stderr, "GLFW Error: %s\n", description);
}

void key_cb(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

int main(int argc, char *argv[]) {
	GLFWwindow *window;

	glfwSetErrorCallback(error_cb);
	if (!glfwInit())
		return -1;

	window = glfwCreateWindow(640, 480, "Space Invaders!", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -2;
	}

	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_cb);

	while (!glfwWindowShouldClose(window)) {
		int width;
		int height;

		glfwGetFramebufferSize(window, &width, &height);

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
