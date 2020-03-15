//Erica Chou 03/15/20
//Lunar Landers Code
//Main Function
#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"

#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"

#include "Entity.h"


#include <vector>;

//Hardstuck Variables
#define PLATFORM_COUNT 10
#define SUCCESSBLOCK_COUNT 4
#define FIXED_TIMESTEP .016666f
#define RESET 5

//Entities
struct GameState {
	Entity *player;
	Entity *background;
	Entity *platforms;
	Entity *success;

};

//Globals
GameState state;
GLuint kirbyTextureID;
float LastTicks = 0.0f;
SDL_Window* displayWindow;
bool gameIsRunning = true;
bool start = false;
float accumulator = 0.0f;
float resetTimer = 0;
float accelerationx = 0.25f;


float edgeLeft = -5.00f;
float edgeRight = 5.00f;

float edgeTop = 3.75f;
float edgeBottom = -3.75f;


//Font Set Up and other Globals
GLuint fontTextureID;
glm::vec3 fontPos1 = glm::vec3(-1.5f, 2.0f, 0);
glm::vec3 fontPos2 = glm::vec3(-3.25f,0,0);
glm::vec3 fontPos3 = glm::vec3(-2.75f, 1.0f, 0);


//Vectors to keep Entities in, might change to Entity*
std::vector<Entity> collisionChecksPlayer;
std::vector<Entity> fillIn;


//Shader Program and Model Matrix
ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, modelMatrix2, projectionMatrix;

GLuint LoadTexture(const char* filePath) {//Loads textures 
	int w, h, n;
	unsigned char* image = stbi_load(filePath, &w, &h, &n, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}
	GLuint textureID;
	glGenTextures(1, &textureID);

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	stbi_image_free(image);
	return textureID;
}

//Draw Text Program 
void DrawText(ShaderProgram *program, GLuint fontTextureID, std::string text, float size, float spacing, glm::vec3 position) {
	//Setting up
	float width = 1.0f / 16.0f;
	float height = 1.0f / 16.0f;
	std::vector<float> vertices;
	std::vector<float> texCoords;
	for (int i = 0; i < text.size(); i++) {//For each letter in the string, we create their own verticies and text coordinates
		int index = (int)text[i];
		float offset = (size + spacing) * i;
		float u = (float)(index % 16) / 16.0f;
		float v = (float)(index / 16) / 16.0f;
		vertices.insert(vertices.end(), { offset + (-0.5f * size), 0.5f * size,
			offset + (-0.5f * size), -0.5f * size,
			offset + (0.5f * size), 0.5f * size,
			offset + (0.5f * size), -0.5f * size,
			offset + (0.5f * size), 0.5f * size,
			offset + (-0.5f * size), -0.5f * size, });
		texCoords.insert(texCoords.end(), { u, v,      
			u, v + height,       
			u + width, v,     
			u + width, v + height,   
			u + width, v,      
			u, v + height, 
			}
		);
	} 

	//Rendering text
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, position);
	program->SetModelMatrix(modelMatrix);
	glUseProgram(program->programID);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	
	glBindTexture(GL_TEXTURE_2D, fontTextureID);
	glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));
	
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

void Initialize() {//Initialize the game with variables and other set up
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Lunar Landers!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 640, 480);
	//load shaders for textures
	program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl"); 

	viewMatrix = glm::mat4(1.0f);
	modelMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);
	program.SetColor(1.0f, 0.0f, 0.0f, 1.0f);

	glUseProgram(program.programID);

	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	
	
	fontTextureID = LoadTexture("pixel_font.png");


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//set up Player unit
	state.player = new Entity();
	state.player->position = glm::vec3(0, 3.25f, 0);
	state.player->movement = glm::vec3(0);
	state.player->speed = 1.75f;
	state.player->textureID = LoadTexture("Lunar Landing Box SS.png");

	state.player->entityType = PLAYER;
	state.player->height = 1.0f;
	state.player->width = 1.0f;
	
	//different frames for succeeding and failing
	state.player->animIdle = new int[3]{ 0,1,2 };
	state.player->animFrames = 3;
	state.player->animIndices = state.player->animIdle;
	state.player->animCols = 3;
	state.player->animRows = 1;

	//set up for Background
	state.background = new Entity();
	state.background->textureID = LoadTexture("Sky.png");
	state.background->entityType = BACKGROUND;
	state.background->Update(0, fillIn, 0);
	
	//set up for regular and goal blocks
	state.platforms = new Entity[PLATFORM_COUNT];
	state.success = new Entity[SUCCESSBLOCK_COUNT];

	GLuint platformTextureID = LoadTexture("Grass Block.png");
	GLuint successTextureID = LoadTexture("Grass Block Success.png");

	//positions of the blocks
	//find cleaner way of doing this later
	state.platforms[0].position = glm::vec3(-4.5f, -3.25f, 0.0f);
	state.platforms[1].position = glm::vec3(-3.5f, -3.25f, 0.0f);
	state.platforms[2].position = glm::vec3(-0.5f, -3.25f, 0.0f);
	state.platforms[3].position = glm::vec3(0.5f, -3.25f, 0.0f);
	state.platforms[4].position = glm::vec3(1.5f, -3.25f, 0.0f);
	state.platforms[5].position = glm::vec3(2.5f, -3.25f, 0.0f);
	state.platforms[6].position = glm::vec3(3.5f, -3.25f, 0.0f);
	state.platforms[7].position = glm::vec3(4.5f, -3.25f, 0.0f);
	state.platforms[8].position = glm::vec3(-4.5f, 1.25f, 0.0f);
	state.platforms[9].position = glm::vec3(-3.5f, 1.25f, 0.0f);


	state.success[0].position = glm::vec3(-2.5f, -3.25f, 0.0f);
	state.success[1].position = glm::vec3(-1.5f, -3.25f, 0.0f);
	state.success[2].position = glm::vec3(2.5f, 1.25f, 0.0f);
	state.success[3].position = glm::vec3(3.5f, 1.25f, 0.0f);
	
	//set up for success blocks and pushing in to entity vector for player checking later
	for (int j = 0; j < SUCCESSBLOCK_COUNT; j++) {
		state.success[j].textureID = successTextureID;
		state.success[j].height = 0.8f;
		state.success[j].entityType = SUCCESS_BLOCK;
		state.success[j].Update(0, fillIn, 0); //update once
		collisionChecksPlayer.push_back(state.success[j]);

	}
	
	// set up for regular blocks and pushing into entity vector for collision checking later
	for (int k = 0; k < PLATFORM_COUNT; k++) {
		state.platforms[k].textureID = platformTextureID;
		state.platforms[k].height = 0.8f;
		state.platforms[k].entityType = PLATFORM;
		state.platforms[k].Update(0, fillIn, 0); //update once
		collisionChecksPlayer.push_back(state.platforms[k]);
	}
	
	
}

void ProcessInput() {//takes in inputs from the keyboard and stores values for updates
	SDL_Event event;
	while (SDL_PollEvent(&event)) {

		state.player->movement = glm::vec3(0);

		switch (event.type) {
		case SDL_QUIT:
		case SDL_WINDOWEVENT_CLOSE:
			gameIsRunning = false;
			break;

		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {//start game button
			case SDLK_SPACE:
				start = true;
				state.player->acceleration = glm::vec3(0, -0.25f, 0);
				break;
			}
			
		}
	}



	
	state.player->movement = glm::vec3(0, 0, 0);
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if(start && (!state.player->success || !state.player->fail)){
		if (keys[SDL_SCANCODE_LEFT]) {//If player press left we increase acceleration to left
			state.player->acceleration.x += -accelerationx;

		}
		else if (keys[SDL_SCANCODE_RIGHT]) {//If player presses right, we increase acceleration to right
			state.player->acceleration.x += accelerationx;

		}

		if (glm::length(state.player->movement) > 1.0f) {//normalize vector for even movement
			state.player->movement = glm::normalize(state.player->movement);
		}
	}
}




void Update() {//Updates game logic
	
	//Set up Delta Time
	float tick = (float)SDL_GetTicks() / 1000.f;
	float deltaTime = tick - LastTicks;
	LastTicks = tick;

	//Fixed Time step used to regulate the distance our character moves 
	deltaTime += accumulator;
	if (deltaTime < FIXED_TIMESTEP) {
		accumulator = deltaTime;
		return;
	}
	while (deltaTime >= FIXED_TIMESTEP) {
		state.player->Update(FIXED_TIMESTEP, collisionChecksPlayer, collisionChecksPlayer.size());
		if (state.player->position.x > edgeRight || state.player->position.x < edgeLeft) {
			state.player->fail = true;
		}
		deltaTime -= FIXED_TIMESTEP;
	}
	accumulator = deltaTime;

	//This is the reset button pretty much for when the game is over
	if (state.player->fail || state.player->success) {
		resetTimer += FIXED_TIMESTEP; //why not
		//We let the game sit to let the player read if they succeed or fail & reset the game after
		if (resetTimer >= RESET) {
			resetTimer = 0;
			start = false;
			
			//clean player variables that need cleaning
			state.player->fail = false;
			state.player->success = false;
			state.player->collidedBottom = false;
			state.player->collidedTop = false;
			state.player->collidedLeft = false;
			state.player->collidedRight = false;

			state.player->position = glm::vec3(0, 3.25f, 0);
			state.player->movement = glm::vec3(0);
			state.player->acceleration = glm::vec3(0, 0, 0);
			state.player->velocity = glm::vec3(0, 0, 0);
			state.player->animIndex = 2;
			//clean platform variables
			for (int k = 0; k < collisionChecksPlayer.size(); k++) {
				collisionChecksPlayer[k].collidedBottom = false;
				collisionChecksPlayer[k].collidedTop = false;
				collisionChecksPlayer[k].collidedLeft = false;
				collisionChecksPlayer[k].collidedRight = false;
			}
		}
	}
	
}

void Render() { //renders all the parts of our game
	glClear(GL_COLOR_BUFFER_BIT);
	
	//Render Background
	state.background->Render(&program);
	
	//Render Success Blocks
	for (int i = 0; i < SUCCESSBLOCK_COUNT; i++) {
		state.success[i].Render(&program);
	}
	
	//Render Regular Blocks
	for (int j = 0; j < PLATFORM_COUNT; j++) {
		state.platforms[j].Render(&program);
	}
	
	//would make this a switch case but no time
	//determine what we are drawing to the screen
	if (state.player->fail) {
		state.player->animIndex = 1; 
	}

	else if (state.player->success) {
		state.player->animIndex = 0; 
	}
	else {
		state.player->animIndex = 2;
	}

	//Render Player after determining index
	state.player->Render(&program);
	
	//Different game states need different texts
	if (!start) {
		DrawText(&program, fontTextureID, "Level: 1-1", .2f, 0.1f, fontPos1);
		DrawText(&program, fontTextureID, "Pichu Landers", .3f, 0.1f, fontPos3);
		DrawText(&program, fontTextureID, "Press Space To Start!", .2f, 0.1f, fontPos2);
	}
	else if (state.player->success) {
		DrawText(&program, fontTextureID, "Success!", .3f, 0.1f, fontPos1);
	}
	else if (state.player->fail) {
		DrawText(&program, fontTextureID, "Fail! Try Again", .3f, 0.1f, fontPos1);
	}


	SDL_GL_SwapWindow(displayWindow);


}



void Shutdown() {
	SDL_Quit();
}

int main(int argc, char* argv[]) {
	Initialize();

	while (gameIsRunning) {
		ProcessInput();
		Update();
		Render();
	}

	Shutdown();
	return 0;
}