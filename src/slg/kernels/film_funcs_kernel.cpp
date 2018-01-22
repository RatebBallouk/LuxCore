#include <string>
namespace slg { namespace ocl {
std::string KernelSource_film_funcs = 
"#line 2 \"film_funcs.cl\"\n"
"\n"
"/***************************************************************************\n"
" * Copyright 1998-2018 by authors (see AUTHORS.txt)                        *\n"
" *                                                                         *\n"
" *   This file is part of LuxCoreRender.                                   *\n"
" *                                                                         *\n"
" * Licensed under the Apache License, Version 2.0 (the \"License\");         *\n"
" * you may not use this file except in compliance with the License.        *\n"
" * You may obtain a copy of the License at                                 *\n"
" *                                                                         *\n"
" *     http://www.apache.org/licenses/LICENSE-2.0                          *\n"
" *                                                                         *\n"
" * Unless required by applicable law or agreed to in writing, software     *\n"
" * distributed under the License is distributed on an \"AS IS\" BASIS,       *\n"
" * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*\n"
" * See the License for the specific language governing permissions and     *\n"
" * limitations under the License.                                          *\n"
" ***************************************************************************/\n"
"\n"
"void Film_GetSampleXY(const float u0, const float u1, float *filmX, float *filmY,\n"
"		const uint filmWidth, const uint filmHeight,\n"
"		const uint filmSubRegion0, const uint filmSubRegion1,\n"
"		const uint filmSubRegion2, const uint filmSubRegion3) {\n"
"	*filmX = filmSubRegion0 + u0 * (filmSubRegion1 - filmSubRegion0 + 1);\n"
"	*filmY = filmSubRegion2 + u1 * (filmSubRegion3 - filmSubRegion2 + 1);\n"
"}\n"
"\n"
"void Film_SetPixel2(__global float *dst, __global  float *val) {\n"
"	dst[0] = val[0];\n"
"	dst[1] = val[1];\n"
"}\n"
"\n"
"void Film_SetPixel3(__global float *dst, __global  float *val) {\n"
"	dst[0] = val[0];\n"
"	dst[1] = val[1];\n"
"	dst[2] = val[2];\n"
"}\n"
"\n"
"bool Film_MinPixel(__global float *dst, const float val) {\n"
"#if defined(PARAM_USE_PIXEL_ATOMICS)\n"
"	return AtomicMin(&dst[0], val);\n"
"#else\n"
"	if (val < dst[0]) {\n"
"		dst[0] = val;\n"
"		return true;\n"
"	} else\n"
"		return false;\n"
"#endif\n"
"}\n"
"\n"
"void Film_AddPixelVal(__global float *dst, const float val) {\n"
"#if defined(PARAM_USE_PIXEL_ATOMICS)\n"
"	AtomicAdd(&dst[0], val);\n"
"#else\n"
"	dst[0] += val;\n"
"#endif\n"
"}\n"
"\n"
"void Film_AddWeightedPixel2Val(__global float *dst, const float val, const float weight) {\n"
"#if defined(PARAM_USE_PIXEL_ATOMICS)\n"
"	AtomicAdd(&dst[0], val * weight);\n"
"	AtomicAdd(&dst[1], weight);\n"
"#else\n"
"	dst[0] += val * weight;\n"
"	dst[1] += weight;\n"
"#endif\n"
"}\n"
"\n"
"void Film_AddWeightedPixel2(__global float *dst, __global float *val, const float weight) {\n"
"#if defined(PARAM_USE_PIXEL_ATOMICS)\n"
"	AtomicAdd(&dst[0], val[0] * weight);\n"
"	AtomicAdd(&dst[1], weight);\n"
"#else\n"
"	dst[0] += val[0] * weight;\n"
"	dst[1] += weight;\n"
"#endif\n"
"}\n"
"\n"
"void Film_AddWeightedPixel4Val(__global float *dst, float3 val, const float weight) {\n"
"	const float r = val.s0;\n"
"	const float g = val.s1;\n"
"	const float b = val.s2;\n"
"\n"
"	if (!isnan(r) && !isinf(r) &&\n"
"			!isnan(g) && !isinf(g) &&\n"
"			!isnan(b) && !isinf(b) &&\n"
"			!isnan(weight) && !isinf(weight)) {\n"
"#if defined(PARAM_USE_PIXEL_ATOMICS)\n"
"		AtomicAdd(&dst[0], r * weight);\n"
"		AtomicAdd(&dst[1], g * weight);\n"
"		AtomicAdd(&dst[2], b * weight);\n"
"		AtomicAdd(&dst[3], weight);\n"
"#else\n"
"		float4 p = VLOAD4F(dst);\n"
"		const float4 s = (float4)(r * weight, g * weight, b * weight, weight);\n"
"		p += s;\n"
"		VSTORE4F(p, dst);\n"
"#endif\n"
"	} /*else {\n"
"		printf(\"NaN/Inf. error: (%f, %f, %f) [%f]\\n\", r, g, b, weight);\n"
"	}*/\n"
"}\n"
"\n"
"void Film_AddWeightedPixel4(__global float *dst, __global float *val, const float weight) {\n"
"	const float r = val[0];\n"
"	const float g = val[1];\n"
"	const float b = val[2];\n"
"\n"
"	if (!isnan(r) && !isinf(r) &&\n"
"			!isnan(g) && !isinf(g) &&\n"
"			!isnan(b) && !isinf(b) &&\n"
"			!isnan(weight) && !isinf(weight)) {\n"
"#if defined(PARAM_USE_PIXEL_ATOMICS)\n"
"		AtomicAdd(&dst[0], r * weight);\n"
"		AtomicAdd(&dst[1], g * weight);\n"
"		AtomicAdd(&dst[2], b * weight);\n"
"		AtomicAdd(&dst[3], weight);\n"
"#else\n"
"		float4 p = VLOAD4F(dst);\n"
"		const float4 s = (float4)(r * weight, g * weight, b * weight, weight);\n"
"		p += s;\n"
"		VSTORE4F(p, dst);\n"
"#endif\n"
"	} /*else {\n"
"		printf(\"NaN/Inf. error: (%f, %f, %f) [%f]\\n\", r, g, b, weight);\n"
"	}*/\n"
"}\n"
"\n"
"void Film_AddSampleResultColor(const uint x, const uint y,\n"
"		__global SampleResult *sampleResult, const float weight\n"
"		FILM_PARAM_DECL) {\n"
"	const uint index1 = x + y * filmWidth;\n"
"	const uint index2 = index1 * 2;\n"
"	const uint index4 = index1 * 4;\n"
"\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_0)\n"
"	Film_AddWeightedPixel4(&((filmRadianceGroup[0])[index4]), sampleResult->radiancePerPixelNormalized[0].c, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_1)\n"
"	Film_AddWeightedPixel4(&((filmRadianceGroup[1])[index4]), sampleResult->radiancePerPixelNormalized[1].c, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_2)\n"
"	Film_AddWeightedPixel4(&((filmRadianceGroup[2])[index4]), sampleResult->radiancePerPixelNormalized[2].c, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_3)\n"
"	Film_AddWeightedPixel4(&((filmRadianceGroup[3])[index4]), sampleResult->radiancePerPixelNormalized[3].c, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_4)\n"
"	Film_AddWeightedPixel4(&((filmRadianceGroup[4])[index4]), sampleResult->radiancePerPixelNormalized[4].c, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_5)\n"
"	Film_AddWeightedPixel4(&((filmRadianceGroup[5])[index4]), sampleResult->radiancePerPixelNormalized[5].c, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_6)\n"
"	Film_AddWeightedPixel4(&((filmRadianceGroup[6])[index4]), sampleResult->radiancePerPixelNormalized[6].c, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_7)\n"
"	Film_AddWeightedPixel4(&((filmRadianceGroup[7])[index4]), sampleResult->radiancePerPixelNormalized[7].c, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_ALPHA)\n"
"	Film_AddWeightedPixel2(&filmAlpha[index2], &sampleResult->alpha, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_DIFFUSE)\n"
"	Film_AddWeightedPixel4(&filmDirectDiffuse[index4], sampleResult->directDiffuse.c, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_GLOSSY)\n"
"	Film_AddWeightedPixel4(&filmDirectGlossy[index4], sampleResult->directGlossy.c, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_EMISSION)\n"
"	Film_AddWeightedPixel4(&filmEmission[index4], sampleResult->emission.c, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_DIFFUSE)\n"
"	Film_AddWeightedPixel4(&filmIndirectDiffuse[index4], sampleResult->indirectDiffuse.c, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_GLOSSY)\n"
"	Film_AddWeightedPixel4(&filmIndirectGlossy[index4], sampleResult->indirectGlossy.c, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_SPECULAR)\n"
"	Film_AddWeightedPixel4(&filmIndirectSpecular[index4], sampleResult->indirectSpecular.c, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_MATERIAL_ID_MASK)\n"
"	const float materialIDMask = (sampleResult->materialID == PARAM_FILM_MASK_MATERIAL_ID) ? 1.f : 0.f;\n"
"	Film_AddWeightedPixel2Val(&filmMaterialIDMask[index2], materialIDMask, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_SHADOW_MASK)\n"
"	Film_AddWeightedPixel2(&filmDirectShadowMask[index2], &sampleResult->directShadowMask, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_SHADOW_MASK)\n"
"	Film_AddWeightedPixel2(&filmIndirectShadowMask[index2], &sampleResult->indirectShadowMask, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_BY_MATERIAL_ID)\n"
"	float3 byMaterialIDColor = BLACK;\n"
"	\n"
"	if (sampleResult->materialID == PARAM_FILM_BY_MATERIAL_ID) {\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_0)\n"
"		byMaterialIDColor += VLOAD3F(sampleResult->radiancePerPixelNormalized[0].c);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_1)\n"
"		byMaterialIDColor += VLOAD3F(sampleResult->radiancePerPixelNormalized[1].c);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_2)\n"
"		byMaterialIDColor += VLOAD3F(sampleResult->radiancePerPixelNormalized[2].c);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_3)\n"
"		byMaterialIDColor += VLOAD3F(sampleResult->radiancePerPixelNormalized[3].c);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_4)\n"
"		byMaterialIDColor += VLOAD3F(sampleResult->radiancePerPixelNormalized[4].c);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_5)\n"
"		byMaterialIDColor += VLOAD3F(sampleResult->radiancePerPixelNormalized[5].c);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_6)\n"
"		byMaterialIDColor += VLOAD3F(sampleResult->radiancePerPixelNormalized[6].c);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_7)\n"
"		byMaterialIDColor += VLOAD3F(sampleResult->radiancePerPixelNormalized[7].c);\n"
"#endif\n"
"	}\n"
"	Film_AddWeightedPixel4Val(&filmByMaterialID[index4], byMaterialIDColor, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_IRRADIANCE)\n"
"	Film_AddWeightedPixel4(&filmIrradiance[index4], sampleResult->irradiance.c, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_OBJECT_ID_MASK)\n"
"	const float objectIDMask = (sampleResult->objectID == PARAM_FILM_MASK_OBJECT_ID) ? 1.f : 0.f;\n"
"	Film_AddWeightedPixel2Val(&filmObjectIDMask[index2], objectIDMask, weight);\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_BY_OBJECT_ID)\n"
"	float3 byObjectIDColor = BLACK;\n"
"	\n"
"	if (sampleResult->objectID == PARAM_FILM_BY_OBJECT_ID) {\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_0)\n"
"		byObjectIDColor += VLOAD3F(sampleResult->radiancePerPixelNormalized[0].c);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_1)\n"
"		byObjectIDColor += VLOAD3F(sampleResult->radiancePerPixelNormalized[1].c);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_2)\n"
"		byObjectIDColor += VLOAD3F(sampleResult->radiancePerPixelNormalized[2].c);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_3)\n"
"		byObjectIDColor += VLOAD3F(sampleResult->radiancePerPixelNormalized[3].c);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_4)\n"
"		byObjectIDColor += VLOAD3F(sampleResult->radiancePerPixelNormalized[4].c);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_5)\n"
"		byObjectIDColor += VLOAD3F(sampleResult->radiancePerPixelNormalized[5].c);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_6)\n"
"		byObjectIDColor += VLOAD3F(sampleResult->radiancePerPixelNormalized[6].c);\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_7)\n"
"		byObjectIDColor += VLOAD3F(sampleResult->radiancePerPixelNormalized[7].c);\n"
"#endif\n"
"	}\n"
"	Film_AddWeightedPixel4Val(&filmByObjectID[index4], byObjectIDColor, weight);\n"
"#endif\n"
"}\n"
"\n"
"void Film_AddSampleResultData(const uint x, const uint y,\n"
"		__global SampleResult *sampleResult\n"
"		FILM_PARAM_DECL) {\n"
"	const uint index1 = x + y * filmWidth;\n"
"	const uint index2 = index1 * 2;\n"
"	const uint index3 = index1 * 3;\n"
"\n"
"	bool depthWrite = true;\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DEPTH)\n"
"	depthWrite = Film_MinPixel(&filmDepth[index1], sampleResult->depth);\n"
"#endif\n"
"\n"
"	if (depthWrite) {\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_POSITION)\n"
"		Film_SetPixel3(&filmPosition[index3], &sampleResult->position.x);\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_GEOMETRY_NORMAL)\n"
"		Film_SetPixel3(&filmGeometryNormal[index3], &sampleResult->geometryNormal.x);\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_SHADING_NORMAL)\n"
"		Film_SetPixel3(&filmShadingNormal[index3], &sampleResult->shadingNormal.x);\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_MATERIAL_ID)\n"
"		filmMaterialID[index1] = sampleResult->materialID;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_UV)\n"
"		Film_SetPixel2(&filmUV[index2], &sampleResult->uv.u);\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_OBJECT_ID)\n"
"		const uint objectID = sampleResult->objectID;\n"
"		if (objectID != NULL_INDEX)\n"
"			filmObjectID[index1] = sampleResult->objectID;\n"
"#endif\n"
"	}\n"
"\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_RAYCOUNT)\n"
"	Film_AddPixelVal(&filmRayCount[index1], sampleResult->rayCount);\n"
"#endif\n"
"}\n"
"\n"
"void Film_AddSample(const uint x, const uint y,\n"
"		__global SampleResult *sampleResult, const float weight\n"
"		FILM_PARAM_DECL) {\n"
"	Film_AddSampleResultColor(x, y, sampleResult, weight\n"
"			FILM_PARAM);\n"
"	Film_AddSampleResultData(x, y, sampleResult\n"
"			FILM_PARAM);\n"
"}\n"
"\n"
"#if (PARAM_IMAGE_FILTER_TYPE == 0)\n"
"\n"
"#elif (PARAM_IMAGE_FILTER_TYPE == 1) || (PARAM_IMAGE_FILTER_TYPE == 2) || (PARAM_IMAGE_FILTER_TYPE == 3) || (PARAM_IMAGE_FILTER_TYPE == 4) || (PARAM_IMAGE_FILTER_TYPE == 5)\n"
"\n"
"void Film_AddSampleFilteredResultColor(const int x, const int y,\n"
"		const float distX, const float distY,\n"
"		__global SampleResult *sampleResult, const float weight\n"
"		FILM_PARAM_DECL) {\n"
"	if ((x >= 0) && (x < (int)filmWidth) && (y >= 0) && (y < (int)filmHeight)) {\n"
"		const float filterWeight = ImageFilter_Evaluate(distX, distY);\n"
"\n"
"		Film_AddSampleResultColor(x, y, sampleResult, weight * filterWeight\n"
"			FILM_PARAM);\n"
"	}\n"
"}\n"
"\n"
"#else\n"
"\n"
"Error: unknown image filter !!!\n"
"\n"
"#endif\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Film_Clear Kernel\n"
"//------------------------------------------------------------------------------\n"
"\n"
"__kernel __attribute__((work_group_size_hint(64, 1, 1))) void Film_Clear(\n"
"		const uint filmWidth, const uint filmHeight,\n"
"		const uint filmSubRegion0, const uint filmSubRegion1,\n"
"		const uint filmSubRegion2, const uint filmSubRegion3\n"
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
"#if defined(PARAM_FILM_CHANNELS_HAS_BY_MATERIAL_ID)\n"
"		, __global float *filmByMaterialID\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_IRRADIANCE)\n"
"		, __global float *filmIrradiance\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_OBJECT_ID)\n"
"		, __global uint *filmObjectID\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_OBJECT_ID_MASK)\n"
"		, __global float *filmObjectIDMask\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_BY_OBJECT_ID)\n"
"		, __global float *filmByObjectID\n"
"#endif\n"
"		) {\n"
"	const size_t gid = get_global_id(0);\n"
"	if (gid >= filmWidth * filmHeight)\n"
"		return;\n"
"\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_0)\n"
"	filmRadianceGroup0[gid * 4] = 0.f;\n"
"	filmRadianceGroup0[gid * 4 + 1] = 0.f;\n"
"	filmRadianceGroup0[gid * 4 + 2] = 0.f;\n"
"	filmRadianceGroup0[gid * 4 + 3] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_1)\n"
"	filmRadianceGroup1[gid * 4] = 0.f;\n"
"	filmRadianceGroup1[gid * 4 + 1] = 0.f;\n"
"	filmRadianceGroup1[gid * 4 + 2] = 0.f;\n"
"	filmRadianceGroup1[gid * 4 + 3] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_2)\n"
"	filmRadianceGroup2[gid * 4] = 0.f;\n"
"	filmRadianceGroup2[gid * 4 + 1] = 0.f;\n"
"	filmRadianceGroup2[gid * 4 + 2] = 0.f;\n"
"	filmRadianceGroup2[gid * 4 + 3] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_3)\n"
"	filmRadianceGroup3[gid * 4] = 0.f;\n"
"	filmRadianceGroup3[gid * 4 + 1] = 0.f;\n"
"	filmRadianceGroup3[gid * 4 + 2] = 0.f;\n"
"	filmRadianceGroup3[gid * 4 + 3] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_4)\n"
"	filmRadianceGroup4[gid * 4] = 0.f;\n"
"	filmRadianceGroup4[gid * 4 + 1] = 0.f;\n"
"	filmRadianceGroup4[gid * 4 + 2] = 0.f;\n"
"	filmRadianceGroup4[gid * 4 + 3] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_5)\n"
"	filmRadianceGroup5[gid * 4] = 0.f;\n"
"	filmRadianceGroup5[gid * 4 + 1] = 0.f;\n"
"	filmRadianceGroup5[gid * 4 + 2] = 0.f;\n"
"	filmRadianceGroup5[gid * 4 + 3] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_6)\n"
"	filmRadianceGroup6[gid * 4] = 0.f;\n"
"	filmRadianceGroup6[gid * 4 + 1] = 0.f;\n"
"	filmRadianceGroup6[gid * 4 + 2] = 0.f;\n"
"	filmRadianceGroup6[gid * 4 + 3] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_7)\n"
"	filmRadianceGroup7[gid * 4] = 0.f;\n"
"	filmRadianceGroup7[gid * 4 + 1] = 0.f;\n"
"	filmRadianceGroup7[gid * 4 + 2] = 0.f;\n"
"	filmRadianceGroup7[gid * 4 + 3] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_ALPHA)\n"
"	filmAlpha[gid * 2] = 0.f;\n"
"	filmAlpha[gid * 2 + 1] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DEPTH)\n"
"	filmDepth[gid] = INFINITY;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_POSITION)\n"
"	filmPosition[gid * 3] = INFINITY;\n"
"	filmPosition[gid * 3 + 1] = INFINITY;\n"
"	filmPosition[gid * 3 + 2] = INFINITY;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_GEOMETRY_NORMAL)\n"
"	filmGeometryNormal[gid * 3] = INFINITY;\n"
"	filmGeometryNormal[gid * 3 + 1] = INFINITY;\n"
"	filmGeometryNormal[gid * 3 + 2] = INFINITY;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_SHADING_NORMAL)\n"
"	filmShadingNormal[gid * 3] = INFINITY;\n"
"	filmShadingNormal[gid * 3 + 1] = INFINITY;\n"
"	filmShadingNormal[gid * 3 + 2] = INFINITY;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_MATERIAL_ID)\n"
"	filmMaterialID[gid] = NULL_INDEX;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_DIFFUSE)\n"
"	filmDirectDiffuse[gid * 4] = 0.f;\n"
"	filmDirectDiffuse[gid * 4 + 1] = 0.f;\n"
"	filmDirectDiffuse[gid * 4 + 2] = 0.f;\n"
"	filmDirectDiffuse[gid * 4 + 3] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_GLOSSY)\n"
"	filmDirectGlossy[gid * 4] = 0.f;\n"
"	filmDirectGlossy[gid * 4 + 1] = 0.f;\n"
"	filmDirectGlossy[gid * 4 + 2] = 0.f;\n"
"	filmDirectGlossy[gid * 4 + 3] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_EMISSION)\n"
"	filmEmission[gid * 4] = 0.f;\n"
"	filmEmission[gid * 4 + 1] = 0.f;\n"
"	filmEmission[gid * 4 + 2] = 0.f;\n"
"	filmEmission[gid * 4 + 3] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_DIFFUSE)\n"
"	filmIndirectDiffuse[gid * 4] = 0.f;\n"
"	filmIndirectDiffuse[gid * 4 + 1] = 0.f;\n"
"	filmIndirectDiffuse[gid * 4 + 2] = 0.f;\n"
"	filmIndirectDiffuse[gid * 4 + 3] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_GLOSSY)\n"
"	filmIndirectGlossy[gid * 4] = 0.f;\n"
"	filmIndirectGlossy[gid * 4 + 1] = 0.f;\n"
"	filmIndirectGlossy[gid * 4 + 2] = 0.f;\n"
"	filmIndirectGlossy[gid * 4 + 3] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_SPECULAR)\n"
"	filmIndirectSpecular[gid * 4] = 0.f;\n"
"	filmIndirectSpecular[gid * 4 + 1] = 0.f;\n"
"	filmIndirectSpecular[gid * 4 + 2] = 0.f;\n"
"	filmIndirectSpecular[gid * 4 + 3] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_MATERIAL_ID_MASK)\n"
"	filmMaterialIDMask[gid * 2] = 0.f;\n"
"	filmMaterialIDMask[gid * 2 + 1] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_SHADOW_MASK)\n"
"	filmDirectShadowMask[gid * 2] = 0.f;\n"
"	filmDirectShadowMask[gid * 2 + 1] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_SHADOW_MASK)\n"
"	filmIndirectShadowMask[gid * 2] = 0.f;\n"
"	filmIndirectShadowMask[gid * 2 + 1] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_UV)\n"
"	filmUV[gid * 2] = INFINITY;\n"
"	filmUV[gid * 2 + 1] = INFINITY;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_RAYCOUNT)\n"
"	filmRayCount[gid] = 0;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_BY_MATERIAL_ID)\n"
"	filmByMaterialID[gid * 4] = 0.f;\n"
"	filmByMaterialID[gid * 4 + 1] = 0.f;\n"
"	filmByMaterialID[gid * 4 + 2] = 0.f;\n"
"	filmByMaterialID[gid * 4 + 3] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_IRRADIANCE)\n"
"	filmIrradiance[gid * 4] = 0.f;\n"
"	filmIrradiance[gid * 4 + 1] = 0.f;\n"
"	filmIrradiance[gid * 4 + 2] = 0.f;\n"
"	filmIrradiance[gid * 4 + 3] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_OBJECT_ID)\n"
"	filmObjectID[gid] = NULL_INDEX;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_OBJECT_ID_MASK)\n"
"	filmObjectIDMask[gid * 2] = 0.f;\n"
"	filmObjectIDMask[gid * 2 + 1] = 0.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_BY_OBJECT_ID)\n"
"	filmByObjectID[gid * 4] = 0.f;\n"
"	filmByObjectID[gid * 4 + 1] = 0.f;\n"
"	filmByObjectID[gid * 4 + 2] = 0.f;\n"
"	filmByObjectID[gid * 4 + 3] = 0.f;\n"
"#endif\n"
"}\n"
; } }
