/*
Copyright (c) 2012-2016 Daniel Burke

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <vulkan/vulkan.h>

#include "vulkan.h"

#include "log.h"
#include "global.h"
#include "text.h"
#include "3dmaths.h"
#include "mesh.h"
#include "vr.h"


long long time_start = 0;

float current_time = 0.0f;

/*
WF_OBJ * bunny;
IMG * img;
GLSLSHADER *shader;
*/

void gfx_init(void);

int main_init(int argc, char *argv[])
{
	time_start = sys_time();

	if( vulkan_init() )
	{
		log_fatal("Vulkan Init: FAILED");
		return 1;
	}
	
//	vr_init();


	log_info("Initialised : OK");
	return 0;   // it worked!
}


void main_end(void)
{
	if(vr_using)
	{
		vr_end();
	}
	vulkan_end();
	log_info("Shutdown    : OK");
}

int p_swim = 0;

// last digit of angle is x-fov, in radians
vec4 pos = {0.0, 0.0, 0.0, 0.0};
vec4 angle = {0.0, 0.0, 0.0, M_PI*0.5};


void render(mat4x4 view, mat4x4 projection, struct MESH_UNIFORM_BUFFER *dest)
{
	mat4x4 model = mat4x4_identity();
	model = mul( model, mat4x4_rot_y(current_time) );		// rotate the bunny
	model = mul( model, mat4x4_translate_float(-0.5, -0.5, -0.5) ); // around it's own origin
	model = mul( mat4x4_translate_float( 0, 0, -2), model );	// move it 2 metres infront of the origin

	model = mul(mat4x4_translate_vect( pos.xyz ), model);	// move to player position
	model = mul(mat4x4_rot_y(angle.y ), model);
	model = mul(mat4x4_rot_x(angle.x ), model);

	dest->modelview = mul( view, model );
	dest->projection = projection;
}


void main_loop(void)
{
	if(keys[KEY_ESCAPE])
	{
		log_info("Shutdown on : Button press (ESC)");
		killme=1;
	}

	if(keys[KEY_F8])
	{	// to test if I'm leaking
		keys[KEY_F8] = 0;
		log_info("Restart Vulkan");
		vulkan_end();
		vulkan_init();
	}


	if(keys[KEY_F9])
	{
		keys[KEY_F9] = 0;
		log_info("VR %s", (vr_using?"Shutdown":"Startup") );
		if(!vr_using)vr_init();
		else vr_end();
	}

	// for first person movement
	pos.w = 0.5 / (float)vid_width;

	vect req = {0,0,0};
	float nice = 0.0002;

	if(keys[KEY_LSHIFT])
		nice *= 10.0;

	if(keys[KEY_ESCAPE])
	{
		killme=1;
	}
	if(keys[KEY_W])
	{
		req.z -= nice;
	}
	if(keys[KEY_S])
	{
		req.z += nice;
	}
	if(keys[KEY_A])
	{
		req.x += nice;
	}
	if(keys[KEY_D])
	{
		req.x -= nice;
	}
	if(keys[KEY_LCONTROL])
	{
		req.y += nice;
	}
	if(keys[KEY_SPACE])
	{
		req.y -= nice;
	}

	if(mouse[2]) /// right mouse
	{
		angle.x -= mickey_y * 0.003;
		angle.y -= mickey_x * 0.003;
	}
	if(keys[KEY_O])
	{
		p_swim = !p_swim;
		keys[KEY_S] = 0;
		log_info("Swimming is %s.", p_swim ? "engaged" : "off");
	}
	if(keys[KEY_P])
	{
		log_info("vec4 pos = {%f, %f, %f, 0.0};", pos.x, pos.y, pos.z);
		log_info("vec4 angle = {%f, %f, %f, M_PI*0.5};", angle.x, angle.y, angle.z);
	}

	if(p_swim)
	{
		float cx = cos(angle.x), sx = sin(angle.x), ty = req.y;
		req.y = req.y * cx - req.z * sx;	// around x
		req.z = ty * sx + req.z * cx;
	}


	float cy = cos(angle.y), sy = sin(angle.y), tx = req.x;
	req.x = req.x * cy + req.z * sy;	// around y
	req.z = tx * sy - req.z * cy;


	pos.xyz = add(pos.xyz, req);



	current_time = (sys_time() - time_start) / (float)sys_ticksecond;

//	if(!vr_using)
	{
		mat4x4 projection = mat4x4_identity();
		projection = mat4x4_perspective(1, 30, 1, (float)vid_height / (float)vid_width);
//		projection = mat4x4_orthographic(0.1, 30, 1, (float)vid_height / (float)vid_width);
		mat4x4 modelview = mat4x4_translate_float(0, 0, 0); // move the camera 1m above ground
		render(modelview, projection, &ubo_eye_left);
		vulkan_loop();
	}
	if(vr_using)
	{
		vr_loop();
	}

}

