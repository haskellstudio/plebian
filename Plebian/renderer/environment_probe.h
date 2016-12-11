#ifndef ENVIRONMENT_PROBE_H
#define ENVIRONMENT_PROBE_H

#include <glm/vec3.hpp>
#include "texture.h"
#include "mesh_renderer.h"
#include "camera.h"

struct EnvironmentProbe {
	glm::vec3 pos;
	Texture* cubemap;
};

#endif