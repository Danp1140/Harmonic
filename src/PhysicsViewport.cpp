#include "PhysicsViewport.h"
#include "PhysicsHelper.h"


PhysicsViewport::PhysicsViewport(GLFWwindow *window, int width, int height, Camera *camera, GLuint shader) : Viewport(window, width, height, camera, shader)
{
	simFrame = 0;
	update_state = CONTINUOUS;
}

PhysicsViewport::PhysicsViewport(GLFWwindow *window, int width, int height, GLuint shader) : Viewport(window, width, height, shader)
{
	simFrame = 0;
	update_state = CONTINUOUS;
}

PhysicsViewport::~PhysicsViewport()
{

}

void PhysicsViewport::addGlobalForce(glm::vec3 force)
{
	globalForces.push_back(force);
}

PhysicsObject* PhysicsViewport::addPhysicsObject(const char* materialName) {
	auto *o = new PhysicsObject(mainCamera, getMaterial(materialName), infoShader);
	objects.push_back(o);
	return o;
}

void PhysicsViewport::updatePhysics()
{
	if(update_state==WAIT){
		return;
	}else if(update_state==SINGLE_FRAME){
		update_state=WAIT;
	}
	//Else must be equal to CONTINUOUS
	collisions.clear();
	std::vector<PhysicsObject *> physicsObjects;
	for (auto &object : objects) {
		//Filter master object list by Object type
		if (strcmp(typeid(*object).name(), "PhysicsObjectclass"))
		{
			physicsObjects.push_back((PhysicsObject *) object);
		}
	}
	for(auto &physicsObject : physicsObjects) {
		physicsObject->updateForces(*deltaTime, globalForces);
	}
	Collision *temp;
	//TODO: Make sure this doesn't run when there's less than 2 objects
	//TODO: do spatial build optimizations
	for (int i = 0; i < physicsObjects.size()-1; ++i) {
		for (int j = i+1; j < physicsObjects.size(); ++j) {
			//temp = physicsObjects[j]->updateCollisions(physicsObjects[i]);
			temp = PhysicsHelper::gjk_intersection(physicsObjects[j], physicsObjects[i], window, false);
			if(temp != nullptr) {
				collisions.push_back(temp);
				collide(temp);
			}
		}
	}

	simFrame++;
}

long PhysicsViewport::getSimFrame() const {	return simFrame; }

void PhysicsViewport::collide(Collision *collision) {

}
