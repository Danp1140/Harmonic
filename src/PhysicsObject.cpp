/*
 * PhysicsObject.cpp
 *
 *  Created on: Nov 2, 2017
 *      Author: josephpaavola
 */

#include "PhysicsObject.h"

#include <utility>

PhysicsObject::PhysicsObject(Camera *camera, Material *material, GLuint shader) : Object(camera, material){
	mass = 1.0;
	isPinned = false;
	colliding = false;
	obbTree = nullptr;
	this->shader = shader;
	drawCenter = false;
	drawBBBasis = false;
}

PhysicsObject::~PhysicsObject() {
	delete(obbTree);
}

Collision* PhysicsObject::updateCollisions(PhysicsObject *otherObject) {
	//TODO: change otherObject name to be less terrible
	//Get all 15 axis of separation
	//TODO: Make this an array for conservation
	std::vector<glm::vec3> L;
	L.push_back(obbTree->getTrueAxis(0));
	L.push_back(obbTree->getTrueAxis(1));
	L.push_back(obbTree->getTrueAxis(2));
	L.push_back(otherObject->obbTree->getTrueAxis(0));
	L.push_back(otherObject->obbTree->getTrueAxis(1));
	L.push_back(otherObject->obbTree->getTrueAxis(2));
	L.push_back(glm::cross(obbTree->getTrueAxis(0), otherObject->obbTree->getTrueAxis(0)));
	L.push_back(glm::cross(obbTree->getTrueAxis(0), otherObject->obbTree->getTrueAxis(1)));
	L.push_back(glm::cross(obbTree->getTrueAxis(0), otherObject->obbTree->getTrueAxis(2)));
	L.push_back(glm::cross(obbTree->getTrueAxis(1), otherObject->obbTree->getTrueAxis(0)));
	L.push_back(glm::cross(obbTree->getTrueAxis(1), otherObject->obbTree->getTrueAxis(1)));
	L.push_back(glm::cross(obbTree->getTrueAxis(1), otherObject->obbTree->getTrueAxis(2)));
	L.push_back(glm::cross(obbTree->getTrueAxis(2), otherObject->obbTree->getTrueAxis(0)));
	L.push_back(glm::cross(obbTree->getTrueAxis(2), otherObject->obbTree->getTrueAxis(1)));
	L.push_back(glm::cross(obbTree->getTrueAxis(2), otherObject->obbTree->getTrueAxis(2)));

	glm::vec3 A = obbTree->getCenter();
	glm::vec3 B = otherObject->obbTree->getCenter();
	//Half dimensions
	size_t index_max_a=0, index_max_b=0;
	glm::vec3 a = B - this->obbTree->getExtrema(0);
	glm::vec3 b = A - otherObject->obbTree->getExtrema(0);
	for (size_t j = 1; j < 8; ++j) {
		if(glm::min(a, glm::abs(B - this->obbTree->getExtrema(j))) != a) {
			index_max_a = j;
		}
		if(glm::min(b, glm::abs(A - otherObject->obbTree->getExtrema(j))) != b) {
			index_max_b = j;
		}
		a = glm::min(a, glm::abs(B - this->obbTree->getExtrema(j)));
		b = glm::min(b, glm::abs(A - otherObject->obbTree->getExtrema(j)));
	}
	a = glm::abs(A - this->obbTree->getExtrema(index_max_a));
	b = glm::abs(B - otherObject->obbTree->getExtrema(index_max_b));

	//Initialize radii
	//glm::fvec3 r_a = glm::vec3(0);
	//glm::fvec3 r_b = glm::vec3(0);
	float r_a, r_b;
	//TODO vvv ake sure the signs on this are ok vvv
	glm::vec3 T = B - A;
	bool disjoint = true;
	float radii=0 , distance=0;  //debugging variables

	for (int i = 0; i < 15; ++i) {
		//Find r_a and r_b
		//TODO: figure out how to get rid of these pseudo errors
		r_a = glm::length(glm::dot(a, L[i]));
		r_b = glm::length(glm::dot(b, L[i]));
		distance = glm::length(glm::dot(T, L[i]));
		radii = r_a + r_b;
		//std::cout << "\n=============\ni: " << i << std::endl << "L: " << glm::to_string(L[i]) << std::endl << "Radii: " << radii << std::endl << "Distance: " << distance << std::endl;
		if(distance > radii) {
			disjoint = false;
			break;
		}
	}
	if(!disjoint) {
		colliding = true;
		return new Collision(this, otherObject);
	}else {
		colliding = false;
		return nullptr;
	}

}

void PhysicsObject::applyForce(double deltaT, glm::vec3 force)
{
	double p_x, p_y, p_z, v_x, v_y, v_z;

	transform.acceleration.x = force.x / mass;
	transform.acceleration.y = force.y / mass;
	transform.acceleration.z = force.z / mass;

	v_x = transform.velocity.x + (deltaT * transform.acceleration.x);
	v_y = transform.velocity.y + (deltaT * transform.acceleration.y);
	v_z = transform.velocity.z + (deltaT * transform.acceleration.z);

	p_x = transform.position.x + (transform.velocity.x*deltaT) + (0.5*transform.acceleration.x*deltaT*deltaT);
	p_y = transform.position.y + (transform.velocity.y*deltaT) + (0.5*transform.acceleration.y*deltaT*deltaT);
	p_z = transform.position.z + (transform.velocity.z*deltaT) + (0.5*transform.acceleration.z*deltaT*deltaT);

	transform.velocity = glm::vec3(v_x, v_y, v_z);
	transform.position = glm::vec3(p_x, p_y, p_z);
}

void PhysicsObject::draw() {
	Object::draw();

	// Update and Draw AABB
	//aabb.draw(infoShader, vertices, this->transform, this->m_cam);
	obbTree->draw(shader, this->transform, this->m_cam);
}

void PhysicsObject::setObjectData(const char *objPath) {
	Object::setObjectData(objPath);
	obbTree = new OBBTree(&vertices, &transform);
}

void PhysicsObject::updateForces(double deltaT, std::vector<glm::vec3> globalForces) {
	//Stop update if object doesn't move
	if (isPinned) { return; }
	//Combine global and local forces
	std::vector<glm::vec3> forces;
	forces.push_back(combineForces(std::move(globalForces)));
	forces.push_back(combineForces(transform.localForces));
	//Apply forces
	applyForce(deltaT, combineForces(forces));
}



