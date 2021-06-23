//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_ImplicitFunction_h
#define svtk_m_ImplicitFunction_h

#include <svtkm/Bounds.h>
#include <svtkm/Math.h>
#include <svtkm/VectorAnalysis.h>
#include <svtkm/VirtualObjectBase.h>

namespace svtkm
{

//============================================================================
class SVTKM_ALWAYS_EXPORT ImplicitFunction : public svtkm::VirtualObjectBase
{
public:
  using Scalar = svtkm::FloatDefault;
  using Vector = svtkm::Vec<Scalar, 3>;

  SVTKM_EXEC_CONT virtual Scalar Value(const Vector& point) const = 0;
  SVTKM_EXEC_CONT virtual Vector Gradient(const Vector& point) const = 0;

  SVTKM_EXEC_CONT Scalar Value(Scalar x, Scalar y, Scalar z) const
  {
    return this->Value(Vector(x, y, z));
  }

  SVTKM_EXEC_CONT Vector Gradient(Scalar x, Scalar y, Scalar z) const
  {
    return this->Gradient(Vector(x, y, z));
  }
};

//============================================================================
/// A helpful functor that calls the (virtual) value method of a given ImplicitFunction. Can be
/// passed to things that expect a functor instead of an ImplictFunction class (like an array
/// transform).
///
class SVTKM_ALWAYS_EXPORT ImplicitFunctionValue
{
public:
  using Scalar = svtkm::ImplicitFunction::Scalar;
  using Vector = svtkm::ImplicitFunction::Vector;

  SVTKM_EXEC_CONT ImplicitFunctionValue()
    : Function(nullptr)
  {
  }

  SVTKM_EXEC_CONT ImplicitFunctionValue(const ImplicitFunction* function)
    : Function(function)
  {
  }

  SVTKM_EXEC_CONT Scalar operator()(const Vector& point) const
  {
    return this->Function->Value(point);
  }

private:
  const svtkm::ImplicitFunction* Function;
};

/// A helpful functor that calls the (virtual) gradient method of a given ImplicitFunction. Can be
/// passed to things that expect a functor instead of an ImplictFunction class (like an array
/// transform).
///
class SVTKM_ALWAYS_EXPORT ImplicitFunctionGradient
{
public:
  using Scalar = svtkm::ImplicitFunction::Scalar;
  using Vector = svtkm::ImplicitFunction::Vector;

  SVTKM_EXEC_CONT ImplicitFunctionGradient()
    : Function(nullptr)
  {
  }

  SVTKM_EXEC_CONT ImplicitFunctionGradient(const ImplicitFunction* function)
    : Function(function)
  {
  }

  SVTKM_EXEC_CONT Vector operator()(const Vector& point) const
  {
    return this->Function->Gradient(point);
  }

private:
  const svtkm::ImplicitFunction* Function;
};

//============================================================================
/// \brief Implicit function for a box
///
/// \c Box computes the implicit function and/or gradient for a axis-aligned
/// bounding box. Each side of the box is orthogonal to all other sides
/// meeting along shared edges and all faces are orthogonal to the x-y-z
/// coordinate axes.

class SVTKM_ALWAYS_EXPORT Box : public ImplicitFunction
{
public:
  /// \brief Construct box with center at (0,0,0) and each side of length 1.0.
  SVTKM_EXEC_CONT Box()
    : MinPoint(Vector(Scalar(-0.5)))
    , MaxPoint(Vector(Scalar(0.5)))
  {
  }

  SVTKM_EXEC_CONT Box(const Vector& minPoint, const Vector& maxPoint)
    : MinPoint(minPoint)
    , MaxPoint(maxPoint)
  {
  }

  SVTKM_EXEC_CONT Box(Scalar xmin, Scalar xmax, Scalar ymin, Scalar ymax, Scalar zmin, Scalar zmax)
    : MinPoint(xmin, ymin, zmin)
    , MaxPoint(xmax, ymax, zmax)
  {
  }

  SVTKM_CONT Box(const svtkm::Bounds& bounds) { this->SetBounds(bounds); }

  SVTKM_CONT void SetMinPoint(const Vector& point)
  {
    this->MinPoint = point;
    this->Modified();
  }

  SVTKM_CONT void SetMaxPoint(const Vector& point)
  {
    this->MaxPoint = point;
    this->Modified();
  }

  SVTKM_EXEC_CONT const Vector& GetMinPoint() const { return this->MinPoint; }

  SVTKM_EXEC_CONT const Vector& GetMaxPoint() const { return this->MaxPoint; }

  SVTKM_CONT void SetBounds(const svtkm::Bounds& bounds)
  {
    this->SetMinPoint({ Scalar(bounds.X.Min), Scalar(bounds.Y.Min), Scalar(bounds.Z.Min) });
    this->SetMaxPoint({ Scalar(bounds.X.Max), Scalar(bounds.Y.Max), Scalar(bounds.Z.Max) });
  }

  SVTKM_EXEC_CONT svtkm::Bounds GetBounds() const
  {
    return svtkm::Bounds(svtkm::Range(this->MinPoint[0], this->MaxPoint[0]),
                        svtkm::Range(this->MinPoint[1], this->MaxPoint[1]),
                        svtkm::Range(this->MinPoint[2], this->MaxPoint[2]));
  }

  SVTKM_EXEC_CONT Scalar Value(const Vector& point) const final
  {
    Scalar minDistance = svtkm::NegativeInfinity32();
    Scalar diff, t, dist;
    Scalar distance = Scalar(0.0);
    svtkm::IdComponent inside = 1;

    for (svtkm::IdComponent d = 0; d < 3; d++)
    {
      diff = this->MaxPoint[d] - this->MinPoint[d];
      if (diff != Scalar(0.0))
      {
        t = (point[d] - this->MinPoint[d]) / diff;
        // Outside before the box
        if (t < Scalar(0.0))
        {
          inside = 0;
          dist = this->MinPoint[d] - point[d];
        }
        // Outside after the box
        else if (t > Scalar(1.0))
        {
          inside = 0;
          dist = point[d] - this->MaxPoint[d];
        }
        else
        {
          // Inside the box in lower half
          if (t <= Scalar(0.5))
          {
            dist = MinPoint[d] - point[d];
          }
          // Inside the box in upper half
          else
          {
            dist = point[d] - MaxPoint[d];
          }
          if (dist > minDistance)
          {
            minDistance = dist;
          }
        }
      }
      else
      {
        dist = svtkm::Abs(point[d] - MinPoint[d]);
        if (dist > Scalar(0.0))
        {
          inside = 0;
        }
      }
      if (dist > Scalar(0.0))
      {
        distance += dist * dist;
      }
    }

    distance = svtkm::Sqrt(distance);
    if (inside)
    {
      return minDistance;
    }
    else
    {
      return distance;
    }
  }

  SVTKM_EXEC_CONT Vector Gradient(const Vector& point) const final
  {
    svtkm::IdComponent minAxis = 0;
    Scalar dist = 0.0;
    Scalar minDist = svtkm::Infinity32();
    svtkm::IdComponent3 location;
    Vector normal(Scalar(0));
    Vector inside(Scalar(0));
    Vector outside(Scalar(0));
    Vector center((this->MaxPoint + this->MinPoint) * Scalar(0.5));

    // Compute the location of the point with respect to the box
    // Point will lie in one of 27 separate regions around or within the box
    // Gradient vector is computed differently in each of the regions.
    for (svtkm::IdComponent d = 0; d < 3; d++)
    {
      if (point[d] < this->MinPoint[d])
      {
        // Outside the box low end
        location[d] = 0;
        outside[d] = -1.0;
      }
      else if (point[d] > this->MaxPoint[d])
      {
        // Outside the box high end
        location[d] = 2;
        outside[d] = 1.0;
      }
      else
      {
        location[d] = 1;
        if (point[d] <= center[d])
        {
          // Inside the box low end
          dist = point[d] - this->MinPoint[d];
          inside[d] = -1.0;
        }
        else
        {
          // Inside the box high end
          dist = this->MaxPoint[d] - point[d];
          inside[d] = 1.0;
        }
        if (dist < minDist) // dist is negative
        {
          minDist = dist;
          minAxis = d;
        }
      }
    }

    svtkm::Id indx = location[0] + 3 * location[1] + 9 * location[2];
    switch (indx)
    {
      // verts - gradient points away from center point
      case 0:
      case 2:
      case 6:
      case 8:
      case 18:
      case 20:
      case 24:
      case 26:
        for (svtkm::IdComponent d = 0; d < 3; d++)
        {
          normal[d] = point[d] - center[d];
        }
        svtkm::Normalize(normal);
        break;

      // edges - gradient points out from axis of cube
      case 1:
      case 3:
      case 5:
      case 7:
      case 9:
      case 11:
      case 15:
      case 17:
      case 19:
      case 21:
      case 23:
      case 25:
        for (svtkm::IdComponent d = 0; d < 3; d++)
        {
          if (outside[d] != 0.0)
          {
            normal[d] = point[d] - center[d];
          }
          else
          {
            normal[d] = 0.0;
          }
        }
        svtkm::Normalize(normal);
        break;

      // faces - gradient points perpendicular to face
      case 4:
      case 10:
      case 12:
      case 14:
      case 16:
      case 22:
        for (svtkm::IdComponent d = 0; d < 3; d++)
        {
          normal[d] = outside[d];
        }
        break;

      // interior - gradient is perpendicular to closest face
      case 13:
        normal[0] = normal[1] = normal[2] = 0.0;
        normal[minAxis] = inside[minAxis];
        break;
      default:
        SVTKM_ASSERT(false);
        break;
    }
    return normal;
  }

private:
  Vector MinPoint;
  Vector MaxPoint;
};

//============================================================================
/// \brief Implicit function for a cylinder
///
/// \c Cylinder computes the implicit function and function gradient
/// for a cylinder using F(r)=r^2-Radius^2. By default the Cylinder is
/// centered at the origin and the axis of rotation is along the
/// y-axis. You can redefine the center and axis of rotation by setting
/// the Center and Axis data members.
///
/// Note that the cylinder is infinite in extent.
///
class SVTKM_ALWAYS_EXPORT Cylinder final : public svtkm::ImplicitFunction
{
public:
  /// Construct cylinder radius of 0.5; centered at origin with axis
  /// along y coordinate axis.
  SVTKM_EXEC_CONT Cylinder()
    : Center(Scalar(0))
    , Axis(Scalar(0), Scalar(1), Scalar(0))
    , Radius(Scalar(0.5))
  {
  }

  SVTKM_EXEC_CONT Cylinder(const Vector& axis, Scalar radius)
    : Center(Scalar(0))
    , Axis(axis)
    , Radius(radius)
  {
  }

  SVTKM_EXEC_CONT Cylinder(const Vector& center, const Vector& axis, Scalar radius)
    : Center(center)
    , Axis(svtkm::Normal(axis))
    , Radius(radius)
  {
  }

  SVTKM_CONT void SetCenter(const Vector& center)
  {
    this->Center = center;
    this->Modified();
  }

  SVTKM_CONT void SetAxis(const Vector& axis)
  {
    this->Axis = svtkm::Normal(axis);
    this->Modified();
  }

  SVTKM_CONT void SetRadius(Scalar radius)
  {
    this->Radius = radius;
    this->Modified();
  }

  SVTKM_EXEC_CONT Scalar Value(const Vector& point) const final
  {
    Vector x2c = point - this->Center;
    FloatDefault proj = svtkm::Dot(this->Axis, x2c);
    return svtkm::Dot(x2c, x2c) - (proj * proj) - (this->Radius * this->Radius);
  }

  SVTKM_EXEC_CONT Vector Gradient(const Vector& point) const final
  {
    Vector x2c = point - this->Center;
    FloatDefault t = this->Axis[0] * x2c[0] + this->Axis[1] * x2c[1] + this->Axis[2] * x2c[2];
    svtkm::Vec<FloatDefault, 3> closestPoint = this->Center + (this->Axis * t);
    return (point - closestPoint) * FloatDefault(2);
  }


private:
  Vector Center;
  Vector Axis;
  Scalar Radius;
};

//============================================================================
/// \brief Implicit function for a frustum
class SVTKM_ALWAYS_EXPORT Frustum final : public svtkm::ImplicitFunction
{
public:
  /// \brief Construct axis-aligned frustum with center at (0,0,0) and each side of length 1.0.
  Frustum() = default;

  SVTKM_EXEC_CONT Frustum(const Vector points[6], const Vector normals[6])
  {
    this->SetPlanes(points, normals);
  }

  SVTKM_EXEC_CONT explicit Frustum(const Vector points[8]) { this->CreateFromPoints(points); }

  SVTKM_EXEC void SetPlanes(const Vector points[6], const Vector normals[6])
  {
    for (svtkm::Id index : { 0, 1, 2, 3, 4, 5 })
    {
      this->Points[index] = points[index];
    }
    for (svtkm::Id index : { 0, 1, 2, 3, 4, 5 })
    {
      this->Normals[index] = normals[index];
    }
    this->Modified();
  }

  SVTKM_EXEC void SetPlane(int idx, const Vector& point, const Vector& normal)
  {
    SVTKM_ASSERT((idx >= 0) && (idx < 6));
    this->Points[idx] = point;
    this->Normals[idx] = normal;
    this->Modified();
  }

  SVTKM_EXEC_CONT void GetPlanes(Vector points[6], Vector normals[6]) const
  {
    for (svtkm::Id index : { 0, 1, 2, 3, 4, 5 })
    {
      points[index] = this->Points[index];
    }
    for (svtkm::Id index : { 0, 1, 2, 3, 4, 5 })
    {
      normals[index] = this->Normals[index];
    }
  }

  SVTKM_EXEC_CONT const Vector* GetPoints() const { return this->Points; }

  SVTKM_EXEC_CONT const Vector* GetNormals() const { return this->Normals; }

  // The points should be specified in the order of hex-cell vertices
  SVTKM_EXEC_CONT void CreateFromPoints(const Vector points[8])
  {
    // XXX(clang-format-3.9): 3.8 is silly. 3.9 makes it look like this.
    // clang-format off
    int planes[6][3] = {
      { 3, 2, 0 }, { 4, 5, 7 }, { 0, 1, 4 }, { 1, 2, 5 }, { 2, 3, 6 }, { 3, 0, 7 }
    };
    // clang-format on

    for (int i = 0; i < 6; ++i)
    {
      const Vector& v0 = points[planes[i][0]];
      const Vector& v1 = points[planes[i][1]];
      const Vector& v2 = points[planes[i][2]];

      this->Points[i] = v0;
      this->Normals[i] = svtkm::Normal(svtkm::TriangleNormal(v0, v1, v2));
    }
    this->Modified();
  }

  SVTKM_EXEC_CONT Scalar Value(const Vector& point) const final
  {
    Scalar maxVal = svtkm::NegativeInfinity<Scalar>();
    for (svtkm::Id index : { 0, 1, 2, 3, 4, 5 })
    {
      const Vector& p = this->Points[index];
      const Vector& n = this->Normals[index];
      const Scalar val = svtkm::Dot(point - p, n);
      maxVal = svtkm::Max(maxVal, val);
    }
    return maxVal;
  }

  SVTKM_EXEC_CONT Vector Gradient(const Vector& point) const final
  {
    Scalar maxVal = svtkm::NegativeInfinity<Scalar>();
    svtkm::Id maxValIdx = 0;
    for (svtkm::Id index : { 0, 1, 2, 3, 4, 5 })
    {
      const Vector& p = this->Points[index];
      const Vector& n = this->Normals[index];
      Scalar val = svtkm::Dot(point - p, n);
      if (val > maxVal)
      {
        maxVal = val;
        maxValIdx = index;
      }
    }
    return this->Normals[maxValIdx];
  }

private:
  Vector Points[6] = { { -0.5f, 0.0f, 0.0f }, { 0.5f, 0.0f, 0.0f },  { 0.0f, -0.5f, 0.0f },
                       { 0.0f, 0.5f, 0.0f },  { 0.0f, 0.0f, -0.5f }, { 0.0f, 0.0f, 0.5f } };
  Vector Normals[6] = { { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f },  { 0.0f, -1.0f, 0.0f },
                        { 0.0f, 1.0f, 0.0f },  { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 1.0f } };
};

//============================================================================
/// \brief Implicit function for a plane
///
/// A plane is defined by a point in the plane and a normal to the plane.
/// The normal does not have to be a unit vector. The implicit function will
/// still evaluate to 0 at the plane, but the values outside the plane
/// (and the gradient) will be scaled by the length of the normal vector.
class SVTKM_ALWAYS_EXPORT Plane final : public svtkm::ImplicitFunction
{
public:
  /// Construct plane passing through origin and normal to z-axis.
  SVTKM_EXEC_CONT Plane()
    : Origin(Scalar(0))
    , Normal(Scalar(0), Scalar(0), Scalar(1))
  {
  }

  /// Construct a plane through the origin with the given normal.
  SVTKM_EXEC_CONT explicit Plane(const Vector& normal)
    : Origin(Scalar(0))
    , Normal(normal)
  {
  }

  /// Construct a plane through the given point with the given normal.
  SVTKM_EXEC_CONT Plane(const Vector& origin, const Vector& normal)
    : Origin(origin)
    , Normal(normal)
  {
  }

  SVTKM_CONT void SetOrigin(const Vector& origin)
  {
    this->Origin = origin;
    this->Modified();
  }

  SVTKM_CONT void SetNormal(const Vector& normal)
  {
    this->Normal = normal;
    this->Modified();
  }

  SVTKM_EXEC_CONT const Vector& GetOrigin() const { return this->Origin; }
  SVTKM_EXEC_CONT const Vector& GetNormal() const { return this->Normal; }

  SVTKM_EXEC_CONT Scalar Value(const Vector& point) const final
  {
    return svtkm::Dot(point - this->Origin, this->Normal);
  }

  SVTKM_EXEC_CONT Vector Gradient(const Vector&) const final { return this->Normal; }

private:
  Vector Origin;
  Vector Normal;
};

//============================================================================
/// \brief Implicit function for a sphere
///
/// A sphere is defined by its center and a radius.
///
/// The value of the sphere implicit function is the square of the distance
/// from the center biased by the radius (so the surface of the sphere is
/// at value 0).
class SVTKM_ALWAYS_EXPORT Sphere final : public svtkm::ImplicitFunction
{
public:
  /// Construct sphere with center at (0,0,0) and radius = 0.5.
  SVTKM_EXEC_CONT Sphere()
    : Radius(Scalar(0.5))
    , Center(Scalar(0))
  {
  }

  /// Construct a sphere with center at (0,0,0) and the given radius.
  SVTKM_EXEC_CONT explicit Sphere(Scalar radius)
    : Radius(radius)
    , Center(Scalar(0))
  {
  }

  SVTKM_EXEC_CONT Sphere(Vector center, Scalar radius)
    : Radius(radius)
    , Center(center)
  {
  }

  SVTKM_CONT void SetRadius(Scalar radius)
  {
    this->Radius = radius;
    this->Modified();
  }

  SVTKM_CONT void SetCenter(const Vector& center)
  {
    this->Center = center;
    this->Modified();
  }

  SVTKM_EXEC_CONT Scalar GetRadius() const { return this->Radius; }

  SVTKM_EXEC_CONT const Vector& GetCenter() const { return this->Center; }

  SVTKM_EXEC_CONT Scalar Value(const Vector& point) const final
  {
    return svtkm::MagnitudeSquared(point - this->Center) - (this->Radius * this->Radius);
  }

  SVTKM_EXEC_CONT Vector Gradient(const Vector& point) const final
  {
    return Scalar(2) * (point - this->Center);
  }

private:
  Scalar Radius;
  Vector Center;
};

} // namespace svtkm

#ifdef SVTKM_CUDA

// Cuda seems to have a bug where it expects the template class VirtualObjectTransfer
// to be instantiated in a consistent order among all the translation units of an
// executable. Failing to do so results in random crashes and incorrect results.
// We workaroud this issue by explicitly instantiating VirtualObjectTransfer for
// all the implicit functions here.

#include <svtkm/cont/cuda/internal/VirtualObjectTransferCuda.h>

SVTKM_EXPLICITLY_INSTANTIATE_TRANSFER(svtkm::Box);
SVTKM_EXPLICITLY_INSTANTIATE_TRANSFER(svtkm::Cylinder);
SVTKM_EXPLICITLY_INSTANTIATE_TRANSFER(svtkm::Frustum);
SVTKM_EXPLICITLY_INSTANTIATE_TRANSFER(svtkm::Plane);
SVTKM_EXPLICITLY_INSTANTIATE_TRANSFER(svtkm::Sphere);

#endif

#endif //svtk_m_ImplicitFunction_h
