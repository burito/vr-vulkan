/*
Copyright (c) 2013 Daniel Burke

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

#include "3dmaths.h"
#include "image.h"
#include <vulkan/vulkan.h>
#include "vulkan.h"


typedef struct WF_MTL
{
	float Ns, Ni;		// specular coefficient, ?
	vect Ka, Kd, Ks, Ke;	// ambient, diffuse, specular, emit
	float4 colour;		// colour + alpha
	IMG *map_Ka, *map_Kd, *map_d, *map_bump;	// amb, spec, alpha, bump
	char *name;
	struct WF_MTL *next;
	int nf;
} WF_MTL;

typedef struct WF_FACE
{
	int3 f, t, n;
	vect normal;
	int s, g;
	WF_MTL *m;
} WF_FACE;


typedef struct WF_OBJ
{
	WF_MTL *m;
	WF_FACE *f;
	int3 *vf;
	int nv, nn, nt, nf, ng, ns, nm;
	vect *v, *vt, *vn, *fn;
	coord *uv;
	float *p;
	char *filename;
	char **groups;
	int *sgroups;
	struct VULKAN_BUFFER vertex_buffer, index_buffer;
	VkIndexType index_type;
//	uint32_t va, ab, eb;
	void (*draw)(struct WF_OBJ*);
} WF_OBJ;


typedef struct WF_ARRAY
{
	vect v, n;
	coord t;
} WF_ARRAY;

void mtl_begin(WF_MTL *m);
void mtl_end(void);

WF_OBJ* wf_load(char *filename);
void wf_free(WF_OBJ *w);

void wf_gpu_load(WF_OBJ *w);
void wf_gpu_unload(WF_OBJ *w);



