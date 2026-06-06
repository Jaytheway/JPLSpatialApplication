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

#pragma once

#include "Model/RoomModel.h"
#include "Geometry/SimpleGeometry.h"

#ifndef JPL_HAS_PATH_TRACING
#define JPL_HAS_PATH_TRACING 1
#endif // !JPL_HAS_PATH_TRACING

#include "JPLSpatial/AcousticMaterial.h"

#include "JPLSpatial/Math/MinimalVec3.h"

#include <JPLSpatial/PathTracing/SceneInterface.h>
#include <JPLSpatial/PathTracing/SpecularPath.h>
#include <JPLSpatial/PathTracing/SpecularPathCache.h>
#include <JPLSpatial/PathTracing/SpecularRayTracing.h>

namespace JPL
{
    class ERTracer
    {
    public:
        using Vec3 = JPL::MinimalVec3;

        struct Ray
        {
            Vec3 Origin;
            Vec3 Direction;
        };

        struct Intersection
        {
            Vec3 Normal;
            Vec3 Position;
            int Material;
            int SurfaceID;
        };

        struct Source
        {
            Vec3 Position; //? temp for testing
            uint32_t Id;
        };

        struct Listener
        {
            Vec3 Position; //? temp for testing
            uint32_t Id;
        };

        using SourceData = Listener;
        using ReceiverData = Source;
    public:
        using Vec3 = JPL::MinimalVec3;

        ERTracer();
        ~ERTracer() = default;

        void InitScene(const Vec3& dimensions);
        void Trace(uint32_t numPrimaryRays, uint32_t maxOrder);
        const JPL::SpecularPathCache<Vec3>& GetCache() const { return mCache; }
        void ClearCache() { mCache = {}; }

        // RoomView::ChangeListener interface
        void OnListenerChanged(const Vec3& listener);
        void OnSourceChanged(const Vec3& source);
        void OnRoomSizeChanged(const Vec3& sizes);

        // SceneInterface
        bool Intersect(const Ray& ray, Intersection& outIntersection) const;
        bool Intersect(const Ray& ray, float maxRayLength, Intersection& outIntersection) const;
        bool Intersect(const Vec3& posA, const Vec3& posB, Intersection& outIntersection) const;
        bool IsOccluded(const Vec3&, const Vec3&) const;
        float GetMaterialFactor(int) const;
        bool GetMaterialAbsorption(int surfaceId, EnergyBands& outAbsorption) const;
        bool GetMaterialAbsorption(const TraceNode<Intersection>& newMaterial, EnergyBands& outAbsorption) const;
        Vec3 GetSourcePosition(int) const { return mListener.Position; } // we do backtracing, so these are backwards
        Vec3 GetReceiverPosition(int) const { return mSource.Position; }
        Vec3 GetListenerPosition(int) const { return mListener.Position; }
        bool GetSurfacePlane(int surfaceId, Vec3& planeNormal, Vec3& planePoint) const;

        void SetSurfaceMaterial(const AcousticMaterial& newMaterial);

        static inline bool IsSameSurface(const Intersection& a, const Intersection b);
        void CacheSubpath(std::span<const TraceNode<Intersection>> subpath, std::span<int32> nodeCache);
        inline uint32 GetHash(const Intersection& intersection) const { return intersection.SurfaceID; }
        inline Vec3 GetPosition(const Intersection& intersection) const { return intersection.Position; }
        inline Vec3 GetNormal(const Intersection& intersection) const { return intersection.Normal; }

        // Validate visibility of the reflected image source from the reciever
        static inline bool ValidatePath(
            const ERTracer& scene,
            std::span<const int> triCache,
            std::span<const Vec3> imageSources,
            const Vec3& receiver)
        {
            Vec3 R = receiver;

            // Check that we do intersect the correct surface sequance,
            // and nothing is obstructing visibility of the image source
            for (auto i = imageSources.size() - 1; i >= 1; --i)
            {
                Intersection hit;
                if (not scene.Intersect(R, imageSources[i], hit) || hit.SurfaceID != triCache[i])
                    return false;

                // Small offset to avoide self intersection
                static constexpr float offset = 0.000f;

                R = hit.Position + hit.Normal * offset;
            }

            // Lastly check visibility between
            // source and first refleciton point
            return not scene.IsOccluded(R, imageSources[0]);
        }

    private:
        bool IntersectImpl(const Ray& ray, Intersection& outIntersection, float maxDistance = std::numeric_limits<float>::infinity()) const;

    private:
        JPL::SpecularRayTracing mTracer;
        JPL::SpecularPathCache<Vec3> mCache;

        Box<Vec3> mRoom{ Vec3(25.0f, 0.0f, 25.0f), Vec3{25.0f, 5.0f, 25.0f} };
        Listener mListener;
        Source mSource;

        const JPL::AcousticMaterial* mSurfaceMaterial;
    };
} // namespace JPL
