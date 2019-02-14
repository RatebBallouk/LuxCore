/***************************************************************************
 * Copyright 1998-2018 by authors (see AUTHORS.txt)                        *
 *                                                                         *
 *   This file is part of LuxCoreRender.                                   *
 *                                                                         *
 * Licensed under the Apache License, Version 2.0 (the "License");         *
 * you may not use this file except in compliance with the License.        *
 * You may obtain a copy of the License at                                 *
 *                                                                         *
 *     http://www.apache.org/licenses/LICENSE-2.0                          *
 *                                                                         *
 * Unless required by applicable law or agreed to in writing, software     *
 * distributed under the License is distributed on an "AS IS" BASIS,       *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
 * See the License for the specific language governing permissions and     *
 * limitations under the License.                                          *
 ***************************************************************************/

#include <boost/format.hpp>

#if defined(_OPENMP)
#include <omp.h>
#endif

#include "slg/samplers/sobol.h"
#include "slg/utils/pathdepthinfo.h"
#include "slg/engines/caches/photongi/photongicache.h"
#include "slg/engines/caches/photongi/tracephotonsthread.h"
#include "slg/engines/caches/photongi/tracevisibilitythread.h"

using namespace std;
using namespace luxrays;
using namespace slg;

// TODO: serialization

//------------------------------------------------------------------------------
// PhotonGICache
//------------------------------------------------------------------------------

PhotonGICache::PhotonGICache(const Scene *scn, const PhotonGICacheParams &p) :
		scene(scn), params(p),
		samplerSharedData(nullptr),
		indirectPhotonTracedCount(0),
		causticPhotonTracedCount(0),
		visibilitySobolSharedData(131, nullptr),
		visibilityParticlesOctree(nullptr),
		causticPhotonsBVH(nullptr),
		radiancePhotonsBVH(nullptr) {
	if (!params.indirect.enabled)
		params.indirect.maxSize = 0;

	if (!params.caustic.enabled)
		params.caustic.maxSize = 0;

	if (params.indirect.enabled) {
		if (params.caustic.enabled) {
			params.visibility.lookUpRadius = Min(params.indirect.lookUpRadius, params.caustic.lookUpRadius) * .95f;
			params.visibility.lookUpNormalAngle = Min(params.indirect.lookUpNormalAngle, params.caustic.lookUpNormalAngle) * .95f;
		} else {
			params.visibility.lookUpRadius = params.indirect.lookUpRadius * .95f;
			params.visibility.lookUpNormalAngle = params.indirect.lookUpNormalAngle * .95f;
		}
	} else {
		if (params.caustic.enabled) {
			params.visibility.lookUpRadius = params.caustic.lookUpRadius * .95f;
			params.visibility.lookUpNormalAngle = params.caustic.lookUpNormalAngle * .95f;
		} else
			throw runtime_error("Indirect and/or caustic cache must be enabled in PhotonGI");
	}
	
	params.visibility.lookUpRadius2 = params.visibility.lookUpRadius * params.visibility.lookUpRadius;
	params.indirect.lookUpRadius2 = params.indirect.lookUpRadius * params.indirect.lookUpRadius;
	params.caustic.lookUpRadius2 = params.caustic.lookUpRadius * params.caustic.lookUpRadius;
}

PhotonGICache::~PhotonGICache() {
	delete samplerSharedData;
	
	delete visibilityParticlesOctree;

	delete causticPhotonsBVH;
	delete radiancePhotonsBVH;
}

bool PhotonGICache::IsPhotonGIEnabled(const BSDF &bsdf) const {
	const MaterialType materialType = bsdf.GetMaterialType();
	if (((materialType == GLOSSY2) && (bsdf.GetGlossiness() >= params.indirect.glossinessUsageThreshold)) ||
			(materialType == MATTE) ||
			(materialType == ROUGHMATTE))
		return bsdf.IsPhotonGIEnabled();
	else
		return false;
}

float PhotonGICache::GetIndirectUsageThreshold(const BSDFEvent lastBSDFEvent, const float lastGlossiness) const {
	// Decide if the glossy surface is "nearly specular"

	if ((lastBSDFEvent & GLOSSY) && (lastGlossiness < params.indirect.glossinessUsageThreshold)) {
		// Disable the cache, the surface is "nearly specular"
		return numeric_limits<float>::infinity();
	} else
		return params.indirect.usageThresholdScale * params.indirect.lookUpRadius;
}

bool PhotonGICache::IsDirectLightHitVisible(const bool causticCacheAlreadyUsed,
		const BSDFEvent lastBSDFEvent) const {
	return !params.caustic.enabled ||
		(!causticCacheAlreadyUsed && (params.debugType == PGIC_DEBUG_NONE));
}

void PhotonGICache::TraceVisibilityParticles() {
	const size_t renderThreadCount = boost::thread::hardware_concurrency();
	vector<TraceVisibilityThread *> renderThreads(renderThreadCount, nullptr);
	SLG_LOG("PhotonGI trace visibility particles thread count: " << renderThreads.size());

	// Initialize the Octree where to store the visibility points
	visibilityParticlesOctree = new PGCIOctree(visibilityParticles, scene->dataSet->GetBBox(),
			params.visibility.lookUpRadius, params.visibility.lookUpNormalAngle);

	globalVisibilityParticlesCount = 0;
	visibilityCacheLookUp = 0;
	visibilityCacheHits = 0;
	visibilityWarmUp = true;

	// Create the visibility particles tracing threads
	for (size_t i = 0; i < renderThreads.size(); ++i)
		renderThreads[i] = new TraceVisibilityThread(*this, i);

	// Start visibility particles tracing threads
	for (size_t i = 0; i < renderThreads.size(); ++i)
		renderThreads[i]->Start();
	
	// Wait for the end of visibility particles tracing threads
	for (size_t i = 0; i < renderThreads.size(); ++i) {
		renderThreads[i]->Join();

		delete renderThreads[i];
	}

	visibilityParticles.shrink_to_fit();
	SLG_LOG("PhotonGI visibility total entries: " << visibilityParticles.size());
}

void PhotonGICache::TracePhotons() {
	const size_t renderThreadCount = boost::thread::hardware_concurrency();
	vector<TracePhotonsThread *> renderThreads(renderThreadCount, nullptr);
	SLG_LOG("PhotonGI trace photons thread count: " << renderThreads.size());
	
	globalPhotonsCounter = 0;
	globalIndirectPhotonsTraced = 0;
	globalCausticPhotonsTraced = 0;
	globalIndirectSize = 0;
	globalCausticSize = 0;

	// Create the photon tracing threads
	for (size_t i = 0; i < renderThreads.size(); ++i)
		renderThreads[i] = new TracePhotonsThread(*this, i);

	// Start photon tracing threads
	for (size_t i = 0; i < renderThreads.size(); ++i)
		renderThreads[i]->Start();
	
	// Wait for the end of photon tracing threads
	u_int indirectPhotonStored = 0;
	for (size_t i = 0; i < renderThreads.size(); ++i) {
		renderThreads[i]->Join();

		// Copy all photons
		for (auto const &p : renderThreads[i]->indirectPhotons) {
			VisibilityParticle &vp = visibilityParticles[p.visibilityParticelIndex];

			vp.alphaAccumulated += p.alpha;
		}
		indirectPhotonStored += renderThreads[i]->indirectPhotons.size();

		causticPhotons.insert(causticPhotons.end(), renderThreads[i]->causticPhotons.begin(),
				renderThreads[i]->causticPhotons.end());

		delete renderThreads[i];
	}

	indirectPhotonTracedCount = globalIndirectPhotonsTraced;
	causticPhotonTracedCount = globalCausticPhotonsTraced;
	
	causticPhotons.shrink_to_fit();

	// globalPhotonsCounter isn't exactly the number: there is an error due
	// last bucket of work likely being smaller than work bucket size
	SLG_LOG("PhotonGI total photon traced: " << globalPhotonsCounter);
	SLG_LOG("PhotonGI total indirect photon stored: " << indirectPhotonStored <<
			" (" << indirectPhotonTracedCount << " traced)");
	SLG_LOG("PhotonGI total caustic photon stored: " << causticPhotons.size() <<
			" (" << causticPhotonTracedCount << " traced)");
}

void PhotonGICache::CreateRadiancePhotons() {
	// Create a radiance map entry for each visibility entry

	for (auto const &vp : visibilityParticles) {
		// Using here params.visibility.lookUpRadius2 would be more correct. However
		// params.visibility.lookUpRadius2 is usually jut 95% of params.indirect.lookUpRadius2.
		const Spectrum outgoingRadiance = (vp.bsdfEvaluateTotal * INV_PI) *
				vp.alphaAccumulated /
				(indirectPhotonTracedCount * params.indirect.lookUpRadius2 * M_PI);

		assert (!outgoingRadiance.IsNaN() && !outgoingRadiance.IsInf());

		if (!outgoingRadiance.Black())
			radiancePhotons.push_back(RadiancePhoton(vp.p,
					vp.n, outgoingRadiance));
	}

	radiancePhotons.shrink_to_fit();
	
	SLG_LOG("PhotonGI total radiance photon stored: " << radiancePhotons.size());
}

void PhotonGICache::Preprocess() {
	//--------------------------------------------------------------------------
	// Trace visibility particles
	//--------------------------------------------------------------------------

	TraceVisibilityParticles();

	//--------------------------------------------------------------------------
	// Fill all photon vectors
	//--------------------------------------------------------------------------

	TracePhotons();

	//--------------------------------------------------------------------------
	// Radiance photon map
	//--------------------------------------------------------------------------

	if (params.indirect.enabled) {	
		SLG_LOG("PhotonGI building radiance photon data");
		CreateRadiancePhotons();

		SLG_LOG("PhotonGI building radiance photons BVH");
		radiancePhotonsBVH = new PGICRadiancePhotonBvh(radiancePhotons,
				params.indirect.lookUpRadius, params.indirect.lookUpNormalAngle);
	}

	//--------------------------------------------------------------------------
	// Caustic photon map
	//--------------------------------------------------------------------------

	if ((causticPhotons.size() > 0) && params.caustic.enabled) {
		SLG_LOG("PhotonGI building caustic photons BVH");
		causticPhotonsBVH = new PGICPhotonBvh(causticPhotons, params.caustic.lookUpMaxCount,
				params.caustic.lookUpRadius, params.caustic.lookUpNormalAngle);
	}

	//--------------------------------------------------------------------------
	// Free visibility map
	//--------------------------------------------------------------------------

	delete visibilityParticlesOctree;
	visibilityParticlesOctree = nullptr;
	visibilityParticles.clear();
	visibilityParticles.shrink_to_fit();

	//--------------------------------------------------------------------------
	// Print some statistics about memory usage
	//--------------------------------------------------------------------------

	size_t totalMemUsage = 0;

	if (causticPhotonsBVH) {
		SLG_LOG("PhotonGI caustic cache photons memory usage: " << ToMemString(causticPhotons.size() * sizeof(Photon)));
		SLG_LOG("PhotonGI caustic cache BVH memory usage: " << ToMemString(causticPhotonsBVH->GetMemoryUsage()));

		totalMemUsage += causticPhotons.size() * sizeof(Photon) + causticPhotonsBVH->GetMemoryUsage();
	}

	if (radiancePhotonsBVH) {
		SLG_LOG("PhotonGI indirect cache photons memory usage: " << ToMemString(radiancePhotons.size() * sizeof(RadiancePhoton)));
		SLG_LOG("PhotonGI indirect cache BVH memory usage: " << ToMemString(radiancePhotonsBVH->GetMemoryUsage()));

		totalMemUsage += radiancePhotons.size() * sizeof(Photon) + radiancePhotonsBVH->GetMemoryUsage();
	}

	SLG_LOG("PhotonGI total memory usage: " << ToMemString(totalMemUsage));
}

// Simpson filter from PBRT v2. Filter the photons according their
// distance, giving more weight to the nearest.
static inline float SimpsonKernel(const Point &p1, const Point &p2,
		const float maxDist2) {
	const float dist2 = DistanceSquared(p1, p2);

	// The distance between p1 and p2 is supposed to be < maxDist2
	assert (dist2 <= maxDist2);
    const float s = (1.f - dist2 / maxDist2);

    return 3.f * INV_PI * s * s;
}

Spectrum PhotonGICache::ProcessCacheEntries(const vector<NearPhoton> &entries,
		const u_int photonTracedCount, const float maxDistance2, const BSDF &bsdf) const {
	Spectrum result;

	if (entries.size() > 0) {
		if (bsdf.GetMaterialType() == MaterialType::MATTE) {
			// A fast path for matte material

			for (auto const &nearPhoton : entries) {
				const Photon *photon = (const Photon *)nearPhoton.photon;

				// Using a Simpson filter here
				result += SimpsonKernel(bsdf.hitPoint.p, photon->p, maxDistance2) * 
						AbsDot(bsdf.hitPoint.shadeN, -photon->d) * photon->alpha;
			}
			
			result *= bsdf.EvaluateTotal() * INV_PI;
		} else {
			// Generic path

			BSDFEvent event;
			for (auto const &nearPhoton : entries) {
				const Photon *photon = (const Photon *)nearPhoton.photon;

				// Using a Simpson filter here
				result += SimpsonKernel(bsdf.hitPoint.p, photon->p, maxDistance2) *
						bsdf.Evaluate(-photon->d, &event, nullptr, nullptr) * photon->alpha;
			}
		}
	}
	
	result /= photonTracedCount * maxDistance2;

	return result;
}

Spectrum PhotonGICache::GetIndirectRadiance(const BSDF &bsdf) const {
	assert (IsPhotonGIEnabled(bsdf));

	Spectrum result;
	if (radiancePhotonsBVH) {
		// Flip the normal if required
		const Normal n = (bsdf.hitPoint.intoObject ? 1.f: -1.f) * bsdf.hitPoint.shadeN;
		const RadiancePhoton *radiancePhoton = radiancePhotonsBVH->GetNearestEntry(bsdf.hitPoint.p, n);

		if (radiancePhoton)
			result = radiancePhoton->outgoingRadiance;
	}
	
	return result;
}

Spectrum PhotonGICache::GetCausticRadiance(const BSDF &bsdf) const {
	assert (IsPhotonGIEnabled(bsdf));

	if (causticPhotonsBVH) {
		vector<NearPhoton> entries;
		entries.reserve(causticPhotonsBVH->GetEntryMaxLookUpCount());

		// Flip the normal if required
		const Normal n = (bsdf.hitPoint.intoObject ? 1.f: -1.f) * bsdf.hitPoint.shadeN;
		float maxDistance2;
		causticPhotonsBVH->GetAllNearEntries(entries, bsdf.hitPoint.p, n, maxDistance2);

		return ProcessCacheEntries(entries, causticPhotonTracedCount, maxDistance2, bsdf);
	} else
		return Spectrum();
}

PhotonGISamplerType PhotonGICache::String2SamplerType(const string &type) {
	if (type == "RANDOM")
		return PhotonGISamplerType::PGIC_SAMPLER_RANDOM;
	else if (type == "METROPOLIS")
		return PhotonGISamplerType::PGIC_SAMPLER_METROPOLIS;
	else
		throw runtime_error("Unknown PhotonGI cache debug type: " + type);
}

string PhotonGICache::SamplerType2String(const PhotonGISamplerType type) {
	switch (type) {
		case PhotonGISamplerType::PGIC_SAMPLER_RANDOM:
			return "RANDOM";
		case PhotonGISamplerType::PGIC_SAMPLER_METROPOLIS:
			return "METROPOLIS";
		default:
			throw runtime_error("Unsupported wrap type in PhotonGICache::SamplerType2String(): " + ToString(type));
	}
}

PhotonGIDebugType PhotonGICache::String2DebugType(const string &type) {
	if (type == "showindirect")
		return PhotonGIDebugType::PGIC_DEBUG_SHOWINDIRECT;
	else if (type == "showcaustic")
		return PhotonGIDebugType::PGIC_DEBUG_SHOWCAUSTIC;
	else if (type == "none")
		return PhotonGIDebugType::PGIC_DEBUG_NONE;
	else
		throw runtime_error("Unknown PhotonGI cache debug type: " + type);
}

string PhotonGICache::DebugType2String(const PhotonGIDebugType type) {
	switch (type) {
		case PhotonGIDebugType::PGIC_DEBUG_SHOWINDIRECT:
			return "showindirect";
		case PhotonGIDebugType::PGIC_DEBUG_SHOWCAUSTIC:
			return "showcaustic";
		case PhotonGIDebugType::PGIC_DEBUG_NONE:
			return "none";
		default:
			throw runtime_error("Unsupported wrap type in PhotonGICache::DebugType2String(): " + ToString(type));
	}
}

Properties PhotonGICache::ToProperties(const Properties &cfg) {
	Properties props;

	props <<
			cfg.Get(GetDefaultProps().Get("path.photongi.sampler.type")) <<
			cfg.Get(GetDefaultProps().Get("path.photongi.photon.maxcount")) <<
			cfg.Get(GetDefaultProps().Get("path.photongi.photon.maxdepth")) <<
			cfg.Get(GetDefaultProps().Get("path.photongi.visibility.targethitrate")) <<
			cfg.Get(GetDefaultProps().Get("path.photongi.visibility.maxsamplecount")) <<
			cfg.Get(GetDefaultProps().Get("path.photongi.indirect.enabled")) <<
			cfg.Get(GetDefaultProps().Get("path.photongi.indirect.maxsize")) <<
			cfg.Get(GetDefaultProps().Get("path.photongi.indirect.lookup.radius")) <<
			cfg.Get(GetDefaultProps().Get("path.photongi.indirect.lookup.normalangle")) <<
			cfg.Get(GetDefaultProps().Get("path.photongi.indirect.glossinessusagethreshold")) <<
			cfg.Get(GetDefaultProps().Get("path.photongi.indirect.usagethresholdscale")) <<
			cfg.Get(GetDefaultProps().Get("path.photongi.caustic.enabled")) <<
			cfg.Get(GetDefaultProps().Get("path.photongi.caustic.maxsize")) <<
			cfg.Get(GetDefaultProps().Get("path.photongi.caustic.lookup.maxcount")) <<
			cfg.Get(GetDefaultProps().Get("path.photongi.caustic.lookup.radius")) <<
			cfg.Get(GetDefaultProps().Get("path.photongi.caustic.lookup.normalangle")) <<
			cfg.Get(GetDefaultProps().Get("path.photongi.debug.type"));

	return props;
}

const Properties &PhotonGICache::GetDefaultProps() {
	static Properties props = Properties() <<
			Property("path.photongi.sampler.type")("METROPOLIS") <<
			Property("path.photongi.photon.maxcount")(500000) <<
			Property("path.photongi.photon.maxdepth")(4) <<
			Property("path.photongi.visibility.targethitrate")(.99f) <<
			Property("path.photongi.visibility.maxsamplecount")(1024 * 1024) <<
			Property("path.photongi.indirect.enabled")(false) <<
			Property("path.photongi.indirect.maxsize")(100000) <<
			Property("path.photongi.indirect.lookup.radius")(.15f) <<
			Property("path.photongi.indirect.lookup.normalangle")(10.f) <<
			Property("path.photongi.indirect.glossinessusagethreshold")(.2f) <<
			Property("path.photongi.indirect.usagethresholdscale")(2.f) <<
			Property("path.photongi.caustic.enabled")(false) <<
			Property("path.photongi.caustic.maxsize")(100000) <<
			Property("path.photongi.caustic.lookup.maxcount")(256) <<
			Property("path.photongi.caustic.lookup.radius")(.15f) <<
			Property("path.photongi.caustic.lookup.normalangle")(10.f) <<
			Property("path.photongi.debug.type")("none");

	return props;
}

PhotonGICache *PhotonGICache::FromProperties(const Scene *scn, const Properties &cfg) {
	PhotonGICacheParams params;

	params.indirect.enabled = cfg.Get(GetDefaultProps().Get("path.photongi.indirect.enabled")).Get<bool>();
	params.caustic.enabled = cfg.Get(GetDefaultProps().Get("path.photongi.caustic.enabled")).Get<bool>();
	
	if (params.indirect.enabled || params.caustic.enabled) {
		params.samplerType = String2SamplerType(cfg.Get(GetDefaultProps().Get("path.photongi.sampler.type")).Get<string>());

		params.photon.maxTracedCount = Max(1u, cfg.Get(GetDefaultProps().Get("path.photongi.photon.maxcount")).Get<u_int>());
		params.photon.maxPathDepth = Max(1u, cfg.Get(GetDefaultProps().Get("path.photongi.photon.maxdepth")).Get<u_int>());

		params.visibility.targetHitRate = cfg.Get(GetDefaultProps().Get("path.photongi.visibility.targethitrate")).Get<float>();
		params.visibility.maxSampleCount = cfg.Get(GetDefaultProps().Get("path.photongi.visibility.maxsamplecount")).Get<u_int>();

		if (params.indirect.enabled) {
			params.indirect.maxSize = Max(0u, cfg.Get(GetDefaultProps().Get("path.photongi.indirect.maxsize")).Get<u_int>());

			params.indirect.lookUpRadius = Max(DEFAULT_EPSILON_MIN, cfg.Get(GetDefaultProps().Get("path.photongi.indirect.lookup.radius")).Get<float>());
			params.indirect.lookUpNormalAngle = Max(DEFAULT_EPSILON_MIN, cfg.Get(GetDefaultProps().Get("path.photongi.indirect.lookup.normalangle")).Get<float>());

			params.indirect.glossinessUsageThreshold = Max(0.f, cfg.Get(GetDefaultProps().Get("path.photongi.indirect.glossinessusagethreshold")).Get<float>());
			params.indirect.usageThresholdScale = Max(0.f, cfg.Get(GetDefaultProps().Get("path.photongi.indirect.usagethresholdscale")).Get<float>());
		}

		if (params.caustic.enabled) {
			params.caustic.maxSize = Max(0u, cfg.Get(GetDefaultProps().Get("path.photongi.caustic.maxsize")).Get<u_int>());

			params.caustic.lookUpMaxCount = Max(1u, cfg.Get(GetDefaultProps().Get("path.photongi.caustic.lookup.maxcount")).Get<u_int>());
			params.caustic.lookUpRadius = Max(DEFAULT_EPSILON_MIN, cfg.Get(GetDefaultProps().Get("path.photongi.caustic.lookup.radius")).Get<float>());
			params.caustic.lookUpNormalAngle = Max(DEFAULT_EPSILON_MIN, cfg.Get(GetDefaultProps().Get("path.photongi.caustic.lookup.normalangle")).Get<float>());
		}

		params.debugType = String2DebugType(cfg.Get(GetDefaultProps().Get("path.photongi.debug.type")).Get<string>());

		return new PhotonGICache(scn, params);
	} else
		return nullptr;
}