/*
Copyright (c) 2012 Daniel Burke

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
#include <vulkan/vulkan.h>

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "3dmaths.h"
#include "mesh.h"
#include "text.h"
#include "log.h"
#include "vulkan.h"
#include "vulkan_helper.h"

#define BUF_LEN 1024

void mtl_begin(WF_MTL *m)
{
//	GLfloat spec[] = {m->Ks.x, m->Ks.y, m->Ks.z, 1.0f};
//	GLfloat amb[] = {m->Ka.x, m->Ka.y, m->Ka.z, 1.0f};
//	GLfloat diff[] = {m->Kd.x, m->Kd.y, m->Kd.z, 1.0f};
//	GLfloat emit[] = {m->Ke.x, m->Ke.y, m->Ke.z, 1.0f};
//	glMateriali(GL_FRONT, GL_SHININESS, m->Ns);
//	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
//	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
//	glMaterialfv(GL_FRONT, GL_DIFFUSE, diff);
//	glMaterialfv(GL_FRONT, GL_EMISSION, emit);
//	glColor4fv(diff);
//	glColor4f(m->colour.x, m->colour.y, m->colour.z, m->colour.w);
	
/*
	glActiveTexture(GL_TEXTURE0);
	if(m->map_Kd)
	{	glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, m->map_Kd->id); }
	else
	{	glDisable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, 0); }

	if(m->map_d)
	{
		glActiveTexture(GL_TEXTURE2);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m->map_d->id);
	}
	if(m->map_bump)
	{
//		glActiveTexture(GL_TEXTURE3);
//		glEnable(GL_TEXTURE_2D);
//		glBindTexture(GL_TEXTURE_2D, m->map_bump->id);

	}
	glActiveTexture(GL_TEXTURE0);
*/
}
void mtl_end(void)
{
/*
	GLfloat amb[] = {0.2f, 0.2f, 0.2f, 1.0f};
	GLfloat diff[] = {0.8f, 0.8f, 0.8f, 1.0f};
	GLfloat zero[] = {0.0f, 0.0f, 0.0f, 1.0f};
#ifndef __APPLE__
	glMateriali(GL_FRONT, GL_SHININESS, 0);
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, diff);
	glMaterialfv(GL_FRONT, GL_SPECULAR, zero);
	glMaterialfv(GL_FRONT, GL_EMISSION, zero);

	glColor4f(1,1,1,1);
	glMateriali(GL_FRONT, GL_SHININESS, 0);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE2);
	glDisable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE3);
	glDisable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
#endif
*/
}

void mtl_free(WF_MTL *m)
{
	if(!m)return;
	if(m->name)free(m->name);
	if(m->map_Ka)img_free(m->map_Ka);
	if(m->map_Kd)img_free(m->map_Kd);
	if(m->map_d)img_free(m->map_d);
	if(m->map_bump)img_free(m->map_bump);
	free(m);

}

static WF_MTL* mtl_newmtl(char *hostpath, FILE *fptr, char *name)
{
	WF_MTL *m = malloc(sizeof(WF_MTL));
	if(!m)return 0;
	memset(m, 0, sizeof(WF_MTL));
	m->colour.x = m->colour.y = m->colour.z = m->colour.w = 1.0f;
	m->Ns = 50.0f;							// specular coefficient
	m->Ka.x = m->Ka.y = m->Ka.z = 0.2f;		// ambient
	m->Kd.x = m->Kd.y = m->Kd.z = 0.8f;		// diffuse
	m->Ks.x = m->Ks.y = m->Ks.z = 0.2f;		// specular
	m->name = hcopy(name);
	char buf[BUF_LEN];
	float *targetf;
	vect *targetf3;
	IMG **targeti;
	char *path;

	int i=0;

	long last = ftell(fptr);

	while(fgets(buf, BUF_LEN, fptr))
	{
		tailchomp(buf);
		if(strlen(buf) < 3)break;
		for(i = 0; whitespace(buf[i]); i++);

		switch(buf[i]) {
		case 'N':
			switch(buf[i+1]) {
			default:
			case 's': targetf = &m->Ns; break;		// specular coefficient
			case 'i': targetf = &m->Ni; break;		// ?
			}
			*targetf = fast_atof(buf+i+3);
			break;
		case 'K':
			targetf3 = 0;
			switch(buf[i+1]) {
			case 'a': targetf3 = &m->Ka; break;		// ambient
			case 'd': targetf3 = &m->Kd; break;		// diffuse
			case 's': targetf3 = &m->Ks; break;		// specular
			case 'e': targetf3 = &m->Ke; break;		// emissive
			}
			if(!targetf3)break;
			i+=2;
			sscanf(buf+i, "%f %f %f", &targetf3->x, &targetf3->y, &targetf3->z);
			break;
		case 'm':
			switch(buf[i+5]) {
			case 'a': targeti = NULL; break; //targeti = &m->map_Ka; break;		// ambient
			case 'd': targeti = &m->map_Kd; break;		// diffuse
//			case 'u': targeti = &m->map_bump; break;	// bump
			default:  targeti = NULL; break;
			}
			if(!targeti)break;
			while(!whitespace(buf[i]))i++;i++;
			path = repath(hostpath, buf+i);
			*targeti = img_load(path);
			free(path);
			break;
		case 'd':
			m->colour.w = fast_atof(buf+i+2);		// alpha
			break;
		case 'T':									// colour?
		sscanf(buf+i, "Tf %f %f %f", &m->colour.x, &m->colour.y, &m->colour.z);
			break;
		case 'n':
			fseek(fptr, last, SEEK_SET);
			return m;
		}
		last = ftell(fptr);
	}
	return m;
}


static void mtl_load(WF_OBJ *w, char *filename)
{
	if(!w)return;
	if(!filename)return;
	if(!w->filename)return;
	char *filepath = repath(w->filename, filename);
	log_info("Loading Wavefront MTL(\"%s\");", filepath);
	FILE *fptr = fopen(filepath, "r");
	if(!fptr)
	{
		free(filepath);
		return;
	}

	char buf[BUF_LEN];
	while(fgets(buf, BUF_LEN, fptr))
	if(strstr(buf, "newmtl"))
	{
		tailchomp(buf);
		WF_MTL *m = mtl_newmtl(filepath, fptr, buf+7);
		if(m)
		{
			w->nm++;
			m->next = w->m;
			w->m = m;
		}
	}
	free(filepath);
	fclose(fptr);
}



static WF_MTL* find_material(WF_MTL *m, char *name)
{
	if(!m)return 0;
	if(!name)return 0;
	if(!m->name)return 0;
	while(strcmp(m->name, name))
	{
		if(!m->next)return 0;
		m = m->next;
		if(!m->name)return 0;
	}
	return m;
}


static void wf_bound(WF_OBJ *w)
{
	if(!w)return;
	vect *v = w->v;
	if(!v)return;
	vect min, max, size, mid;
	max = min = v[0];
	for(int i=0; i<w->nv; i++)
	{
		if(v[i].x < min.x)min.x = v[i].x;
		if(v[i].x > max.x)max.x = v[i].x;

		if(v[i].y < min.y)min.y = v[i].y;
		if(v[i].y > max.y)max.y = v[i].y;

		if(v[i].z < min.z)min.z = v[i].z;
		if(v[i].z > max.z)max.z = v[i].z;
	}

	size = sub(max, min);
	float longest = vmax(size);
	mid = sub(longest, size);
	mid = mul(mid, 0.5);
//	longest = longest * (12.0 / 14.0);
	for(int i=0; i<w->nv; i++)
	{
		v[i] = sub(v[i], min);
		v[i] = add(v[i], mid);
		v[i] = mul(v[i], 1.0 / longest);
		v[i] = mul(v[i], (12.0 / 14.0));
		v[i] = add(v[i], (1.0 / 14.0));
	}

	log_debug("Volume = (%f, %f, %f)", size.x, size.y, size.z);
}



void wf_interleave(WF_OBJ *w)
{
	if(!w)return;
	log_debug("At interleave we have %d/%d/%d", w->nv, w->nn, w->nt);
	if(!w->nv)return;
	float *p = malloc(w->nv * 32);
	if(!p)return;
	memset(p, 0, w->nv * 32);
	// interleave the verts, normals and texture coords for the VBO
	for(int i=0; i<w->nv; i++)
	{
		p[i*8+0] = w->v[i].x;
		p[i*8+1] = w->v[i].y;
		p[i*8+2] = w->v[i].z;
		if(w->nv == w->nn)
		{
			p[i*8+3] = w->vn[i].x;
			p[i*8+4] = w->vn[i].y;
			p[i*8+5] = w->vn[i].z;
		}
		if(w->uv)
		{
			p[i*8+6] = w->uv[i].x;
			p[i*8+7] = w->uv[i].y;
		}
		else
		{
			p[i*8+6] = 0.0;
			p[i*8+7] = 0.0;

		}
	}
	log_debug("Interleaved ok!");
	w->p = p;

	// now store the faces, per material for fast rendering
	int3 *f = malloc(w->nf*12);
	WF_MTL *m = w->m;
	int o=0;
	for(;m;m = m->next)
	{
		m->nf = 0;
		for(int i=0; i<w->nf; i++)
		if(w->f[i].m == m)
		{
			f[o++] = w->f[i].f;
			m->nf++;
		}
	}
	
	for(;o < w->nf; o++)
	for(int i=0; i<w->nf; i++)
	if(w->f[i].m == 0)
		f[o++] = w->f[i].f;
	w->vf = f;
	log_debug("Interleaved faces ok!");
//	free(w->v); w->v = 0;
//	free(w->vn); w->vn = 0;
}

static void wf_face_normals(WF_OBJ *w)
{
	if(!w)return;
	if(!w->nf)return;

	vect a, b, t;

	// Generate per face normals
	for(int i=0; i<w->nf; i++)
	{
		a = sub( w->v[w->f[i].f.x], w->v[w->f[i].f.y] );
		b = sub( w->v[w->f[i].f.x], w->v[w->f[i].f.z] );
		t = vect_cross( a, b );
		w->f[i].normal = vect_norm( t );
	}
}


static void wf_vertex_normals(WF_OBJ *w)
{
	if(!w)return;
	if(!w->nf)return;
	if(!w->vn)return;

	typedef struct LLIST
	{
		int face;
		struct LLIST* next;
	} LLIST;

	LLIST *vert = malloc(sizeof(LLIST)*w->nv);
	memset(vert, 0, sizeof(LLIST)*w->nv);

	LLIST *tmp;
	int index;

	// tell each vert about it's faces
	for(int i=0; i<w->nf; i++)
//	if(w->f[i].s == smoothgroup)
	{
		index = w->f[i].f.x;
		tmp = malloc(sizeof(LLIST));
		tmp->face = i;
		tmp->next = vert[index].next;
		vert[index].next = tmp;

		index = w->f[i].f.y;
		tmp = malloc(sizeof(LLIST));
		tmp->face = i;
		tmp->next = vert[index].next;
		vert[index].next = tmp;

		index = w->f[i].f.z;
		tmp = malloc(sizeof(LLIST));
		tmp->face = i;
		tmp->next = vert[index].next;
		vert[index].next = tmp;
	}

	// now average the normals and store in the output
	for(int i=0; i<w->nv; i++)
	{
		tmp = vert[i].next;
		if(!tmp)continue;
		vect t = {0,0,0};
		while(tmp)
		{
			t = add(t, w->f[tmp->face].normal);
			LLIST *last = tmp;
			tmp = tmp->next;
			free(last);
		}

		if((t.x*t.x + t.y*t.y + t.z*t.z )>0.1)
		{
			w->vn[i] = vect_norm(t);
		}

	}
	free(vert);
}



static void wf_normals(WF_OBJ *w)
{
	if(!w)return;
	if(w->nv == w->nn)return;

	if(w->vn)free(w->vn);
	w->vn = malloc(sizeof(vect)*w->nv);
	memset(w->vn, 0, sizeof(vect)*w->nv);
	w->nn = w->nv;
	
	wf_face_normals(w);
	wf_vertex_normals(w);


}

static void wf_texcoords(WF_OBJ *w)
{
	if(!w)return;
	if(!w->nt)return;
	const int size = w->nv * sizeof(coord);
	w->uv = malloc(size);
	memset(w->uv, 0, size);

	int uvcopy = 0;

	for(int i=0; i<w->nf; i++)
	{
		if(mag(w->uv[w->f[i].f.x]) > 0.1)
		{
			uvcopy++;
		}
		w->uv[w->f[i].f.x] = w->vt[w->f[i].t.x].xy;
		w->uv[w->f[i].f.y] = w->vt[w->f[i].t.y].xy;
		w->uv[w->f[i].f.z] = w->vt[w->f[i].t.z].xy;
	}
	log_debug("UV's copied, wanted %d verts", uvcopy);
}


void wf_gpu_load(WF_OBJ *w)
{
	if(!w)return;

	VkResult result;
	struct VULKAN_BUFFER vertex_staging;
	struct VULKAN_BUFFER index_staging;
	struct VULKAN_BUFFER vertex_buffer;
	struct VULKAN_BUFFER index_buffer;
	VkDeviceSize vertex_buffer_size = w->nv * 32;
	VkDeviceSize index_buffer_size = 4;

	vk_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&vertex_staging, vertex_buffer_size, w->p );

	vk_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&index_staging, index_buffer_size, w->p );

	vk_buffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&w->vertex_buffer, vertex_staging.size, NULL);

	vk_buffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&w->index_buffer, index_staging.size, NULL);

	VkCommandBuffer cmdbuf;
	VkCommandBufferAllocateInfo cmdbufallocinf = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,	// VkStructureType         sType;
		NULL,						// const void*             pNext;
		vk.commandpool,					// VkCommandPool           commandPool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,		// VkCommandBufferLevel    level;
		1						// uint32_t                commandBufferCount;
	};
	result = vkAllocateCommandBuffers(vk.device, &cmdbufallocinf, &cmdbuf);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkAllocateCommandBuffers = %s", vulkan_result(result));
//		goto TODO_ERROR_HANDLING;
	}

	VkCommandBufferBeginInfo vk_cmdbegin = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType                          sType;
		NULL,						// const void*                              pNext;
		VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,	// VkCommandBufferUsageFlags                flags;
		NULL						// const VkCommandBufferInheritanceInfo*    pInheritanceInfo;
	};
	result = vkBeginCommandBuffer(cmdbuf, &vk_cmdbegin);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkBeginCommandBuffer = %s", vulkan_result(result));
//		goto TODO_ERROR_HANDLING;
	}

	VkBufferCopy region = {
		0,	// VkDeviceSize    srcOffset;
		0,	// VkDeviceSize    dstOffset;
		0	// VkDeviceSize    size;
	};
	region.size = vertex_staging.size;
	vkCmdCopyBuffer(cmdbuf, vertex_staging.buffer, w->vertex_buffer.buffer, 1, &region);
	region.size = index_staging.size;
	vkCmdCopyBuffer(cmdbuf, index_staging.buffer, w->index_buffer.buffer, 1, &region);
	result = vkEndCommandBuffer(cmdbuf);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkEndCommandBuffer = %s", vulkan_result(result));
//		goto TODO_ERROR_HANDLING;
	}

	VkPipelineStageFlags vkflags = VK_PIPELINE_STAGE_TRANSFER_BIT;
	VkSubmitInfo submit_info = {
		VK_STRUCTURE_TYPE_SUBMIT_INFO,		// VkStructureType                sType;
		NULL,					// const void*                    pNext;
		0,					// uint32_t                       waitSemaphoreCount;
		NULL,					// const VkSemaphore*             pWaitSemaphores;
		&vkflags,				// const VkPipelineStageFlags*    pWaitDstStageMask;
		1,					// uint32_t                       commandBufferCount;
		&cmdbuf,				// const VkCommandBuffer*         pCommandBuffers;
		0,					// uint32_t                       signalSemaphoreCount;
		NULL					// const VkSemaphore*             pSignalSemaphores;
	};
	result = vkQueueSubmit(vk.queue, 1, &submit_info, VK_NULL_HANDLE);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkQueueSubmit = %s", vulkan_result(result));
//		goto TODO_ERROR_HANDLING;
	}
	result = vkQueueWaitIdle(vk.queue);
	if( result != VK_SUCCESS )
	{
		log_fatal("vkQueueWaitIdle = %s", vulkan_result(result));
//		goto TODO_ERROR_HANDLING;
	}

	vkFreeCommandBuffers(vk.device, vk.commandpool, 1, &cmdbuf);
	vkFreeMemory(vk.device, index_staging.memory, NULL);
	vkDestroyBuffer(vk.device, index_staging.buffer, NULL);
	vkFreeMemory(vk.device, vertex_staging.memory, NULL);
	vkDestroyBuffer(vk.device, vertex_staging.buffer, NULL);
}

void wf_gpu_unload(WF_OBJ *w)
{
	vkFreeMemory(vk.device, w->index_buffer.memory, NULL);
	vkDestroyBuffer(vk.device, w->index_buffer.buffer, NULL);
	vkFreeMemory(vk.device, w->vertex_buffer.memory, NULL);
	vkDestroyBuffer(vk.device, w->vertex_buffer.buffer, NULL);
}

void wf_draw(WF_OBJ *w)
{
	if(!w)return;
//	glBindVertexArray( w->va );
//	glDrawElements( GL_TRIANGLES, w->nf*3, GL_UNSIGNED_INT, 0 );

	WF_MTL *m = w->m;
	intptr_t i=0;
	while(m)
	{
		mtl_end();
		if(!m->nf)continue;
		mtl_begin(m);
		
//	glDrawElements(GL_TRIANGLES, m->nf*12, GL_UNSIGNED_INT, (GLvoid*)(12*i));
		i += m->nf;
		m = m->next;
	}

//	glBindVertexArray( 0 );
	return;
}

WF_OBJ* wf_parse(char *filename)
{
	log_info("Loading Wavefront OBJ(\"%s\");", filename);
	FILE *fptr = fopen(filename, "r");
	if(!fptr)return 0;

	char buf[1024];
	int i;


	WF_OBJ *w = malloc(sizeof(WF_OBJ));
	if(!w)
	{
		fclose(fptr);
		log_warning("wf_parse() malloc failed");
		return 0;
	}
	memset(w, 0, sizeof(WF_OBJ));
	w->filename = hcopy(filename);


	// count the verts
	while(fgets(buf, 1024, fptr))
	switch(buf[0]) {
	case 'm':
		tailchomp(buf);
		if(strstr(buf, "mtllib"))
			mtl_load(w, buf+7);
		break;
	case 'v':	// vertex data
		switch(buf[1]) {
		case ' ': w->nv++; break;
		case 'n': w->nn++; break;
		case 't': w->nt++; break;
		}
		break;
	case 'f':	// face
		tailchomp(buf);
		i = 2;
		for(; !eol(buf[i]) && ' ' != buf[i]; i++);
		if(' ' != buf[i])break;		// only a single index
		i++;
		for(; !eol(buf[i]) && ' ' != buf[i]; i++);
		if(' ' != buf[i])break;		// only a line
		i++;
		w->nf++;
		// every point after this is an extra triangle
		while(!eol(buf[i]))
		{		// I sure hope I only see convex polygons
			if(' ' == buf[i])
				w->nf++;
			i++;
		}
		break;
	case 'o':
	case 'g': w->ng++; break;
	case 's': w->ns++; break;
	}
	// alloc the memory
	if(w->nv)w->v = malloc(sizeof(vect)*w->nv);
	if(w->nt)w->vt = malloc(sizeof(vect)*w->nt);
	if(w->nn)w->vn = malloc(sizeof(vect)*w->nn);
	if(w->nf)
	{
		w->f = malloc(sizeof(WF_FACE)*w->nf);
		memset(w->f, 0, sizeof(WF_FACE)*w->nf);
	}
	if(w->ng)w->groups = malloc(sizeof(char*)*w->ng);
	if(w->ns)w->sgroups = malloc(sizeof(int)*w->ns);

	log_debug("Counted v=%d,t=%d,n=%d,f=%d,g=%d,s=%d,m=%d",
			w->nv, w->nt, w->nn, w->nf, w->ng, w->ns, w->nm);
	int vi=0, ti=0, ni=0, fi=0, gi=0, si=0;
	WF_MTL *mi=0;

	int first, last;
	int order=1;
	// read the verts
	rewind(fptr);
	int tmp;
	vect *target;

	WF_MTL *lastmat=0;
	while(fgets(buf, 1024, fptr))
	{
		tailchomp(buf);
		int i=2;
//		log_trace("%s", buf);
		switch(buf[0]) {
		case 'v':

			switch(buf[1]) {
			default: //case ' ':
				target = &w->v[vi++];
				break;
			case 'n':
				target = &w->vn[ni++];
				i++;
				break;
			case 't': 
				target = &w->vt[ti++];
				i++;
				break;
			}

			while(' '==buf[i])i++;
			target->x = fast_atof(buf+i);
			while(' '!=buf[i])i++; i++;
			target->y = fast_atof(buf+i);
			if(buf[1]!='t')
			{
				while(' '!=buf[i])i++; i++;
				target->z = fast_atof(buf+i);
			}
			else
			{
				target->y = 1.0 - target->y;

			}
			break;
	
		case 'u':	
			tailchomp(buf);
			lastmat = find_material(w->m, buf+7);
			break;
		case 's':	// smooth group
			si = atoi(buf+2);

			break;
		case 'g':	// group - I haven't seen them together
		case 'o':	// object
			tailchomp(buf);
			w->groups[gi] = hcopy(buf+2);
			gi++;
			break;
		case 'f':
			i=2;
			for(; !eol(buf[i]) && ' ' != buf[i]; i++);
			if(' ' != buf[i])break;		// only a single index
			i++;
			for(; !eol(buf[i]) && ' ' != buf[i]; i++);
			if(' ' != buf[i])break;		// only a line
			i++;
			if(strstr(buf, "/"))order = 2;
			else order = 1;
			i=1;
			while(' '==buf[i])i++;	//find next space
			switch(order) {
			case 1:
				tmp = atoi(buf+i);
				if(tmp < 0)tmp += w->nv;
				else tmp--;
				w->f[fi].f.x = tmp;

//					log_trace("%d/", g->f[g->nf].x);
				while(' '!=buf[i])i++; i++;	//find next space
				tmp = atoi(buf+i);
				if(tmp < 0)tmp += w->nv;
				else tmp--;
				w->f[fi].f.y = tmp;
//					log_trace("%d/", g->f[g->nf].y);
				while(' '!=buf[i])i++; i++;
				tmp = atoi(buf+i);
				if(tmp < 0)tmp += w->nv;
				else tmp--;
				w->f[fi].f.z = tmp;
//					log_trace("%d", g->f[g->nf].z);
				w->f[fi].s = si;
				w->f[fi].m = lastmat;
				w->f[fi].g = gi;
				first = fi;
				last = first;
				fi++;
				// every point after this is an extra triangle
				while(!eol(buf[i]))
				{		// I sure hope I only see convex polygons
					if(' ' == buf[i])
					{
						i++;
						w->f[fi].f.x = w->f[first].f.x;
						w->f[fi].f.y = w->f[last].f.z;
						tmp = atoi(buf+i);
						if(tmp < 0) tmp += w->nv;
						else tmp--;
						w->f[fi].f.z = tmp;
				w->f[fi].s = si;
				w->f[fi].m = lastmat;
				w->f[fi].g = gi;


						last = fi;
						fi++;
					}
					i++;
				}
				break;
			case 2:
				tmp = atoi(buf+i);
				if(tmp < 0)tmp += w->nv;
				else tmp--;
				w->f[fi].f.x = tmp;
				while('/'!=buf[i])i++; i++;	//find next slash
				tmp = atoi(buf+i);
				if(tmp < 0)tmp += w->nt;
				else tmp--;
				w->f[fi].t.x = tmp;

//					log_debug("%d/", g->f[g->nf].x);
				while(' '!=buf[i])i++; i++;	//find next space
				tmp = atoi(buf+i);
				if(tmp < 0)tmp += w->nv;
				else tmp--;
				w->f[fi].f.y = tmp;
				while('/'!=buf[i])i++; i++;	//find next slash
				tmp = atoi(buf+i);
				if(tmp < 0)tmp += w->nt;
				else tmp--;
				w->f[fi].t.y = tmp;
//					log_debug("%d/", g->f[g->nf].y);
				while(' '!=buf[i])i++; i++;
				tmp = atoi(buf+i);
				if(tmp < 0)tmp += w->nv;
				else tmp--;
				w->f[fi].f.z = tmp;
				while('/'!=buf[i])i++; i++;	//find next slash
				tmp = atoi(buf+i);
				if(tmp < 0)tmp += w->nt;
				else tmp--;
				w->f[fi].t.z = tmp;
//					log_debug("%d", g->f[g->nf].z);
				w->f[fi].s = si;
				w->f[fi].m = lastmat;
				w->f[fi].g = gi;
				first = fi;
				last = first;
				fi++;
				// every point after this is an extra triangle
				while(!eol(buf[i]))
				{		// I sure hope I only see convex polygons
					if(' ' == buf[i])
					{
						i++;
						w->f[fi].f.x = w->f[first].f.x;
						w->f[fi].t.x = w->f[first].t.x;

						w->f[fi].f.y = w->f[last].f.z;
						w->f[fi].t.y = w->f[last].t.z;

						tmp = atoi(buf+i);
						if(tmp < 0) tmp += w->nv;
						else tmp--;
						w->f[fi].f.z = tmp;
						while('/'!=buf[i])i++; i++;	//find next slash
						tmp = atoi(buf+i);
						if(tmp < 0)tmp += w->nt;
						else tmp--;
						w->f[fi].t.z = tmp;

						w->f[fi].s = si;
						w->f[fi].m = lastmat;
						w->f[fi].g = gi;
						last = fi;
						fi++;
					}
					i++;
				}
				break;
		case 3:
				w->f[fi].f.x = abs(atoi(buf+i))-1;
				while('/'!=buf[i])i++; i++;
				if(buf[i]!='/')w->f[fi].t.x = abs(atoi(buf+i))-1;
//				while('/'!=buf[i])i++; i++;
//				if(buf[i]!=' ')w->f[fi].n.x = abs(atoi(buf+i))-1;

				while(' '!=buf[i])i++; i++;	//find next space
				w->f[fi].f.y = abs(atoi(buf+i))-1;
				while('/'!=buf[i])i++; i++;
				if(buf[i]!='/')w->f[fi].t.y = abs(atoi(buf+i))-1;
//				while('/'!=buf[i])i++; i++;
//				if(buf[i]!=' ')w->f[fi].n.y = abs(atoi(buf+i))-1;

				while(' '!=buf[i])i++; i++;	//find next space
				w->f[fi].f.z = abs(atoi(buf+i))-1;
				while('/'!=buf[i])i++; i++;
				if(buf[i]!='/')w->f[fi].t.z = abs(atoi(buf+i))-1;
//				while('/'!=buf[i])i++; i++;
//				if(buf[i]!=' ')w->f[fi].n.z = abs(atoi(buf+i))-1;
				
				w->f[fi].s = si;
				w->f[fi].m = mi;
				w->f[fi].g = gi;

				first = fi;
				last = first;
				fi++;
				// every point after this is an extra triangle
				while('\n'!=buf[i])
				{		// I sure hope I only see convex polygons
					if(' ' == buf[i])
					{
						i++;
						w->f[fi].f.x = w->f[first].f.x;
						w->f[fi].t.x = w->f[first].t.x;
						w->f[fi].n.x = w->f[first].n.x;
						w->f[fi].f.y = w->f[last].f.z;
						w->f[fi].t.y = w->f[last].t.z;
						w->f[fi].n.y = w->f[last].n.z;
				w->f[fi].f.z = abs(atoi(buf+i))-1;
				while('/'!=buf[i])i++; i++;
				if(buf[i]!='/')w->f[fi].t.z = abs(atoi(buf+i))-1;
//				while('/'!=buf[i])i++; i++;
//				if(buf[i]!=' ')w->f[fi].n.z = abs(atoi(buf+i))-1;
						w->f[fi].s = si;
						w->f[fi].m = mi;
						w->f[fi].g = gi;


						last = fi;
						fi++;
					}
					i++;
				}
				break;	// face reader break
			}
			break;	// line type break
		}
	}
	log_debug("Read    v=%d,t=%d,n=%d,f=%d,g=%d,s=%d",
			vi, ti, ni, fi, gi, si);

	fclose(fptr);
	log_debug("At fclose we have %d/%d/%d", w->nv, w->nt, w->nn);
	return w;
}


WF_OBJ* wf_load(char * filename)
{
	WF_OBJ *w = wf_parse(filename);

	wf_bound(w);

	if(w->nv != w->nn)wf_normals(w);
	wf_texcoords(w);

	wf_interleave(w);
	wf_gpu_load(w);
	w->draw = wf_draw;

	log_debug("All done");
	return w;
}


void wf_free(WF_OBJ *w)
{
	if(!w)return;
	if(w->v)free(w->v);
	if(w->vt)free(w->vt);
	if(w->vn)free(w->vn);
	if(w->p)free(w->p);
	if(w->f)free(w->f);
	if(w->fn)free(w->fn);
#ifndef STATIC_TEST
	wf_gpu_unload(w);
#endif
	WF_MTL *mt, *m = w->m;
	while(m)
	{
		mt = m->next;
		mtl_free(m);
		m = mt;
	}
	if(w->filename)free(w->filename);
	free(w); 
}

#ifdef STATIC_TEST

int main(int argc, char *argv[])
{
	WF_OBJ *w;
	switch(argc) {
		case 2:
			w = wf_load(argv[1]);
			wf_free(w);
			break;
		default:
			log_fatal("user error");
			return 1;
	}

	return 0;
}

#endif /* STATIC_TEST */

