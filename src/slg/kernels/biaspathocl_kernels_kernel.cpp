#include <string>
namespace slg { namespace ocl {
std::string KernelSource_biaspathocl_kernels = 
"#line 2 \"biaspatchocl_kernels.cl\"\n"
"\n"
"/***************************************************************************\n"
" *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *\n"
" *                                                                         *\n"
" *   This file is part of LuxRays.                                         *\n"
" *                                                                         *\n"
" *   LuxRays is free software; you can redistribute it and/or modify       *\n"
" *   it under the terms of the GNU General Public License as published by  *\n"
" *   the Free Software Foundation; either version 3 of the License, or     *\n"
" *   (at your option) any later version.                                   *\n"
" *                                                                         *\n"
" *   LuxRays is distributed in the hope that it will be useful,            *\n"
" *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *\n"
" *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *\n"
" *   GNU General Public License for more details.                          *\n"
" *                                                                         *\n"
" *   You should have received a copy of the GNU General Public License     *\n"
" *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *\n"
" *                                                                         *\n"
" *   LuxRays website: http://www.luxrender.net                             *\n"
" ***************************************************************************/\n"
"\n"
"// List of symbols defined at compile time:\n"
"//  PARAM_TASK_COUNT\n"
"//  PARAM_TILE_SIZE\n"
"//  PARAM_TILE_PROGRESSIVE_REFINEMENT\n"
"//  PARAM_AA_SAMPLES\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// InitSeed Kernel\n"
"//------------------------------------------------------------------------------\n"
"\n"
"__kernel __attribute__((work_group_size_hint(64, 1, 1))) void InitSeed(\n"
"		uint seedBase,\n"
"		__global GPUTask *tasks) {\n"
"	const size_t gid = get_global_id(0);\n"
"	if (gid >= PARAM_TASK_COUNT)\n"
"		return;\n"
"\n"
"	// Initialize the task\n"
"	__global GPUTask *task = &tasks[gid];\n"
"\n"
"	// Initialize random number generator\n"
"	Seed seed;\n"
"	Rnd_Init(seedBase + gid, &seed);\n"
"\n"
"	// Save the seed\n"
"	task->seed.s1 = seed.s1;\n"
"	task->seed.s2 = seed.s2;\n"
"	task->seed.s3 = seed.s3;\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// InitStats Kernel\n"
"//------------------------------------------------------------------------------\n"
"\n"
"__kernel __attribute__((work_group_size_hint(64, 1, 1))) void InitStat(\n"
"		__global GPUTaskStats *taskStats) {\n"
"	const size_t gid = get_global_id(0);\n"
"	if (gid >= PARAM_TASK_COUNT)\n"
"		return;\n"
"\n"
"	__global GPUTaskStats *taskStat = &taskStats[gid];\n"
"	taskStat->raysCount = 0;\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// BiasAdvancePaths Kernel\n"
"//------------------------------------------------------------------------------\n"
"\n"
"void SampleGrid(Seed *seed, const uint size,\n"
"		const uint ix, const uint iy, float *u0, float *u1) {\n"
"	*u0 = Rnd_FloatValue(seed);\n"
"	*u1 = Rnd_FloatValue(seed);\n"
"\n"
"	if (size > 1) {\n"
"		const float idim = 1.f / size;\n"
"		*u0 = (ix + *u0) * idim;\n"
"		*u1 = (iy + *u1) * idim;\n"
"	}\n"
"}\n"
"\n"
"void GenerateCameraRay(\n"
"		Seed *seed,\n"
"		__global GPUTask *task,\n"
"		__global Camera *camera,\n"
"		__global float *pixelFilterDistribution,\n"
"		const uint pixelX, const uint pixelY,\n"
"		const uint tileStartX, const uint tileStartY, const int tileSampleIndex,\n"
"		const uint engineFilmWidth, const uint engineFilmHeight,\n"
"		Ray *ray) {\n"
"	float u0, u1;\n"
"	SampleGrid(seed, PARAM_AA_SAMPLES,\n"
"			tileSampleIndex % PARAM_AA_SAMPLES, tileSampleIndex / PARAM_AA_SAMPLES,\n"
"			&u0, &u1);\n"
"\n"
"	float2 xy;\n"
"	float distPdf;\n"
"	Distribution2D_SampleContinuous(pixelFilterDistribution, u0, u1, &xy, &distPdf);\n"
"\n"
"	const float filmX = pixelX + .5f + xy.x;\n"
"	const float filmY = pixelY + .5f + xy.y;\n"
"	task->result.filmX = filmX;\n"
"	task->result.filmY = filmY;\n"
"\n"
"#if defined(PARAM_CAMERA_HAS_DOF)\n"
"	const float dofSampleX = Rnd_FloatValue(seed);\n"
"	const float dofSampleY = Rnd_FloatValue(seed);\n"
"#endif\n"
"\n"
"	Camera_GenerateRay(camera, engineFilmWidth, engineFilmHeight, ray, tileStartX + filmX, tileStartY + filmY\n"
"#if defined(PARAM_CAMERA_HAS_DOF)\n"
"			, dofSampleX, dofSampleY\n"
"#endif\n"
"			);	\n"
"}\n"
"\n"
"__kernel __attribute__((work_group_size_hint(64, 1, 1))) void RenderSample(\n"
"		const uint tileStartX,\n"
"		const uint tileStartY,\n"
"		const int tileSampleIndex,\n"
"		const uint engineFilmWidth, const uint engineFilmHeight,\n"
"		__global GPUTask *tasks,\n"
"		__global GPUTaskStats *taskStats,\n"
"		__global float *pixelFilterDistribution,\n"
"		// Film parameters\n"
"		const uint filmWidth, const uint filmHeight\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_0)\n"
"		, __global float *filmRadianceGroup0\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_1)\n"
"		, __global float *filmRadianceGroup1\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_2)\n"
"		, __global float *filmRadianceGroup2\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_3)\n"
"		, __global float *filmRadianceGroup3\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_4)\n"
"		, __global float *filmRadianceGroup4\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_5)\n"
"		, __global float *filmRadianceGroup5\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_6)\n"
"		, __global float *filmRadianceGroup6\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_7)\n"
"		, __global float *filmRadianceGroup7\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_ALPHA)\n"
"		, __global float *filmAlpha\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DEPTH)\n"
"		, __global float *filmDepth\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_POSITION)\n"
"		, __global float *filmPosition\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_GEOMETRY_NORMAL)\n"
"		, __global float *filmGeometryNormal\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_SHADING_NORMAL)\n"
"		, __global float *filmShadingNormal\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_MATERIAL_ID)\n"
"		, __global uint *filmMaterialID\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_DIFFUSE)\n"
"		, __global float *filmDirectDiffuse\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_GLOSSY)\n"
"		, __global float *filmDirectGlossy\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_EMISSION)\n"
"		, __global float *filmEmission\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_DIFFUSE)\n"
"		, __global float *filmIndirectDiffuse\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_GLOSSY)\n"
"		, __global float *filmIndirectGlossy\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_SPECULAR)\n"
"		, __global float *filmIndirectSpecular\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_MATERIAL_ID_MASK)\n"
"		, __global float *filmMaterialIDMask\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_SHADOW_MASK)\n"
"		, __global float *filmDirectShadowMask\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_SHADOW_MASK)\n"
"		, __global float *filmIndirectShadowMask\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_UV)\n"
"		, __global float *filmUV\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_RAYCOUNT)\n"
"		, __global float *filmRayCount\n"
"#endif\n"
"		,\n"
"		// Scene parameters\n"
"		const float worldCenterX,\n"
"		const float worldCenterY,\n"
"		const float worldCenterZ,\n"
"		const float worldRadius,\n"
"		__global Material *mats,\n"
"		__global Texture *texs,\n"
"		__global uint *meshMats,\n"
"		__global Mesh *meshDescs,\n"
"		__global Point *vertices,\n"
"#if defined(PARAM_HAS_NORMALS_BUFFER)\n"
"		__global Vector *vertNormals,\n"
"#endif\n"
"#if defined(PARAM_HAS_UVS_BUFFER)\n"
"		__global UV *vertUVs,\n"
"#endif\n"
"#if defined(PARAM_HAS_COLS_BUFFER)\n"
"		__global Spectrum *vertCols,\n"
"#endif\n"
"#if defined(PARAM_HAS_ALPHAS_BUFFER)\n"
"		__global float *vertAlphas,\n"
"#endif\n"
"		__global Triangle *triangles,\n"
"		__global Camera *camera,\n"
"		__global float *lightsDistribution\n"
"#if defined(PARAM_HAS_INFINITELIGHT)\n"
"		, __global InfiniteLight *infiniteLight\n"
"		, __global float *infiniteLightDistribution\n"
"#endif\n"
"#if defined(PARAM_HAS_SUNLIGHT)\n"
"		, __global SunLight *sunLight\n"
"#endif\n"
"#if defined(PARAM_HAS_SKYLIGHT)\n"
"		, __global SkyLight *skyLight\n"
"#endif\n"
"#if (PARAM_DL_LIGHT_COUNT > 0)\n"
"		, __global TriangleLight *triLightDefs\n"
"		, __global uint *meshTriLightDefsOffset\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_0)\n"
"		, __global ImageMap *imageMapDescs, __global float *imageMapBuff0\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_1)\n"
"		, __global float *imageMapBuff1\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_2)\n"
"		, __global float *imageMapBuff2\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_3)\n"
"		, __global float *imageMapBuff3\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_4)\n"
"		, __global float *imageMapBuff4\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_5)\n"
"		, __global float *imageMapBuff5\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_6)\n"
"		, __global float *imageMapBuff6\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_7)\n"
"		, __global float *imageMapBuff7\n"
"#endif\n"
"		ACCELERATOR_INTERSECT_PARAM_DECL\n"
"		) {\n"
"	const size_t gid = get_global_id(0);\n"
"\n"
"#if defined(PARAM_TILE_PROGRESSIVE_REFINEMENT)\n"
"	const uint sampleIndex = tileSampleIndex;\n"
"	const uint samplePixelX = gid % PARAM_TILE_SIZE;\n"
"	const uint samplePixelY = gid / PARAM_TILE_SIZE;\n"
"#else\n"
"	const uint sampleIndex = gid % (PARAM_AA_SAMPLES * PARAM_AA_SAMPLES);\n"
"	const uint samplePixelIndex = gid / (PARAM_AA_SAMPLES * PARAM_AA_SAMPLES);\n"
"	const uint samplePixelX = samplePixelIndex % PARAM_TILE_SIZE;\n"
"	const uint samplePixelY = samplePixelIndex / PARAM_TILE_SIZE;\n"
"#endif\n"
"\n"
"	if ((tileStartX + samplePixelX >= engineFilmWidth) ||\n"
"			(tileStartY + samplePixelY >= engineFilmHeight))\n"
"		return;\n"
"\n"
"	__global GPUTask *task = &tasks[gid];\n"
"	__global GPUTaskStats *taskStat = &taskStats[gid];\n"
"\n"
"	//--------------------------------------------------------------------------\n"
"	// Initialize image maps page pointer table\n"
"	//--------------------------------------------------------------------------\n"
"\n"
"#if defined(PARAM_HAS_IMAGEMAPS)\n"
"	__global float *imageMapBuff[PARAM_IMAGEMAPS_COUNT];\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_0)\n"
"	imageMapBuff[0] = imageMapBuff0;\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_1)\n"
"	imageMapBuff[1] = imageMapBuff1;\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_2)\n"
"	imageMapBuff[2] = imageMapBuff2;\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_3)\n"
"	imageMapBuff[3] = imageMapBuff3;\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_4)\n"
"	imageMapBuff[4] = imageMapBuff4;\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_5)\n"
"	imageMapBuff[5] = imageMapBuff5;\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_6)\n"
"	imageMapBuff[6] = imageMapBuff6;\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_7)\n"
"	imageMapBuff[7] = imageMapBuff7;\n"
"#endif\n"
"#endif\n"
"\n"
"	//--------------------------------------------------------------------------\n"
"	// Initialize Film radiance group pointer table\n"
"	//--------------------------------------------------------------------------\n"
"\n"
"	__global float *filmRadianceGroup[PARAM_FILM_RADIANCE_GROUP_COUNT];\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_0)\n"
"		filmRadianceGroup[0] = filmRadianceGroup0;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_1)\n"
"		filmRadianceGroup[1] = filmRadianceGroup1;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_2)\n"
"		filmRadianceGroup[2] = filmRadianceGroup2;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_3)\n"
"		filmRadianceGroup[3] = filmRadianceGroup3;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_4)\n"
"		filmRadianceGroup[3] = filmRadianceGroup4;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_5)\n"
"		filmRadianceGroup[3] = filmRadianceGroup5;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_6)\n"
"		filmRadianceGroup[3] = filmRadianceGroup6;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_7)\n"
"		filmRadianceGroup[3] = filmRadianceGroup7;\n"
"#endif\n"
"\n"
"	//--------------------------------------------------------------------------\n"
"	// Initialize the first ray\n"
"	//--------------------------------------------------------------------------\n"
"\n"
"	// Read the seed\n"
"	Seed seed;\n"
"	seed.s1 = task->seed.s1;\n"
"	seed.s2 = task->seed.s2;\n"
"	seed.s3 = task->seed.s3;\n"
"\n"
"	SampleResult_Init(&task->result);\n"
"\n"
"	Ray ray;\n"
"	RayHit rayHit;\n"
"	GenerateCameraRay(&seed, task, camera, pixelFilterDistribution,\n"
"			samplePixelX, samplePixelY,\n"
"			tileStartX, tileStartY, tileSampleIndex,\n"
"			engineFilmWidth, engineFilmHeight, &ray);\n"
"	PathState pathState = RT_NEXT_VERTEX;\n"
"\n"
"	//--------------------------------------------------------------------------\n"
"	// Render a sample\n"
"	//--------------------------------------------------------------------------\n"
"\n"
"	uint tracedRaysCount = taskStat->raysCount;\n"
"	do {\n"
"		//----------------------------------------------------------------------\n"
"		// Ray trace step\n"
"		//----------------------------------------------------------------------\n"
"\n"
"		Accelerator_Intersect(&ray, &rayHit\n"
"			ACCELERATOR_INTERSECT_PARAM);\n"
"		++tracedRaysCount;\n"
"\n"
"		//----------------------------------------------------------------------\n"
"		// Advance the finite state machine step\n"
"		//----------------------------------------------------------------------\n"
"\n"
"		//----------------------------------------------------------------------\n"
"		// Evaluation of the Path finite state machine.\n"
"		//\n"
"		// From: RT_NEXT_VERTEX\n"
"		// To: SPLAT_SAMPLE or GENERATE_DL_RAY\n"
"		//----------------------------------------------------------------------\n"
"\n"
"		if (pathState == RT_NEXT_VERTEX) {\n"
"			if (rayHit.meshIndex != NULL_INDEX) {\n"
"				//--------------------------------------------------------------\n"
"				// Something was hit\n"
"				//--------------------------------------------------------------\n"
"\n"
"				VADD3F(&task->result.radiancePerPixelNormalized[0].r, WHITE);\n"
"				pathState = SPLAT_SAMPLE;\n"
"			} else {\n"
"				//--------------------------------------------------------------\n"
"				// Nothing was hit, add environmental lights radiance\n"
"				//--------------------------------------------------------------\n"
"\n"
"				VADD3F(&task->result.radiancePerPixelNormalized[0].r, BLACK);\n"
"				pathState = SPLAT_SAMPLE;\n"
"			}\n"
"		}\n"
"\n"
"		//----------------------------------------------------------------------\n"
"		// Evaluation of the Path finite state machine.\n"
"		//\n"
"		// From: SPLAT_SAMPLE\n"
"		// To: RT_NEXT_VERTEX\n"
"		//----------------------------------------------------------------------\n"
"\n"
"		if (pathState == SPLAT_SAMPLE) {\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_RAYCOUNT)\n"
"			task->result.rayCount = tracedRaysCount;\n"
"#endif\n"
"#if defined(PARAM_TILE_PROGRESSIVE_REFINEMENT)\n"
"			Film_AddSample(samplePixelX, samplePixelY, &task->result, 1.f\n"
"					FILM_PARAM);\n"
"#endif\n"
"\n"
"			pathState = DONE;\n"
"		}\n"
"\n"
"		pathState = DONE;\n"
"	} while (pathState != DONE);\n"
"\n"
"	//--------------------------------------------------------------------------\n"
"\n"
"	taskStat->raysCount = tracedRaysCount;\n"
"\n"
"	// Save the seed\n"
"	task->seed.s1 = seed.s1;\n"
"	task->seed.s2 = seed.s2;\n"
"	task->seed.s3 = seed.s3;\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// MergePixelSamples\n"
"//------------------------------------------------------------------------------\n"
"\n"
"void SR_Init(SampleResult *sampleResult) {\n"
"	// Initialize only Spectrum fields\n"
"\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_0)\n"
"	sampleResult->radiancePerPixelNormalized[0].r = 0.f;\n"
"	sampleResult->radiancePerPixelNormalized[0].g = 0.f;\n"
"	sampleResult->radiancePerPixelNormalized[0].b = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_1)\n"
"	sampleResult->radiancePerPixelNormalized[1].r = 0.f;\n"
"	sampleResult->radiancePerPixelNormalized[1].g = 0.f;\n"
"	sampleResult->radiancePerPixelNormalized[1].b = 0.f\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_2)\n"
"	sampleResult->radiancePerPixelNormalized[2].r = 0.f;\n"
"	sampleResult->radiancePerPixelNormalized[2].g = 0.f;\n"
"	sampleResult->radiancePerPixelNormalized[2].b = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_3)\n"
"	sampleResult->radiancePerPixelNormalized[3].r = 0.f;\n"
"	sampleResult->radiancePerPixelNormalized[3].g = 0.f;\n"
"	sampleResult->radiancePerPixelNormalized[3].b = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_4)\n"
"	sampleResult->radiancePerPixelNormalized[4].r = 0.f;\n"
"	sampleResult->radiancePerPixelNormalized[4].g = 0.f;\n"
"	sampleResult->radiancePerPixelNormalized[4].b = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_5)\n"
"	sampleResult->radiancePerPixelNormalized[5].r = 0.f;\n"
"	sampleResult->radiancePerPixelNormalized[5].g = 0.f;\n"
"	sampleResult->radiancePerPixelNormalized[5].b = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_6)\n"
"	sampleResult->radiancePerPixelNormalized[6].r = 0.f;\n"
"	sampleResult->radiancePerPixelNormalized[6].g = 0.f;\n"
"	sampleResult->radiancePerPixelNormalized[6].b = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_7)\n"
"	sampleResult->radiancePerPixelNormalized[7].r = 0.f;\n"
"	sampleResult->radiancePerPixelNormalized[7].g = 0.f;\n"
"	sampleResult->radiancePerPixelNormalized[7].b = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_DIFFUSE)\n"
"	sampleResult->directDiffuse.r = 0.f;\n"
"	sampleResult->directDiffuse.g = 0.f;\n"
"	sampleResult->directDiffuse.b = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_GLOSSY)\n"
"	sampleResult->directGlossy.r = 0.f;\n"
"	sampleResult->directGlossy.g = 0.f;\n"
"	sampleResult->directGlossy.b = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_EMISSION)\n"
"	sampleResult->emission.r = 0.f;\n"
"	sampleResult->emission.g = 0.f;\n"
"	sampleResult->emission.b = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_DIFFUSE)\n"
"	sampleResult->indirectDiffuse.r = 0.f;\n"
"	sampleResult->indirectDiffuse.g = 0.f;\n"
"	sampleResult->indirectDiffuse.b = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_GLOSSY)\n"
"	sampleResult->indirectGlossy.r = 0.f;\n"
"	sampleResult->indirectGlossy.g = 0.f;\n"
"	sampleResult->indirectGlossy.b = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_SPECULAR)\n"
"	sampleResult->indirectSpecular.r = 0.f;\n"
"	sampleResult->indirectSpecular.g = 0.f;\n"
"	sampleResult->indirectSpecular.b = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DEPTH)\n"
"	sampleResult->depth = INFINITY;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_RAYCOUNT)\n"
"	sampleResult->rayCount = 0.f;\n"
"#endif\n"
"}\n"
"\n"
"void SR_Accumulate(__global SampleResult *src, SampleResult *dst) {\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_0)\n"
"	dst->radiancePerPixelNormalized[0].r += src->radiancePerPixelNormalized[0].r;\n"
"	dst->radiancePerPixelNormalized[0].g += src->radiancePerPixelNormalized[0].g;\n"
"	dst->radiancePerPixelNormalized[0].b += src->radiancePerPixelNormalized[0].b;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_1)\n"
"	dst->radiancePerPixelNormalized[1].r += src->radiancePerPixelNormalized[1].r;\n"
"	dst->radiancePerPixelNormalized[1].g += src->radiancePerPixelNormalized[1].g;\n"
"	dst->radiancePerPixelNormalized[1].b += src->radiancePerPixelNormalized[1].b;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_2)\n"
"	dst->radiancePerPixelNormalized[2].r += src->radiancePerPixelNormalized[2].r;\n"
"	dst->radiancePerPixelNormalized[2].g += src->radiancePerPixelNormalized[2].g;\n"
"	dst->radiancePerPixelNormalized[2].b += src->radiancePerPixelNormalized[2].b;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_3)\n"
"	dst->radiancePerPixelNormalized[3].r += src->radiancePerPixelNormalized[3].r;\n"
"	dst->radiancePerPixelNormalized[3].g += src->radiancePerPixelNormalized[3].g;\n"
"	dst->radiancePerPixelNormalized[3].b += src->radiancePerPixelNormalized[3].b;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_4)\n"
"	dst->radiancePerPixelNormalized[4].r += src->radiancePerPixelNormalized[4].r;\n"
"	dst->radiancePerPixelNormalized[4].g += src->radiancePerPixelNormalized[4].g;\n"
"	dst->radiancePerPixelNormalized[4].b += src->radiancePerPixelNormalized[4].b;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_5)\n"
"	dst->radiancePerPixelNormalized[5].r += src->radiancePerPixelNormalized[5].r;\n"
"	dst->radiancePerPixelNormalized[5].g += src->radiancePerPixelNormalized[5].g;\n"
"	dst->radiancePerPixelNormalized[5].b += src->radiancePerPixelNormalized[5].b;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_6)\n"
"	dst->radiancePerPixelNormalized[6].r += src->radiancePerPixelNormalized[6].r;\n"
"	dst->radiancePerPixelNormalized[6].g += src->radiancePerPixelNormalized[6].g;\n"
"	dst->radiancePerPixelNormalized[6].b += src->radiancePerPixelNormalized[6].b;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_7)\n"
"	dst->radiancePerPixelNormalized[7].r += src->radiancePerPixelNormalized[7].r;\n"
"	dst->radiancePerPixelNormalized[7].g += src->radiancePerPixelNormalized[7].g;\n"
"	dst->radiancePerPixelNormalized[7].b += src->radiancePerPixelNormalized[7].b;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_ALPHA)\n"
"	dst->alpha += dst->alpha + src->alpha;\n"
"#endif\n"
"\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_DIFFUSE)\n"
"	dst->directDiffuse.r += src->directDiffuse.r;\n"
"	dst->directDiffuse.g += src->directDiffuse.g;\n"
"	dst->directDiffuse.b += src->directDiffuse.b;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_GLOSSY)\n"
"	dst->directGlossy.r += src->directGlossy.r;\n"
"	dst->directGlossy.g += src->directGlossy.g;\n"
"	dst->directGlossy.b += src->directGlossy.b;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_EMISSION)\n"
"	dst->emission.r += src->emission.r;\n"
"	dst->emission.g += src->emission.g;\n"
"	dst->emission.b += src->emission.b;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_DIFFUSE)\n"
"	dst->indirectDiffuse.r += src->indirectDiffuse.r;\n"
"	dst->indirectDiffuse.g += src->indirectDiffuse.g;\n"
"	dst->indirectDiffuse.b += src->indirectDiffuse.b;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_GLOSSY)\n"
"	dst->indirectGlossy.r += src->indirectGlossy.r;\n"
"	dst->indirectGlossy.g += src->indirectGlossy.g;\n"
"	dst->indirectGlossy.b += src->indirectGlossy.b;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_SPECULAR)\n"
"	dst->indirectSpecular.r += src->indirectSpecular.r;\n"
"	dst->indirectSpecular.g += src->indirectSpecular.g;\n"
"	dst->indirectSpecular.b += src->indirectSpecular.b;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_SHADOW_MASK)\n"
"	dst->directShadowMask += src->directShadowMask;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_SHADOW_MASK)\n"
"	dst->indirectShadowMask += src->indirectShadowMask;\n"
"#endif\n"
"\n"
"	bool depthWrite = true;\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DEPTH)\n"
"	const float srcDepthValue = src->depth;\n"
"	if (srcDepthValue <= dst->depth)\n"
"		dst->depth = srcDepthValue;\n"
"	else\n"
"		depthWrite = false;\n"
"#endif\n"
"	if (depthWrite) {\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_POSITION)\n"
"		dst->position = src->position;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_GEOMETRY_NORMAL)\n"
"		dst->geometryNormal = src->geometryNormal;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_SHADING_NORMAL)\n"
"		dst->shadingNormal = src->shadingNormal;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_MATERIAL_ID)\n"
"		// Note: MATERIAL_ID_MASK is calculated starting from materialID field\n"
"		dst->materialID = src->materialID;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_UV)\n"
"		dst->uv = src->uv;\n"
"#endif\n"
"	}\n"
"\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_RAYCOUNT)\n"
"	dst->rayCount += src->rayCount;\n"
"#endif\n"
"}\n"
"\n"
"void SR_Normalize(SampleResult *dst, const float k) {\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_0)\n"
"	dst->radiancePerPixelNormalized[0].r *= k;\n"
"	dst->radiancePerPixelNormalized[0].g *= k;\n"
"	dst->radiancePerPixelNormalized[0].b *= k;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_1)\n"
"	dst->radiancePerPixelNormalized[1].r *= k;\n"
"	dst->radiancePerPixelNormalized[1].g *= k;\n"
"	dst->radiancePerPixelNormalized[1].b *= k;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_2)\n"
"	dst->radiancePerPixelNormalized[2].r *= k;\n"
"	dst->radiancePerPixelNormalized[2].g *= k;\n"
"	dst->radiancePerPixelNormalized[2].b *= k;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_3)\n"
"	dst->radiancePerPixelNormalized[3].r *= k;\n"
"	dst->radiancePerPixelNormalized[3].g *= k;\n"
"	dst->radiancePerPixelNormalized[3].b *= k;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_4)\n"
"	dst->radiancePerPixelNormalized[4].r *= k;\n"
"	dst->radiancePerPixelNormalized[4].g *= k;\n"
"	dst->radiancePerPixelNormalized[4].b *= k;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_5)\n"
"	dst->radiancePerPixelNormalized[5].r *= k;\n"
"	dst->radiancePerPixelNormalized[5].g *= k;\n"
"	dst->radiancePerPixelNormalized[5].b *= k;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_6)\n"
"	dst->radiancePerPixelNormalized[6].r *= k;\n"
"	dst->radiancePerPixelNormalized[6].g *= k;\n"
"	dst->radiancePerPixelNormalized[6].b *= k;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_7)\n"
"	dst->radiancePerPixelNormalized[7].r *= k;\n"
"	dst->radiancePerPixelNormalized[7].g *= k;\n"
"	dst->radiancePerPixelNormalized[7].b *= k;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_ALPHA)\n"
"	dst->alpha *= k;\n"
"#endif\n"
"\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_DIFFUSE)\n"
"	dst->directDiffuse.r *= k;\n"
"	dst->directDiffuse.g *= k;\n"
"	dst->directDiffuse.b *= k;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_GLOSSY)\n"
"	dst->directGlossy.r *= k;\n"
"	dst->directGlossy.g *= k;\n"
"	dst->directGlossy.b *= k;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_EMISSION)\n"
"	dst->emission.r *= k;\n"
"	dst->emission.g *= k;\n"
"	dst->emission.b *= k;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_DIFFUSE)\n"
"	dst->indirectDiffuse.r *= k;\n"
"	dst->indirectDiffuse.g *= k;\n"
"	dst->indirectDiffuse.b *= k;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_GLOSSY)\n"
"	dst->indirectGlossy.r *= k;\n"
"	dst->indirectGlossy.g *= k;\n"
"	dst->indirectGlossy.b *= k;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_SPECULAR)\n"
"	dst->indirectSpecular.r *= k;\n"
"	dst->indirectSpecular.g *= k;\n"
"	dst->indirectSpecular.b *= k;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_SHADOW_MASK)\n"
"	dst->directShadowMask *= k;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_SHADOW_MASK)\n"
"	dst->indirectShadowMask *= k;\n"
"#endif\n"
"}\n"
"\n"
"__kernel __attribute__((work_group_size_hint(64, 1, 1))) void MergePixelSamples(\n"
"		const uint tileStartX,\n"
"		const uint tileStartY,\n"
"		const uint engineFilmWidth, const uint engineFilmHeight,\n"
"		__global GPUTask *tasks,\n"
"		// Film parameters\n"
"		const uint filmWidth, const uint filmHeight\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_0)\n"
"		, __global float *filmRadianceGroup0\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_1)\n"
"		, __global float *filmRadianceGroup1\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_2)\n"
"		, __global float *filmRadianceGroup2\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_3)\n"
"		, __global float *filmRadianceGroup3\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_4)\n"
"		, __global float *filmRadianceGroup4\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_5)\n"
"		, __global float *filmRadianceGroup5\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_6)\n"
"		, __global float *filmRadianceGroup6\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_7)\n"
"		, __global float *filmRadianceGroup7\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_ALPHA)\n"
"		, __global float *filmAlpha\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DEPTH)\n"
"		, __global float *filmDepth\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_POSITION)\n"
"		, __global float *filmPosition\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_GEOMETRY_NORMAL)\n"
"		, __global float *filmGeometryNormal\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_SHADING_NORMAL)\n"
"		, __global float *filmShadingNormal\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_MATERIAL_ID)\n"
"		, __global uint *filmMaterialID\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_DIFFUSE)\n"
"		, __global float *filmDirectDiffuse\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_GLOSSY)\n"
"		, __global float *filmDirectGlossy\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_EMISSION)\n"
"		, __global float *filmEmission\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_DIFFUSE)\n"
"		, __global float *filmIndirectDiffuse\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_GLOSSY)\n"
"		, __global float *filmIndirectGlossy\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_SPECULAR)\n"
"		, __global float *filmIndirectSpecular\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_MATERIAL_ID_MASK)\n"
"		, __global float *filmMaterialIDMask\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_SHADOW_MASK)\n"
"		, __global float *filmDirectShadowMask\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_SHADOW_MASK)\n"
"		, __global float *filmIndirectShadowMask\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_UV)\n"
"		, __global float *filmUV\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_RAYCOUNT)\n"
"		, __global float *filmRayCount\n"
"#endif\n"
"		) {\n"
"	const size_t gid = get_global_id(0);\n"
"\n"
"	uint sampleX, sampleY;\n"
"	sampleX = gid % PARAM_TILE_SIZE;\n"
"	sampleY = gid / PARAM_TILE_SIZE;\n"
"\n"
"	if ((tileStartX + sampleX >= engineFilmWidth) ||\n"
"			(tileStartY + sampleY >= engineFilmHeight))\n"
"		return;\n"
"\n"
"	__global GPUTask *sampleTasks = &tasks[gid * PARAM_AA_SAMPLES * PARAM_AA_SAMPLES];\n"
"\n"
"	//--------------------------------------------------------------------------\n"
"	// Initialize Film radiance group pointer table\n"
"	//--------------------------------------------------------------------------\n"
"\n"
"	__global float *filmRadianceGroup[PARAM_FILM_RADIANCE_GROUP_COUNT];\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_0)\n"
"	filmRadianceGroup[0] = filmRadianceGroup0;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_1)\n"
"	filmRadianceGroup[1] = filmRadianceGroup1;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_2)\n"
"	filmRadianceGroup[2] = filmRadianceGroup2;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_3)\n"
"	filmRadianceGroup[3] = filmRadianceGroup3;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_4)\n"
"	filmRadianceGroup[3] = filmRadianceGroup4;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_5)\n"
"	filmRadianceGroup[3] = filmRadianceGroup5;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_6)\n"
"	filmRadianceGroup[3] = filmRadianceGroup6;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_7)\n"
"	filmRadianceGroup[3] = filmRadianceGroup7;\n"
"#endif\n"
"\n"
"	//--------------------------------------------------------------------------\n"
"	// Merge all samples\n"
"	//--------------------------------------------------------------------------\n"
"\n"
"	SampleResult result;\n"
"	SR_Init(&result);\n"
"\n"
"	for (uint i = 0; i < PARAM_AA_SAMPLES * PARAM_AA_SAMPLES; ++i)\n"
"		SR_Accumulate(&sampleTasks[i].result, &result);\n"
"	SR_Normalize(&result, 1.f / (PARAM_AA_SAMPLES * PARAM_AA_SAMPLES));\n"
"\n"
"	// I have to save result in __global space in order to be able\n"
"	// to use Film_AddSample(). OpenCL can be so stupid some time...\n"
"	sampleTasks[0].result = result;\n"
"	Film_AddSample(sampleX, sampleY, &sampleTasks[0].result, PARAM_AA_SAMPLES * PARAM_AA_SAMPLES\n"
"			FILM_PARAM);\n"
"}\n"
; } }
