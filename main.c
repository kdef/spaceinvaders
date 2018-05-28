#include <stdlib.h>
#include <stdio.h>
#include <GLFW/glfw3.h>

typedef struct {
	size_t width;
	size_t height;
	uint32_t *data;
} Buffer;

uint32_t rgb_to_uint32(uint8_t r, uint8_t g, uint8_t b) {
	//use rightmost 8bits as alpha, ignored for now
	return (r << 24) | (g << 16) | (b << 8) | 255;
}

void buffer_clear(Buffer *buffer, uint32_t color) {
	size_t i;
	for (i = 0; i < (buffer->width * buffer->height); i++) {
		buffer->data[i] = color;
	}
}

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
	Buffer buffer;

	buffer.width = 640;
	buffer.height = 480;
	buffer.data = malloc(sizeof (uint32_t) * (buffer.width * buffer.height));

	uint32_t clear_color = rgb_to_uint32(0, 255, 0);
	buffer_clear(&buffer, clear_color);

	glfwSetErrorCallback(error_cb);
	if (!glfwInit())
		return -1;

	window = glfwCreateWindow(buffer.width, buffer.height, "Space Invaders!", NULL, NULL);
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
