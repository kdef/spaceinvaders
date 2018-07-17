#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GAME_MAX_BULLETS 100
#define NUM_ALIEN_SPRITES 6

//http://nicktasios.nl/posts/space-invaders-from-scratch-part-2.html
const char *vertex_shader =
	"#version 330\n"
	"noperspective out vec2 TexCoord;\n"
	"void main(void){\n"
	"	 TexCoord.x = (gl_VertexID == 2)? 2.0: 0.0;\n"
	"	 TexCoord.y = (gl_VertexID == 1)? 2.0: 0.0;\n"
	"	 gl_Position = vec4(2.0 * TexCoord - 1.0, 0.0, 1.0);\n"
	"}\n";

const char* fragment_shader =
	"#version 330\n"
	"uniform sampler2D buffer;\n"
	"noperspective in vec2 TexCoord;\n"
	"out vec3 outColor;\n"
	"void main(void){\n"
	"	 outColor = texture(buffer, TexCoord).rgb;\n"
	"}\n";

int move_dir = 0;
int should_fire = 0; //false

typedef struct Buffer {
	size_t width;
	size_t height;
	uint32_t *data;
} Buffer;

typedef struct Sprite {
	size_t width;
	size_t height;
	uint8_t *data;
} Sprite;

typedef struct SpriteAnimation {
	int should_loop;
	size_t num_frames;
	size_t frame_duration;
	size_t time;
	Sprite **frames;
} SpriteAnimation;

typedef struct Alien {
	size_t x;
	size_t y;
	enum Type {ALIEN_DEAD, ALIEN_A, ALIEN_B, ALIEN_C} type;
} Alien;

typedef struct Bullet {
	size_t x;
	size_t y;
	int dir;
} Bullet;

typedef struct Player {
	size_t x;
	size_t y;
	size_t life;
} Player;

typedef struct Game {
	size_t width;
	size_t height;
	size_t num_aliens;
	size_t num_bullets;
	Alien *aliens;
	Player player;
	Bullet bullets[GAME_MAX_BULLETS];
} Game;


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

void buffer_draw_sprite(Buffer *buffer, Sprite *sprite,
						size_t x, size_t y, uint32_t color) {
	for (size_t xi = 0; xi < sprite->width; xi++) {
		for (size_t yi = 0; yi < sprite->height; yi++) {
			size_t sy = sprite->height - 1 + y - yi;
			size_t sx = x + xi;

			if (sprite->data[yi * sprite->width + xi] &&
				sy < buffer->height && sx < buffer->width) {
				buffer->data[sy * buffer->width + sx] = color;
			}
		}
	}
}

int sprite_overlap_check(Sprite *sprite1, size_t x1, size_t y1,
						 Sprite *sprite2, size_t x2, size_t y2) {
	if (x1 < x2 + sprite2->width && x1 + sprite1->width > x2 &&
		y1 < y2 + sprite2->height && y1 + sprite1->height > y2) {
		return 1;
	}
	return 0;
}

void sprite_create_all(Sprite **aliens, Sprite **alien_death, Sprite **bullet,
					   Sprite **player) {
	static Sprite alien_sprites[NUM_ALIEN_SPRITES];

	alien_sprites[0].width = 8;
	alien_sprites[0].height = 8;
	alien_sprites[0].data = malloc(sizeof(uint8_t) * 64);
	{
		uint8_t tmp[] = {
			0,0,0,1,1,0,0,0, // ...@@...
			0,0,1,1,1,1,0,0, // ..@@@@..
			0,1,1,1,1,1,1,0, // .@@@@@@.
			1,1,0,1,1,0,1,1, // @@.@@.@@
			1,1,1,1,1,1,1,1, // @@@@@@@@
			0,1,0,1,1,0,1,0, // .@.@@.@.
			1,0,0,0,0,0,0,1, // @......@
			0,1,0,0,0,0,1,0  // .@....@.
		};
		memcpy(alien_sprites[0].data, tmp, sizeof(uint8_t) * 64);
	}

	alien_sprites[1].width = 8;
	alien_sprites[1].height = 8;
	alien_sprites[1].data = malloc(sizeof(uint8_t) * 64);
	{
		uint8_t tmp[] = {
			0,0,0,1,1,0,0,0, // ...@@...
			0,0,1,1,1,1,0,0, // ..@@@@..
			0,1,1,1,1,1,1,0, // .@@@@@@.
			1,1,0,1,1,0,1,1, // @@.@@.@@
			1,1,1,1,1,1,1,1, // @@@@@@@@
			0,0,1,0,0,1,0,0, // ..@..@..
			0,1,0,1,1,0,1,0, // .@.@@.@.
			1,0,1,0,0,1,0,1  // @.@..@.@
		};
		memcpy(alien_sprites[1].data, tmp, sizeof(uint8_t) * 64);
	}

	alien_sprites[2].width = 11;
	alien_sprites[2].height = 8;
	alien_sprites[2].data = malloc(sizeof(uint8_t) * 88);
	{
		uint8_t tmp[] = {
			0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
			0,0,0,1,0,0,0,1,0,0,0, // ...@...@...
			0,0,1,1,1,1,1,1,1,0,0, // ..@@@@@@@..
			0,1,1,0,1,1,1,0,1,1,0, // .@@.@@@.@@.
			1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
			1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
			1,0,1,0,0,0,0,0,1,0,1, // @.@.....@.@
			0,0,0,1,1,0,1,1,0,0,0  // ...@@.@@...
		};
		memcpy(alien_sprites[2].data, tmp, sizeof(uint8_t) * 88);
	}

	alien_sprites[3].width = 11;
	alien_sprites[3].height = 8;
	alien_sprites[3].data = malloc(sizeof(uint8_t) * 88);
	{
		uint8_t tmp[] = {
			0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
			1,0,0,1,0,0,0,1,0,0,1, // @..@...@..@
			1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
			1,1,1,0,1,1,1,0,1,1,1, // @@@.@@@.@@@
			1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
			0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
			0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
			0,1,0,0,0,0,0,0,0,1,0  // .@.......@.
		};
		memcpy(alien_sprites[3].data, tmp, sizeof(uint8_t) * 88);
	}

	alien_sprites[4].width = 12;
	alien_sprites[4].height = 8;
	alien_sprites[4].data = malloc(sizeof(uint8_t) * 96);
	{
		uint8_t tmp[] = {
			0,0,0,0,1,1,1,1,0,0,0,0, // ....@@@@....
			0,1,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@@.
			1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
			1,1,1,0,0,1,1,0,0,1,1,1, // @@@..@@..@@@
			1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
			0,0,0,1,1,0,0,1,1,0,0,0, // ...@@..@@...
			0,0,1,1,0,1,1,0,1,1,0,0, // ..@@.@@.@@..
			1,1,0,0,0,0,0,0,0,0,1,1  // @@........@@
		};
		memcpy(alien_sprites[4].data, tmp, sizeof(uint8_t) * 96);
	}


	alien_sprites[5].width = 12;
	alien_sprites[5].height = 8;
	alien_sprites[5].data = malloc(sizeof(uint8_t) * 96);
	{
		uint8_t tmp[] = {
			0,0,0,0,1,1,1,1,0,0,0,0, // ....@@@@....
			0,1,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@@.
			1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
			1,1,1,0,0,1,1,0,0,1,1,1, // @@@..@@..@@@
			1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
			0,0,1,1,1,0,0,1,1,1,0,0, // ..@@@..@@@..
			0,1,1,0,0,1,1,0,0,1,1,0, // .@@..@@..@@.
			0,0,1,1,0,0,0,0,1,1,0,0  // ..@@....@@..
		};
		memcpy(alien_sprites[5].data, tmp, sizeof(uint8_t) * 96);
	}

	static Sprite alien_death_sprite;
	alien_death_sprite.width = 13;
	alien_death_sprite.height = 7;
	alien_death_sprite.data = malloc(sizeof(uint8_t) * 91);
	{
		uint8_t tmp[] = {
			0,1,0,0,1,0,0,0,1,0,0,1,0, // .@..@...@..@.
			0,0,1,0,0,1,0,1,0,0,1,0,0, // ..@..@.@..@..
			0,0,0,1,0,0,0,0,0,1,0,0,0, // ...@.....@...
			1,1,0,0,0,0,0,0,0,0,0,1,1, // @@.........@@
			0,0,0,1,0,0,0,0,0,1,0,0,0, // ...@.....@...
			0,0,1,0,0,1,0,1,0,0,1,0,0, // ..@..@.@..@..
			0,1,0,0,1,0,0,0,1,0,0,1,0  // .@..@...@..@.
		};
		memcpy(alien_death_sprite.data, tmp, sizeof(uint8_t) * 91);
	}

	static Sprite bullet_sprite;
	bullet_sprite.width = 1;
	bullet_sprite.height = 3;
	bullet_sprite.data = malloc(sizeof(uint8_t) * 3);
	{
		uint8_t tmp[] = {
			1, // @
			1, // @
			1  // @
		};
		memcpy(bullet_sprite.data, tmp, sizeof(uint8_t) * 3);
	}

	static Sprite player_sprite;
	player_sprite.width = 11;
	player_sprite.height = 7;
	player_sprite.data = malloc(sizeof(uint8_t) * 77);
	{
		uint8_t tmp[] = {
			0,0,0,0,0,1,0,0,0,0,0, // .....@.....
			0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
			0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
			0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
			1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
			1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
			1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
		};
		memcpy(player_sprite.data, tmp, 77);
	}

	*aliens = alien_sprites;
	*alien_death = &alien_death_sprite;
	*bullet = &bullet_sprite;
	*player = &player_sprite;
}

void error_cb(int errno, const char *description) {
	fprintf(stderr, "GLFW Error: %s\n", description);
}

void key_cb(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	switch (key) {
		case GLFW_KEY_ESCAPE:
			if (action == GLFW_PRESS)
				glfwSetWindowShouldClose(window, GL_TRUE);
			break;
		case GLFW_KEY_RIGHT:
			if (action == GLFW_PRESS) move_dir += 1;
			else if (action == GLFW_RELEASE) move_dir -=1;
			break;
		case GLFW_KEY_LEFT:
			if (action == GLFW_PRESS) move_dir -= 1;
			else if (action == GLFW_RELEASE) move_dir +=1;
			break;
		case GLFW_KEY_SPACE:
			if (action == GLFW_RELEASE) should_fire = 1;
			break;
		default:
			break;
	}
}

void validate_shader(GLuint shader){
	static const unsigned int BUFFER_SIZE = 512;
	char buffer[BUFFER_SIZE];
	GLsizei length = 0;

	glGetShaderInfoLog(shader, BUFFER_SIZE, &length, buffer);

	if(length>0){
		printf("Shader %d compile error: %s\n", shader, buffer);
	}
}

int validate_program(GLuint program){
	static const GLsizei BUFFER_SIZE = 512;
	GLchar buffer[BUFFER_SIZE];
	GLsizei length = 0;

	glGetProgramInfoLog(program, BUFFER_SIZE, &length, buffer);

	if(length>0){
		printf("Program %d link error: %s\n", program, buffer);
		return 0;
	}

	return 1;
}

int main(int argc, char *argv[]) {
	GLFWwindow *window;
	Buffer buffer;

	buffer.width = 224;
	buffer.height = 256;
	buffer.data = malloc(sizeof(uint32_t) * (buffer.width * buffer.height));
	buffer_clear(&buffer, 0);

	Sprite *alien_sprites;
	Sprite *alien_death_sprite;
	Sprite *bullet_sprite;
	Sprite *player_sprite;

	sprite_create_all(&alien_sprites, &alien_death_sprite, &bullet_sprite, &player_sprite);

	SpriteAnimation alien_animations[3];
	
	for (size_t i = 0; i < 3; i++) {
		alien_animations[i].should_loop = 1; //true
		alien_animations[i].num_frames = 2;
		alien_animations[i].frame_duration = 10;
		alien_animations[i].time = 0;

		alien_animations[i].frames = malloc(sizeof(Sprite*) * 2);
		alien_animations[i].frames[0] = &alien_sprites[2 * i];
		alien_animations[i].frames[1] = &alien_sprites[2 * i + 1];
	}

	Game game;
	game.width = buffer.width;
	game.height = buffer.height;
	game.num_aliens = 55;
	game.num_bullets = 0;
	game.aliens = malloc(sizeof(Alien) * game.num_aliens);
	
	game.player.x = 112 - 5;
	game.player.y = 32;
	
	game.player.life = 3;

	for (size_t yi = 0; yi < 5; yi++) {
		for (size_t xi = 0; xi < 11; xi++) {
			game.aliens[yi * 11 + xi].x = 16 * xi + 20;
			game.aliens[yi * 11 + xi].y = 17 * yi + 128;
			game.aliens[yi * 11 + xi].type = (5 - yi) / 2 + 1;
		}
	}

	glfwSetErrorCallback(error_cb);
	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	window = glfwCreateWindow(2 * buffer.width, 2 * buffer.height, "Space Invaders!", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -2;
	}

	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_cb);

	glewExperimental = GL_TRUE;
	if(glewInit() != GLEW_OK)
	{
		fprintf(stderr, "Error initializing GLEW.\n");
		glfwTerminate();
		return -1;
	}

	int glVersion[2] = {-1, 1};
	glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
	glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);
	
	printf("Using OpenGL: %d.%d\n", glVersion[0], glVersion[1]);

	GLuint fullscreen_triangle_vao;
	glGenVertexArrays(1, &fullscreen_triangle_vao);

	GLuint shader_id = glCreateProgram();
	
	//Create vertex shader
	GLuint shader_vp = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(shader_vp, 1, &vertex_shader, 0);
	glCompileShader(shader_vp);
	validate_shader(shader_vp);
	glAttachShader(shader_id, shader_vp);
	glDeleteShader(shader_vp);
	
	//Create fragment shader
	GLuint shader_fp = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(shader_fp, 1, &fragment_shader, 0);
	glCompileShader(shader_fp);
	validate_shader(shader_fp);
	glAttachShader(shader_id, shader_fp);
	glDeleteShader(shader_fp);
	
	glLinkProgram(shader_id);
	if (!validate_program(shader_id)) {
		fprintf(stderr, "Error while validating shader.\n");
		glfwTerminate();
		glDeleteVertexArrays(1, &fullscreen_triangle_vao);
		free(buffer.data);
		return -3;
	}

	glUseProgram(shader_id);

	// start drawing
	uint32_t clear_color = rgb_to_uint32(0, 255, 0);
	buffer_clear(&buffer, 0);

	uint32_t sprite_color = rgb_to_uint32(255, 0, 0);

	GLuint buffer_texture;
	glGenTextures(1, &buffer_texture);

	glBindTexture(GL_TEXTURE_2D, buffer_texture);
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGB8,
		buffer.width, buffer.height, 0,
		GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer.data
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLint location = glGetUniformLocation(shader_id, "buffer");
	glUniform1i(location, 0);

	glDisable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(fullscreen_triangle_vao);

	glfwSwapInterval(1);	
	int player_move_dir = 0;

	uint8_t death_counters[game.num_aliens];
	for (size_t i = 0; i < game.num_aliens; i++) {
		death_counters[i] = 10;
	}

	while (!glfwWindowShouldClose(window)) {
		buffer_clear(&buffer, clear_color);

		Alien a;
		for (size_t i = 0; i < game.num_aliens; i++) {
			if (!death_counters[i]) continue; //alien already dead

			a = game.aliens[i];
			if (a.type == ALIEN_DEAD) {
				buffer_draw_sprite(&buffer, alien_death_sprite, a.x, a.y, sprite_color);
			} else {
				SpriteAnimation animation = alien_animations[a.type - 1]; //enums have a underlying int
				size_t current_frame = animation.time / animation.frame_duration;
				buffer_draw_sprite(&buffer, animation.frames[current_frame], a.x, a.y, sprite_color);
			}
		}

		Bullet bullet;
		for (size_t i = 0; i < game.num_bullets; ++i) {
			bullet = game.bullets[i];
			buffer_draw_sprite(&buffer, bullet_sprite, bullet.x, bullet.y, sprite_color);
		}

		buffer_draw_sprite(&buffer, player_sprite, game.player.x, game.player.y, sprite_color);

		glTexSubImage2D(
			GL_TEXTURE_2D, 0, 0, 0,
			buffer.width, buffer.height,
			GL_RGBA, GL_UNSIGNED_INT_8_8_8_8,
			buffer.data
		);

		glDrawArrays(GL_TRIANGLES, 0, 3);
		glfwSwapBuffers(window);

		for (size_t i = 0; i < 3; i++) {
			alien_animations[i].time++;
			if (alien_animations[i].time == alien_animations[i].num_frames * alien_animations[i].frame_duration) {
				alien_animations[i].time = 0;
			}
		}

		for (size_t ai = 0; ai < game.num_aliens; ai++) {
			Alien alien = game.aliens[ai];
			if (alien.type == ALIEN_DEAD && death_counters[ai]) {
				death_counters[ai]--;
			}
		}

		for (size_t i = 0; i < game.num_bullets;) {
			game.bullets[i].y += game.bullets[i].dir;
			Bullet b = game.bullets[i];
			if (b.y >= game.height || b.y < 0) {
				//delete this bullet by overwriting
				game.bullets[i] = game.bullets[game.num_bullets - 1];
				game.num_bullets--;
				continue;
			}

			for (size_t ai = 0; ai < game.num_aliens; ai++) {
				Alien alien = game.aliens[ai];
				if (alien.type == ALIEN_DEAD) continue;

				Sprite s = alien_sprites[(alien.type - 1) * 2];
				int overlap = sprite_overlap_check(bullet_sprite, b.x, b.y,
												   &s, alien.x, alien.y);
				if (overlap) {
					game.aliens[ai].type = ALIEN_DEAD;
					game.bullets[i] = game.bullets[game.num_bullets - 1];
					game.num_bullets--;
					break;
				}
			}

			i++;
		}

		player_move_dir = 2 * move_dir;
		if (player_move_dir != 0) {
			if (game.player.x + player_sprite->width + player_move_dir >= game.width) {
				game.player.x = game.width - player_sprite->width;
			}
			else if ((int)game.player.x + player_move_dir <= 0) {
				game.player.x = 0;
			}
			else game.player.x += player_move_dir;
		}

		if (should_fire && game.num_bullets < GAME_MAX_BULLETS) {
			game.bullets[game.num_bullets].x = game.player.x + player_sprite->width / 2;
			game.bullets[game.num_bullets].y = game.player.y + player_sprite->height;
			game.bullets[game.num_bullets].dir = 2;
			game.num_bullets++;
		}
		should_fire = 0;

		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	glDeleteVertexArrays(1, &fullscreen_triangle_vao);
	free(buffer.data);

	for (size_t i = 0; i < NUM_ALIEN_SPRITES; i++) {
		free(alien_sprites[i].data);
	}
	free(alien_death_sprite->data);
	free(bullet_sprite->data);
	free(player_sprite->data);

	return 0;
}
