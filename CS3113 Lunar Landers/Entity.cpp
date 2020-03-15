//Erica Chou 03/15/20
//Entity.cpp file, sets up functions for various entities used within the game

#include "Entity.h"
Entity::Entity() { //constructor
	position = glm::vec3(0);   
	movement = glm::vec3(0);
	acceleration = glm::vec3(0);
	velocity = glm::vec3(0);
	speed = 0;        
	modelMatrix = glm::mat4(1.0f); 
}

bool Entity::CheckCollision(Entity* other) {
	//checks if object collided with another object and returns a bool
	float xdist = fabs(position.x - other->position.x) - ((width + other->width) / 2.0f);
	float ydist = fabs(position.y - other->position.y) - ((height + other->height) / 2.0f);

	if (xdist < 0 && ydist < 0) { 
		
		return true;
	}

	return false;
}

void Entity::CheckCollisionsY(std::vector<Entity> objects, int objectCount) { 
	//Fixs entity position if we do have a collision in the y axis
	float penetrationY = 0;
	for (int i = 0; i < objectCount; i++) { 
		Entity *object = &objects[i];     
		if (CheckCollision(object)) {
			float ydist = fabs(position.y - object->position.y);   //for figuring out the difference of the penetration
			float temp = fabs(ydist - (height / 2.0f) - (object->height / 2.0f));
			if (penetrationY < temp) {
				penetrationY = temp;
			}
			if (velocity.y > 0) {//collision between an object occured on its top
				position.y -= penetrationY;              
				velocity.y = 0; 
				collidedTop = true;
				if (entityType == PLAYER) {
					fail = true;
				}
			}
			else if (velocity.y < 0) { //collision happened on bottom
				collidedBottom = true;
				object->collidedTop = true;//for future checks
				if (entityType == PLAYER && object->entityType == PLATFORM) {
					fail = true;
				}
				if (entityType == PLAYER && object->entityType == SUCCESS_BLOCK) {
					success = true;

				}
			}
		}
	}
	if(success && fail){//tiebreaker logic if player lands on both a success and reg block
		Entity* platform=NULL;
		Entity* splatform=NULL;
		for (int j = 0; j < objectCount; j++) {
			Entity* temp = &objects[j];
			if (temp->collidedTop) {
				if(temp->entityType == PLATFORM){
					platform = temp;
				}
				else if (temp->entityType == SUCCESS_BLOCK) {
					splatform = temp;
				}
			}
		}
		//grabs the distance from success block and regular block and compares the distance from both blocks
		float xdistPlat = fabs(position.x - platform->position.x);
		float xdistSucc = fabs(position.x - splatform->position.x);
		//the smaller distance wins
		if (xdistPlat <= xdistSucc) {
			fail = true;
			success = false;
		}
		else {
			success = true;
			fail = false;
		}
	}
	if (collidedBottom) {//updates at the end of all the checks
		position.y += penetrationY;
		velocity.y = 0;
	}
}

void Entity::CheckCollisionsX(std::vector<Entity> objects, int objectCount) {
	//fixes position of object from the x-axis
	for (int i = 0; i < objectCount; i++) { 
		Entity *object = &objects[i];    
		if (CheckCollision(object)) {
			float xdist = fabs(position.x - object->position.x);     
			float penetrationX = fabs(xdist - (width / 2.0f) - (object->width / 2.0f));        
			if (velocity.x > 0) { //collision to the right
				//position.x -= penetrationX;       
				velocity.x = 0; 
				collidedRight = true;
				if (entityType == PLAYER && object->entityType == PLATFORM ){
					fail = true;
				}
				else if (entityType == PLAYER && object->entityType == SUCCESS_BLOCK) {
					fail = true;
				}
			} 
			else if (velocity.x < 0) { //collision to the left
				//position.x += penetrationX;         
				velocity.x = 0; 
				collidedLeft = true;
				if (entityType == PLAYER && object->entityType == PLATFORM) {
					fail = true;
				}
				else if (entityType == PLAYER && object->entityType == SUCCESS_BLOCK) {
					fail = true;
				}
			} 
		}
	}
}

void Entity::Update(float deltaTime, std::vector<Entity>platforms, int platformCount) { 
	//updates entity
	if (success || fail) { 
		//end of game if success or fail so we do not want to update positions and such
		return;
	}

	if (entityType == BACKGROUND) {//we only need this to happen to background to make this bigger
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::scale(modelMatrix, glm::vec3(10.0f, 7.5f, 0.0f));
		return;
	}
	if (entityType == PLAYER) {
		//movement logic
		velocity.x = movement.x*speed;
		velocity += acceleration * deltaTime;

		position.y += velocity.y*deltaTime;
		CheckCollisionsY(platforms, platformCount);

		position.x += velocity.x * deltaTime;
		CheckCollisionsX(platforms, platformCount);
	}
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, position); 
	
}

void Entity::DrawSpriteFromTextureAtlas(ShaderProgram *program, GLuint textureID, int index) {
	//draws sprite from a sprite sheet
	float u = (float)(index % animCols) / (float)animCols;
	float v = (float)(index / animCols) / (float)animRows;
	
	float width = 1.0f / (float)animCols;
	float height = 1.0f / (float)animRows;
	
	float texCoords[] = { u, v + height, u + width, v + height, u + width, v, u, v + height, u + width, v, u, v };
	float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
	
	glBindTexture(GL_TEXTURE_2D, textureID);
	
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	
	glDrawArrays(GL_TRIANGLES, 0, 6);
	
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

void Entity::Render(ShaderProgram* program) { 
	//renders an entity
	program->SetModelMatrix(modelMatrix);    
	if (animIndices != NULL) {
			DrawSpriteFromTextureAtlas(program, textureID, animIndices[animIndex]);
			return;
	}
	float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };   
	float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };    
	glBindTexture(GL_TEXTURE_2D, textureID);  

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);  
	glEnableVertexAttribArray(program->positionAttribute);   

	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);  
	glEnableVertexAttribArray(program->texCoordAttribute);      

	glDrawArrays(GL_TRIANGLES, 0, 6);     

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute); 
}


