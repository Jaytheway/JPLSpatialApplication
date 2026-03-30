//
//      ██╗██████╗     ██╗     ██╗██████╗ ███████╗
//      ██║██╔══██╗    ██║     ██║██╔══██╗██╔════╝		** JPL Spatial Application **
//      ██║██████╔╝    ██║     ██║██████╔╝███████╗
// ██   ██║██╔═══╝     ██║     ██║██╔══██╗╚════██║		https://github.com/Jaytheway/JPLSpatialApplication
// ╚█████╔╝██║         ███████╗██║██████╔╝███████║
//  ╚════╝ ╚═╝         ╚══════╝╚═╝╚═════╝ ╚══════╝
//
//   Copyright Jaroslav Pevno, JPL Spatial Application is offered under the terms of the ISC license:
//
//   Permission to use, copy, modify, and/or distribute this software for any purpose with or
//   without fee is hereby granted, provided that the above copyright notice and this permission
//   notice appear in all copies. THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
//   AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
//   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
//   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
//   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include "ERTracing.h"

namespace JPL
{
	static const std::array<JPL::MinimalVec3, 6> cSurfaces{
		JPL::MinimalVec3{ -1, 0, 0 },
		JPL::MinimalVec3{ 1, 0, 0 },
		JPL::MinimalVec3{ 0,-1, 0 },
		JPL::MinimalVec3{ 0, 1, 0 },
		JPL::MinimalVec3{ 0, 0,-1 },
		JPL::MinimalVec3{ 0, 0, 1 }
	};

	bool ERTracer::IntersectImpl(const Ray& ray, Intersection& outIntersection, float maxDistance) const
	{
		const auto hit = mRoom.CastRay(ray.Origin, ray.Direction, maxDistance);
		if (hit.bHit)
		{
			outIntersection.Normal = DotProduct(ray.Direction, hit.Normal) > 0.0f ? -hit.Normal : hit.Normal;
			outIntersection.Position = hit.Position;
			outIntersection.Material = 1;

			// Set SurfaceID to the index of the box face that was hit
			outIntersection.SurfaceID =
				static_cast<int>(std::distance(cSurfaces.begin(),
											   std::find(cSurfaces.begin(), cSurfaces.end(), hit.Normal)));

			return true;
		}
		return false;
	}

	bool ERTracer::Intersect(const Ray& ray, Intersection& outIntersection) const
	{
		return IntersectImpl(ray, outIntersection);
	}

	bool ERTracer::Intersect(const Ray& ray, float maxRayLength, Intersection& outIntersection) const
	{
		return IntersectImpl(ray, outIntersection, maxRayLength);
	}

	bool ERTracer::Intersect(const Vec3& posA, const Vec3& posB, Intersection& outIntersection) const
	{
		const Vec3 line = posB - posA;
		const float distance = line.Length();
		return IntersectImpl(
			Ray{ .Origin = posA, .Direction = line / distance },
			outIntersection,
			distance);
	}

	bool ERTracer::IsOccluded(const Vec3&, const Vec3&) const
	{
		return false;
	}

	float ERTracer::GetMaterialFactor(int) const
	{
		return 0.6f; // 0 results in fully specular reflections
	}

	bool ERTracer::GetMaterialAbsorption(int surfaceId, EnergyBands& outAbsorption) const
	{
		JPL_ASSERT(mSurfaceMaterial != nullptr);
		outAbsorption = mSurfaceMaterial->Coeffs;
		return true;
	}

	bool ERTracer::GetMaterialAbsorption(const TraceNode<Intersection>& newMaterial, EnergyBands& outAbsorption) const
	{
		JPL_ASSERT(mSurfaceMaterial != nullptr);
		outAbsorption = mSurfaceMaterial->Coeffs;
		return true;
	}

	bool ERTracer::GetSurfacePlane(int surfaceId, Vec3& planeNormal, Vec3& planePoint) const
	{
		if (surfaceId >= cSurfaces.size())
			return false;

		planeNormal = cSurfaces[surfaceId];

		const auto hit = mRoom.CastRay(mRoom.GetCentre(), -planeNormal, std::numeric_limits<float>::infinity());
		if (!hit.bHit)
			return false;

		planePoint = hit.Position;

		return true;
	}

	void ERTracer::SetSurfaceMaterial(const AcousticMaterial& newMaterial)
	{
		mSurfaceMaterial = &newMaterial;
	}

	inline bool ERTracer::IsSameSurface(const Intersection& a, const Intersection b)
	{
		return a.SurfaceID == b.SurfaceID
			&& a.Material == b.Material;
	}

	void ERTracer::CacheSubpath(std::span<const TraceNode<Intersection>> subpath, std::span<int32> nodeCache)
	{
		JPL_ASSERT(subpath.size() == nodeCache.size());

		for (uint32 n = 0; n < subpath.size(); ++n)
		{
			nodeCache[n] = subpath[n].Hash;
		}
	}

	ERTracer::ERTracer()
		: mListener{ .Position = Vec3::Zero(), .Id = 9 }
		, mSource{ .Position = Vec3::Zero(), .Id = 10 }
		, mSurfaceMaterial{ JPL::AcousticMaterial::Get("ConcreteBlockRough") }
	{
		JPL_ASSERT(mSurfaceMaterial != nullptr);
	}

	void ERTracer::InitScene(const Vec3& dimensions)
	{
		const Vec3 halfSize = dimensions * 0.5f;
		mRoom = Box{ halfSize, halfSize };
		mCache = {};
	}

	void ERTracer::Trace(uint32_t numPrimaryRays, uint32_t maxOrder)
	{
		mCache.Validate(*this);

		const TraceParameters parameters
		{
			.NumPrimaryRays = numPrimaryRays,
			.MaxTraceOrder = maxOrder,
			.MaxRayLength = 10'000.0f
		};

		TraceResults<Intersection> traceResults;
		mTracer.Trace(*this, mListener.Position, parameters, traceResults);

		auto* cachePtr = &mCache;
		std::span cacheAdapter(&cachePtr, 1);
		std::span receivers(&mSource, 1);
		mTracer.ProcessTraces(*this, mListener, traceResults, receivers, cacheAdapter);
	}

	void ERTracer::OnListenerChanged(const Vec3& listener)
	{
		mListener.Position = listener;
	}

	void ERTracer::OnSourceChanged(const Vec3& source)
	{
		mSource.Position = source;
	}

	void ERTracer::OnRoomSizeChanged(const Vec3& sizes)
	{
		InitScene(sizes);
	}
} // namespace JPL
