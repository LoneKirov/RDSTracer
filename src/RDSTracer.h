
/**
 * File: RDSTracer.h
 * --------------------
 * The actual Ray Tracer!
 * Author: Ryan Schmitt
 */

#ifndef _RDS_TRACER_H_
#define _RDS_TRACER_H_

#define GLM_FORCE_INLINE

#include <vector>
#include <boost/shared_ptr.hpp>
#include "RDSImage.h"
#include "RDSScene.h"
#include "POVRayParser.h"
#include "cuda_ray_tracer.h"

namespace RDST
{
   /**
    * The actual ray tracing code!
    */
   class Tracer
   {
   private:
      /* Hidden ctors */
      explicit Tracer()
      {}
      explicit Tracer(const Tracer& rhs)
      {}
      ~Tracer()
      {}
   public:
      /* Ray tracing functions */
      static void RayTrace(const SceneDescription& scene, Image& image);
   private:
      /* Helper Functions */
      static std::vector<RayPtr> GenerateRays(const Camera& cam, const Image& image);
      static Intersection*       RayObjectsIntersect(Ray& ray, const std::vector<GeomObjectPtr>& objs);
      static void                ShadePixel(Pixel& p, const SceneDescription& scene, const Intersection& intrs);
      /* CUDA Helper Functions */
      static void                initCudaSpheres(cuda_sphere_t pSphereArr[], const std::vector<SpherePtr>& spheres);
      static void                initCudaRays(cuda_ray_t pRayArr[], const std::vector<RayPtr>& rays);
   };
}

#endif
