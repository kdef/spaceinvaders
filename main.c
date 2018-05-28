#include <stdio.h>
#include <GLFW/glfw3.h>

void error_cb(int errno, const char *description) {
	fprintf(stderr, "GLFW Error: %s\n", description);
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

	while (!glfwWindowShouldClose(window)) {
		int width;
		int height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
