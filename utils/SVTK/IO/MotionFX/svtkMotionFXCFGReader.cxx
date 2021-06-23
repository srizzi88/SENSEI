/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMotionFXCFGReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMotionFXCFGReader.h"

#include "svtkArrayDispatch.h"
#include "svtkAssume.h"
#include "svtkDataArrayRange.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkSMPTools.h"
#include "svtkSTLReader.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTransform.h"
#include "svtkVector.h"
#include "svtkVectorOperators.h"

#include <svtksys/RegularExpression.hxx>
#include <svtksys/SystemTools.hxx>

// Set to 1 to generate debugging trace if grammar match fails.
#include "svtkMotionFXCFGGrammar.h" // grammar

#include <cctype>
#include <fstream>
#include <map>
#include <string>
#include <vector>

//=============================================================================
namespace impl
{
struct Motion;

using MapOfVectorOfMotions =
  std::map<std::string, std::vector<std::shared_ptr<const impl::Motion> > >;

//-----------------------------------------------------------------------------
// this exception is fired to indicate that a required parameter is missing for
// the motion definition.
class MissingParameterError : public std::runtime_error
{
public:
  MissingParameterError(const std::string& what_arg)
    : std::runtime_error(what_arg)
  {
  }
  MissingParameterError(const char* what_arg)
    : std::runtime_error(what_arg)
  {
  }
};

//-----------------------------------------------------------------------------
// these are a bunch of convenience methods used in constructors for various
// motion types that read parameter values from a map of params and then sets
// appropriate member variable. If the parameter is missing, then raises
// MissingParameterError exception.
template <typename Value, typename MapType>
void set(Value& ref, const char* pname, const MapType& params);

template <typename MapType>
void set(std::string& ref, const char* pname, const MapType& params)
{
  auto iter = params.find(pname);
  if (iter == params.end() || iter->second.StringValue.empty())
  {
    throw MissingParameterError(pname);
  }
  ref = iter->second.StringValue;
}

template <typename MapType>
void set(svtkVector3d& ref, const char* pname, const MapType& params)
{
  auto iter = params.find(pname);
  if (iter == params.end() || iter->second.DoubleValue.size() != 3)
  {
    throw MissingParameterError(pname);
  }
  ref = svtkVector3d(&iter->second.DoubleValue[0]);
}

template <typename MapType>
void set(double& ref, const char* pname, const MapType& params)
{
  auto iter = params.find(pname);
  if (iter == params.end() || iter->second.DoubleValue.size() != 1)
  {
    throw MissingParameterError(pname);
  }
  ref = iter->second.DoubleValue[0];
}

//-----------------------------------------------------------------------------
// this is a variant of set that doesn't raise MissingParameterError exception
// instead set the param to the default value indicated.
template <typename Value, typename MapType>
void set(Value& ref, const char* pname, const MapType& params, const Value& defaultValue)
{
  try
  {
    set(ref, pname, params);
  }
  catch (const MissingParameterError&)
  {
    ref = defaultValue;
  }
}

//-----------------------------------------------------------------------------
// Superclass for all motions
// The member variable names match the keyworks in the cfg file and hence are
// left lower-case.
struct Motion
{
  // Starting time of the motion.
  double tstart_prescribe;

  // Ending time of the motion. Note that by changing starting
  //  time and ending time, you can add the motions of a single
  //  phase in order to get a complex motion.
  double tend_prescribe;

  // This specified the period of acceleration time (damping).
  // The motion will start at time tstart_prescribe with 0 velocity and
  // ramp up to the specified value during this time.
  double t_damping;

  // filename for the geometry file.
  std::string stl;

  template <typename MapType>
  Motion(const MapType& params)
  {
    set(this->tstart_prescribe, "tstart_prescribe", params);
    set(this->tend_prescribe, "tend_prescribe", params);
    set(this->t_damping, "t_damping", params, 0.0);
    set(this->stl, "stl", params);
  }
  virtual ~Motion() {}

  virtual bool Move(svtkPoints* pts, double time) const = 0;

protected:
  template <typename Type>
  Type compute_displacement(
    double time, const Type& init_velocity, const Type& acceleration, const Type& velocity) const
  {
    // we don't bother converting freq to angular velocity since it cancels out
    // anyways when we take the final mod.
    Type s(0.0);
    if (this->t_damping > 0)
    {
      // s = u*tA + 0.5 * a * (tA)^2
      const double tA = std::min(time - this->tstart_prescribe, this->t_damping);
      ;
      assert(tA >= 0.0);
      const double tA2 = tA * tA;
      s = s + (init_velocity * tA + acceleration * (tA2 / 2.0));
    }

    if (time > (this->tstart_prescribe + this->t_damping))
    {
      // s = v*t
      const double t =
        std::min(time, this->tend_prescribe) - this->tstart_prescribe - this->t_damping;
      s = s + (velocity * t);
    }
    return s;
  }

  // A worker that apply the Transform provided to all points.
  struct ApplyTransform
  {
    svtkSmartPointer<svtkTransform> Transform;
    ApplyTransform(svtkTransform* transform)
      : Transform(transform)
    {
    }

    template <typename InputArrayType>
    void operator()(InputArrayType* darray)
    {
      SVTK_ASSUME(darray->GetNumberOfComponents() == 3);
      using ValueType = svtk::GetAPIType<InputArrayType>;

      svtkSMPTools::For(0, darray->GetNumberOfTuples(), [&](svtkIdType begin, svtkIdType end) {
        auto drange = svtk::DataArrayTupleRange(darray, begin, end);
        for (auto tuple : drange)
        {
          svtkVector4<ValueType> in, out;
          in[0] = tuple[0];
          in[1] = tuple[1];
          in[2] = tuple[2];
          in[3] = 1.0;

          this->Transform->MultiplyPoint(in.GetData(), out.GetData());

          out[0] /= out[3];
          out[1] /= out[3];
          out[2] /= out[3];
          tuple.SetTuple(out.GetData());
        }
      });
    }
  };
};

//-----------------------------------------------------------------------------
// Move given velocity
struct ImposeVelMotion : public Motion
{
  svtkVector3d impose_vel;      //< Prescribed velocity (vector form).
  svtkVector3d impose_vel_init; //< Prescribed velocity (vector form) at t0 increase to impose_vel
                               // until t_damping.

  svtkVector3d acceleration; // (derived) acceleration during damping time.

  template <typename MapType>
  ImposeVelMotion(const MapType& params)
    : Motion(params)
    , acceleration(0.0)
  {
    std::string motion_type;
    set(motion_type, "motion_type", params);
    assert(motion_type == "IMPOSE_VEL");

    set(this->impose_vel, "impose_vel", params);
    set(this->impose_vel_init, "impose_vel_init", params, this->impose_vel);

    // compute acceleration.
    if (this->t_damping > 0)
    {
      this->acceleration = (this->impose_vel - this->impose_vel_init) * (1.0 / this->t_damping);
    }
  }
  bool Move(svtkPoints* pts, double time) const override
  {
    if (time < this->tstart_prescribe)
    {
      // nothing to do, this motion hasn't been activated yet.
      return false;
    }

    svtkVector3d s =
      this->compute_displacement(time, this->impose_vel_init, this->acceleration, this->impose_vel);
    if (s != svtkVector3d(0.0))
    {
      ApplyDisplacement worker(s);

      // displace points.
      using PointTypes = svtkTypeList::Create<float, double>;
      svtkArrayDispatch::DispatchByValueType<PointTypes>::Execute(pts->GetData(), worker);
      pts->GetData()->Modified();
    }

    return true;
  }

private:
  struct ApplyDisplacement
  {
    const svtkVector3d& Displacement;
    ApplyDisplacement(const svtkVector3d& disp)
      : Displacement(disp)
    {
    }

    template <typename InputArrayType>
    void operator()(InputArrayType* darray)
    {
      using T = svtk::GetAPIType<InputArrayType>;

      svtkSMPTools::For(0, darray->GetNumberOfTuples(), [&](svtkIdType begin, svtkIdType end) {
        for (auto tuple : svtk::DataArrayTupleRange<3>(darray, begin, end))
        {
          tuple[0] += static_cast<T>(this->Displacement[0]);
          tuple[1] += static_cast<T>(this->Displacement[1]);
          tuple[2] += static_cast<T>(this->Displacement[2]);
        }
      });
    }
  };
};

//-----------------------------------------------------------------------------
// Rotate around an arbitrary axis.
struct RotateAxisMotion : public Motion
{
  // Center of rotation. This point needs to lie on the same
  // line as the rotation frequency vector if you want co-axial motion,
  // e.g. gear spinning.
  svtkVector3d rot_cntr;

  // Rotation axis vector.
  svtkVector3d rot_axis;

  // Frequency of rotation [rot/s].
  double rot_axis_freq;

  // Frequency of rotation at t0 increase to rot_axis_freq until t_damping [rot/s].
  double rot_axis_freq_init;

  double rot_axis_w;       // (derived)
  double rot_axis_w_init;  // (derived)
  double rot_acceleration; // (derived) acceleration during  t_damping

  template <typename MapType>
  RotateAxisMotion(const MapType& params)
    : Motion(params)
    , rot_acceleration(0)
  {
    std::string motion_type;
    set(motion_type, "motion_type", params);
    assert(motion_type == "ROTATE_AXIS");

    set(this->rot_cntr, "rot_cntr", params);
    set(this->rot_axis, "rot_axis", params);
    set(this->rot_axis_freq, "rot_axis_freq", params);
    set(this->rot_axis_freq_init, "rot_axis_freq_init", params, this->rot_axis_freq);

    this->rot_axis.Normalize();
    this->rot_axis_w = 2 * svtkMath::Pi() * this->rot_axis_freq;
    this->rot_axis_w_init = 2 * svtkMath::Pi() * this->rot_axis_freq_init;

    if (this->t_damping > 0)
    {
      this->rot_acceleration = (this->rot_axis_w - this->rot_axis_w_init) / this->t_damping;
    }
  }

  bool Move(svtkPoints* pts, double time) const override
  {
    if (time < this->tstart_prescribe)
    {
      // nothing to do, this motion hasn't been activated yet.
      return false;
    }

    double theta = this->compute_displacement(
      time, this->rot_axis_w_init, this->rot_acceleration, this->rot_axis_w);

    if (theta != 0.0)
    {
      // theta is in radians.
      // convert to degress
      theta = svtkMath::DegreesFromRadians(theta);

      svtkNew<svtkTransform> transform;
      transform->Identity();
      transform->Translate(this->rot_cntr.GetData());
      transform->RotateWXYZ(theta, this->rot_axis.GetData());
      transform->Translate(-this->rot_cntr[0], -this->rot_cntr[1], -this->rot_cntr[2]);

      ApplyTransform worker(transform);
      // transform points.
      using PointTypes = svtkTypeList::Create<float, double>;
      svtkArrayDispatch::DispatchByValueType<PointTypes>::Execute(pts->GetData(), worker);
      pts->GetData()->Modified();
    }
    return true;
  }
};

//-----------------------------------------------------------------------------
// Rotate around x,y,z coordinate axes.
struct RotateMotion : public Motion
{
  svtkVector3d rot_freq;
  svtkVector3d rot_cntr;
  svtkVector3d rot_freq_init;    // (optional)
  svtkVector3d rot_acceleration; // (derived)
  svtkVector3d rot_w;            // (derived)
  svtkVector3d rot_w_init;       // (derived)

  template <typename MapType>
  RotateMotion(const MapType& params)
    : Motion(params)
    , rot_acceleration(0, 0, 0)
  {
    std::string motion_type;
    set(motion_type, "motion_type", params);
    assert(motion_type == "ROTATE");

    set(this->rot_freq, "rot_freq", params);
    set(this->rot_cntr, "rot_cntr", params);
    set(this->rot_freq_init, "rot_freq_init", params, this->rot_freq);

    this->rot_w = 2 * svtkMath::Pi() * this->rot_freq;
    this->rot_w_init = 2 * svtkMath::Pi() * this->rot_freq_init;

    if (this->t_damping > 0)
    {
      this->rot_acceleration = (this->rot_w - this->rot_w_init) / svtkVector3d(this->t_damping);
    }
  }

  bool Move(svtkPoints* pts, double time) const override
  {
    if (time < this->tstart_prescribe)
    {
      // nothing to do, this motion hasn't been activated yet.
      return false;
    }

    svtkVector3d theta =
      this->compute_displacement(time, this->rot_w_init, this->rot_acceleration, this->rot_w);

    if (theta != svtkVector3d(0.0))
    {
      // remember, theta is in radians.
      svtkNew<svtkTransform> transform;
      transform->Identity();
      transform->Translate(this->rot_cntr.GetData());
      transform->RotateWXYZ(
        svtkMath::DegreesFromRadians(theta.Norm()), theta[0], theta[1], theta[2]);
      transform->Translate(-this->rot_cntr[0], -this->rot_cntr[1], -this->rot_cntr[2]);

      ApplyTransform worker(transform);
      // transform points.
      using PointTypes = svtkTypeList::Create<float, double>;
      svtkArrayDispatch::DispatchByValueType<PointTypes>::Execute(pts->GetData(), worker);
      pts->GetData()->Modified();
    }
    return true;
  }
};

//-----------------------------------------------------------------------------
// Planetary motion
struct PlanetaryMotion : public Motion
{
  // Center of the sun gear/carrier.
  svtkVector3d orbit_cntr;

  // The radius of the orbit.
  double orbit_radius;

  // The direction vector of the year rotation axis. Doesn't have to be normalized.
  svtkVector3d year_rotationVec;

  // Frequency of the year rotation [rot/s].
  double year_frequency;

  // Frequency of the year rotation at t0 increase to year_frequency until t_damping [rot/s].
  double year_frequency_init;

  // The direction vector of the day rotation axis. Doesn't have to be normalized.
  svtkVector3d day_rotationVec;

  // Frequency of the day rotation [rot/s].
  double day_frequency;

  // Frequency of the day rotation at t0 increase to day_frequency until t_damping [rot/s].
  double day_frequency_init;

  // Any point on the initial day rotation axis.
  svtkVector3d initial_centerOfDayRotation;

  double year_acceleration; // (derived)
  double day_acceleration;  // (derived)
  double year_w;            // (derived)
  double year_w_init;       // (derived)
  double day_w;             // (derived)
  double day_w_init;        // (derived)

  template <typename MapType>
  PlanetaryMotion(const MapType& params)
    : Motion(params)
    , year_acceleration(0.0)
    , day_acceleration(0.0)
  {
    std::string motion_type;
    set(motion_type, "motion_type", params);
    assert(motion_type == "PLANETARY");

    set(this->orbit_cntr, "orbit_cntr", params);
    set(this->orbit_radius, "orbit_radius", params);
    set(this->year_rotationVec, "year_rotationVec", params);
    set(this->year_frequency, "year_frequency", params);
    set(this->year_frequency_init, "year_frequency_init", params, this->year_frequency);
    set(this->day_rotationVec, "day_rotationVec", params);
    set(this->day_frequency, "day_frequency", params);
    set(this->day_frequency_init, "day_frequency_init", params, this->day_frequency);
    set(this->initial_centerOfDayRotation, "initial_centerOfDayRotation", params);

    this->year_rotationVec.Normalize();
    this->day_rotationVec.Normalize();

    this->year_w = 2 * svtkMath::Pi() * this->year_frequency;
    this->year_w_init = 2 * svtkMath::Pi() * this->year_frequency_init;

    this->day_w = 2 * svtkMath::Pi() * this->day_frequency;
    this->day_w_init = 2 * svtkMath::Pi() * this->day_frequency_init;

    if (this->t_damping > 0)
    {
      this->year_acceleration = (this->year_w - this->year_w_init) / this->t_damping;
      this->day_acceleration = (this->day_w - this->day_w_init) / this->t_damping;
    }
  }

  bool Move(svtkPoints* pts, double time) const override
  {
    if (time < this->tstart_prescribe)
    {
      // nothing to do, this motion hasn't been activated yet.
      return false;
    }

    // compute rotation angular displacement
    double day_theta =
      this->compute_displacement(time, this->day_w_init, this->day_acceleration, this->day_w);

    // compute revolution angular displacement
    double year_theta =
      this->compute_displacement(time, this->year_w_init, this->year_acceleration, this->year_w);

    if (day_theta != 0.0 || year_theta != 0.0)
    {

      svtkNew<svtkTransform> transform;
      transform->Identity();

      // year_theta is in radians
      // convert to degrees.
      year_theta = svtkMath::DegreesFromRadians(year_theta);

      transform->Translate(this->orbit_cntr.GetData());
      transform->RotateWXYZ(year_theta, this->year_rotationVec.GetData());
      transform->Translate(-this->orbit_cntr[0], -this->orbit_cntr[1], -this->orbit_cntr[2]);

      // day_theta is in radians.
      // convert to degress
      day_theta = svtkMath::DegreesFromRadians(day_theta);

      transform->Translate(this->initial_centerOfDayRotation.GetData());
      transform->RotateWXYZ(day_theta, this->day_rotationVec.GetData());
      transform->Translate(-this->initial_centerOfDayRotation[0],
        -this->initial_centerOfDayRotation[1], -this->initial_centerOfDayRotation[2]);

      ApplyTransform worker(transform);
      // transform points.
      using PointTypes = svtkTypeList::Create<float, double>;
      svtkArrayDispatch::DispatchByValueType<PointTypes>::Execute(pts->GetData(), worker);
      pts->GetData()->Modified();
    }
    return true;
  }
};

//-----------------------------------------------------------------------------
// Move given a position file.
struct PositionFileMotion : public Motion
{
  // name of the file that contains the coordinates and angular
  // velocity vectors as a function of time.
  std::string positionFile;

  // If this is set to false - old rot.vel. format of the input file is required.
  // If set to true (default), the format becomes t,CoMx,CoMy,CoMz,cosX,cosY,cosZ,Orientation[rad]
  bool isOrientation;

  // Center of mass for time. This is generally the center of bounds for the STL
  // file itself.
  svtkVector3d initial_centerOfMass;

  struct tuple_type
  {
    svtkVector3d center_of_mass;

    // for isOrientation=true
    svtkVector3d direction_cosines;
    double rotation;

    // for isOrientation=false;
    svtkVector3d angular_velocities;

    tuple_type()
      : center_of_mass(0.0)
      , direction_cosines(0.0)
      , rotation(0.0)
      , angular_velocities(0.0)
    {
    }
  };

  mutable std::map<double, tuple_type> positions; // (derived).

  template <typename MapType>
  PositionFileMotion(const MapType& params)
    : Motion(params)
    , positionFile()
    , isOrientation(false)
    , initial_centerOfMass{ SVTK_DOUBLE_MAX }
    , positions()
  {
    std::string motion_type;
    set(motion_type, "motion_type", params);
    assert(motion_type == "POSITION_FILE");

    set(this->positionFile, "positionFile", params);
    set(this->initial_centerOfMass, "initial_centerOfMass", params, this->initial_centerOfMass);

    std::string s_isOrientation;
    set(s_isOrientation, "isOrientation", params, std::string("false"));
    s_isOrientation = svtksys::SystemTools::LowerCase(s_isOrientation);
    if (s_isOrientation == "true" || s_isOrientation == "1")
    {
      this->isOrientation = true;
    }
    else
    {
      // default.
      this->isOrientation = false;
    }
  }

  // read_position_file is defined later since it needs the Actions namespace.
  bool read_position_file(const std::string& rootDir) const;

  bool Move(svtkPoints* pts, double time) const override
  {
    if ((time < this->tstart_prescribe) || (this->positions.size() < 2))
    {
      // nothing to do, this motion hasn't been activated yet.
      // if there's less than 2 position entries, the interpolation logic fails
      // and hence we don't handle it.
      return false;
    }

    time -= this->tstart_prescribe;

    // let's clamp to end time in the position time to avoid complications.
    time = std::min(this->positions.rbegin()->first, time);

    auto iter = this->positions.lower_bound(time);
    if (iter == this->positions.begin() && iter->first != time)
    {
      // first time is greater than `time`, nothing to do.
      return false;
    }

    // iter can never be end since we've clamp time to the last time in the
    // position file.
    assert(iter != this->positions.end());

    svtkNew<svtkTransform> transform;
    transform->PostMultiply();
    // center to the initial_centerOfMass.
    if (this->initial_centerOfMass != svtkVector3d{ SVTK_DOUBLE_MAX })
    {
      transform->Translate((initial_centerOfMass * -1.0).GetData());
    }

    svtkVector3d cumulativeS(0.0); //, cumulativeTheta(0.0);
    if (this->isOrientation == false)
    {
      for (auto citer = this->positions.begin(); citer != iter; ++citer)
      {
        assert(time >= citer->first);

        auto next = std::next(citer);
        assert(next != this->positions.end());

        const double interval = (next->first - citer->first);
        const double dt = std::min(time - citer->first, interval);

        const double t = dt / interval; // normalized dt
        const svtkVector3d s = t * (next->second.center_of_mass - citer->second.center_of_mass);

        // theta = (w0 + w1)*dt / 2
        const svtkVector3d theta =
          (citer->second.angular_velocities + next->second.angular_velocities) * dt * 0.5;
        transform->RotateWXYZ(
          svtkMath::DegreesFromRadians(theta.Norm()), theta[0], theta[1], theta[2]);

        cumulativeS = cumulativeS + s;
        // cumulativeTheta = cumulativeTheta + theta;
      }
    }
    else
    {
      if (iter->first < time)
      {
        auto next = std::next(iter);
        assert(next != this->positions.end());

        const double interval = (next->first - iter->first);
        const double dt = std::min(time - iter->first, interval);
        const double t = dt / interval; // normalized dt

        const double rotation = (1.0 - t) * iter->second.rotation + t * next->second.rotation;
        const svtkVector3d cosines =
          (1.0 - t) * iter->second.direction_cosines + t * next->second.direction_cosines;
        transform->RotateWXYZ(svtkMath::DegreesFromRadians(rotation), cosines.GetData());

        const svtkVector3d disp =
          (1.0 - t) * iter->second.center_of_mass + t * next->second.center_of_mass;
        transform->Translate(disp.GetData());
      }
      else // iter->first == time
      {
        transform->RotateWXYZ(svtkMath::DegreesFromRadians(iter->second.rotation),
          iter->second.direction_cosines.GetData());
        transform->Translate(iter->second.center_of_mass.GetData());
      }
    }
    // restore
    if (this->initial_centerOfMass != svtkVector3d{ SVTK_DOUBLE_MAX })
    {
      transform->Translate(initial_centerOfMass.GetData());
    }
    transform->Translate(cumulativeS.GetData());

    ApplyTransform worker(transform);
    // transform points.
    using PointTypes = svtkTypeList::Create<float, double>;
    svtkArrayDispatch::DispatchByValueType<PointTypes>::Execute(pts->GetData(), worker);
    pts->GetData()->Modified();
    return true;
  }
};

template <typename MapType>
std::shared_ptr<const Motion> CreateMotion(const MapType& params)
{
  std::string motion_type;
  try
  {
    set(motion_type, "motion_type", params);
  }
  catch (const MissingParameterError&)
  {
    svtkGenericWarningMacro("Missing 'motion_type'. Cannot determine motion type. Skipping.");
    return nullptr;
  }

  try
  {
    if (motion_type == "IMPOSE_VEL")
    {
      return std::make_shared<ImposeVelMotion>(params);
    }
    else if (motion_type == "ROTATE_AXIS")
    {
      return std::make_shared<RotateAxisMotion>(params);
    }
    else if (motion_type == "ROTATE")
    {
      return std::make_shared<RotateMotion>(params);
    }
    else if (motion_type == "PLANETARY")
    {
      return std::make_shared<PlanetaryMotion>(params);
    }
    else if (motion_type == "POSITION_FILE")
    {
      return std::make_shared<PositionFileMotion>(params);
    }
    svtkGenericWarningMacro("Unsupported motion_type '" << motion_type << "'. Skipping.");
  }
  catch (const MissingParameterError& e)
  {
    svtkGenericWarningMacro("Missing required parameter '" << e.what() << "' "
                                                          << "for motion_type='" << motion_type
                                                          << "'");
  }

  return nullptr;
}
}

//=============================================================================
namespace Actions
{
using namespace tao::pegtl;

//-----------------------------------------------------------------------------
// actions when parsing LegacyPositionFile::Grammar or
// OrientationsPositionFile::Grammar
namespace PositionFile
{
template <typename Rule>
struct action : nothing<Rule>
{
};

template <>
struct action<MotionFX::Common::Number>
{
  // if a Number is encountered, push it into the set of active_numbers.
  template <typename Input, typename OtherState>
  static void apply(const Input& in, std::vector<double>& active_numbers, OtherState&)
  {
    active_numbers.push_back(std::atof(in.string().c_str()));
  }
};

template <>
struct action<MotionFX::LegacyPositionFile::Row>
{
  // for each row parsed, add the item to the state.
  template <typename AngularVelocitiesType>
  static void apply0(std::vector<double>& active_numbers, AngularVelocitiesType& state)
  {
    assert(active_numbers.size() == 7);

    using tuple_type = typename AngularVelocitiesType::mapped_type;
    tuple_type tuple;
    tuple.center_of_mass = svtkVector3d(active_numbers[1], active_numbers[2], active_numbers[3]);

    auto freq = svtkVector3d(active_numbers[4], active_numbers[5], active_numbers[6]);
    // convert rot/s to angular velocity
    tuple.angular_velocities = freq * 2 * svtkMath::Pi();

    state[active_numbers[0]] = tuple;
    active_numbers.clear();
  }
};

template <>
struct action<MotionFX::OrientationsPositionFile::Row>
{
  template <typename OrientationsType>
  static void apply0(std::vector<double>& active_numbers, OrientationsType& state)
  {
    assert(active_numbers.size() == 8);
    using tuple_type = typename OrientationsType::mapped_type;
    tuple_type tuple;
    tuple.center_of_mass = svtkVector3d(active_numbers[1], active_numbers[2], active_numbers[3]);
    tuple.direction_cosines = svtkVector3d(active_numbers[4], active_numbers[5], active_numbers[6]);
    tuple.rotation = active_numbers[7];
    state[active_numbers[0]] = tuple;
    active_numbers.clear();
  }
};
} // namespace PositionFile

//-----------------------------------------------------------------------------
// actions when parsing CFG::Grammar
namespace CFG
{
//-----------------------------------------------------------------------------
// When parsing CFG, we need to accumulate values and keep track of them.
// Value and ActiveState help us do that.
struct Value
{
  std::vector<double> DoubleValue;
  std::string StringValue;
  void clear()
  {
    this->StringValue.clear();
    this->DoubleValue.clear();
  }
};

struct ActiveState
{
  std::string ActiveParameterName;
  Value ActiveValue;
  std::map<std::string, Value> ActiveParameters;
  impl::MapOfVectorOfMotions& Motions;

  ActiveState(impl::MapOfVectorOfMotions& motions)
    : Motions(motions)
  {
  }
  ~ActiveState() {}

private:
  ActiveState(const ActiveState&) = delete;
  void operator=(const ActiveState&) = delete;
};
//-----------------------------------------------------------------------------

template <typename Rule>
struct action : nothing<Rule>
{
};

template <>
struct action<MotionFX::CFG::Value>
{

  template <typename Input>
  static void apply(const Input& in, ActiveState& state)
  {
    auto content = in.string();
    // the value can have trailing spaces; remove them.
    while (content.size() > 0 && std::isspace(content.back()))
    {
      content.pop_back();
    }
    svtksys::RegularExpression tupleRe("^\"([^\"]+)\"$");
    svtksys::RegularExpression numberRe(
      "^[ \t]*[-+]?(([0-9]+.?)|([0-9]*.))[0-9]*([eE][-+]?[0-9]+)?[ \t]*$");
    if (tupleRe.find(content))
    {
      state.ActiveValue.DoubleValue.clear();
      const auto tuple = tupleRe.match(1);
      auto values = svtksys::SystemTools::SplitString(tuple, ' ');
      for (const auto& val : values)
      {
        if (numberRe.find(val))
        {
          state.ActiveValue.DoubleValue.push_back(std::atof(numberRe.match(0).c_str()));
        }
        else
        {
          svtkGenericWarningMacro("Expecting number, got '" << val << "'");
        }
      }
    }
    else if (numberRe.find(content))
    {
      state.ActiveValue.DoubleValue.push_back(std::atof(numberRe.match(0).c_str()));
    }
    else
    {
      state.ActiveValue.StringValue = content;
    }
  }
};

template <>
struct action<MotionFX::CFG::ParameterName>
{
  template <typename Input>
  static void apply(const Input& in, ActiveState& state)
  {
    state.ActiveParameterName = in.string();
  }
};

template <>
struct action<MotionFX::CFG::Statement>
{
  static void apply0(ActiveState& state)
  {
    auto& params = state.ActiveParameters;
    if (params.find(state.ActiveParameterName) != params.end())
    {
      // warn: duplicate parameter, overriding.
    }
    params[state.ActiveParameterName] = state.ActiveValue;
    state.ActiveParameterName.clear();
    state.ActiveValue.clear();
  }
};

template <>
struct action<MotionFX::CFG::Motion>
{
  static void apply0(ActiveState& state)
  {
    if (auto motion = impl::CreateMotion(state.ActiveParameters))
    {
      // fixme: lets add logic to catch overlapping motions.
      state.Motions[motion->stl].push_back(motion);
    }
    state.ActiveValue.clear();
  }
};

template <>
struct action<MotionFX::CFG::Grammar>
{
  static void apply0(ActiveState& state)
  {
    // let's sort all motions according to tstart_prescribe.
    for (auto& apair : state.Motions)
    {
      std::sort(apair.second.begin(), apair.second.end(),
        [](const std::shared_ptr<const impl::Motion>& m0,
          const std::shared_ptr<const impl::Motion>& m1) {
          return m0->tstart_prescribe < m1->tstart_prescribe;
        });
    }
  }
};

} // namespace CFG

} // namespace Actions

namespace impl
{
bool PositionFileMotion::read_position_file(const std::string& rootDir) const
{
  // read positionFile.
  try
  {
    tao::pegtl::read_input<> in(rootDir + "/" + this->positionFile);
    if (this->isOrientation)
    {
      std::vector<double> numbers;
      tao::pegtl::parse<MotionFX::OrientationsPositionFile::Grammar,
        Actions::PositionFile::action /*, tao::pegtl::tracer*/>(in, numbers, this->positions);
    }
    else
    {
      std::vector<double> numbers;
      tao::pegtl::parse<MotionFX::LegacyPositionFile::Grammar,
        Actions::PositionFile::action /*, tao::pegtl::tracer*/>(in, numbers, this->positions);
    }
    return true;
  }
  catch (const tao::pegtl::input_error& e)
  {
    svtkGenericWarningMacro("PositionFileMotion::read_position_file failed: " << e.what());
  }
  return false;
}
} // impl

class svtkMotionFXCFGReader::svtkInternals
{
public:
  svtkInternals()
    : Motions()
    , TimeRange(0, -1)
    , Geometries()
  {
  }
  ~svtkInternals() {}

  const svtkVector2d& GetTimeRange() const { return this->TimeRange; }

  bool Parse(const std::string& filename)
  {
    tao::pegtl::read_input<> in(filename);
    Actions::CFG::ActiveState state(this->Motions);
    tao::pegtl::parse<MotionFX::CFG::Grammar, Actions::CFG::action>(in, state);
    if (this->Motions.size() == 0)
    {
      svtkGenericWarningMacro(
        "No valid 'motions' were parsed from the CFG file. "
        "This indicates a potential mismatch in the grammar rules and the file contents. "
        "A highly verbose log for advanced debugging can be generated by defining the environment "
        "variable `MOTIONFX_DEBUG_GRAMMAR` to debug grammar related issues.");
      if (getenv("MOTIONFX_DEBUG_GRAMMAR") != nullptr)
      {
        tao::pegtl::read_input<> in2(filename);
        tao::pegtl::parse<MotionFX::CFG::Grammar, tao::pegtl::nothing, tao::pegtl::tracer>(in2);
      }
      return false;
    }

    const auto dir = svtksys::SystemTools::GetFilenamePath(filename);

    // lets read the STL files for each of the bodies and remove any bodies that
    // do not have readable STL files.
    for (auto iter = this->Motions.begin(); iter != this->Motions.end();)
    {
      const std::string fname = dir + "/" + iter->first;
      if (svtksys::SystemTools::TestFileAccess(fname, svtksys::TEST_FILE_OK | svtksys::TEST_FILE_READ))
      {
        svtkNew<svtkSTLReader> reader;
        reader->SetFileName(fname.c_str());
        reader->Update();

        svtkPolyData* pd = reader->GetOutput();
        if (pd->GetNumberOfPoints() > 0)
        {
          this->Geometries.push_back(
            std::pair<std::string, svtkSmartPointer<svtkPolyData> >(iter->first, pd));
          ++iter;
          continue;
        }
      }
      svtkGenericWarningMacro(
        "Failed to open '" << iter->first << "'. Skipping motions associated with it.");
      iter = this->Motions.erase(iter);
    }

    if (this->Motions.size() == 0)
    {
      svtkGenericWarningMacro("All parsed `motion`s were skipped!");
      return false;
    }

    // now let's process and extra initializations needed by the active motions.
    for (const auto& pair : this->Motions)
    {
      for (const auto& motion : pair.second)
      {
        if (auto mpf = std::dynamic_pointer_cast<const impl::PositionFileMotion>(motion))
        {
          mpf->read_position_file(dir);
        }
      }
    }

    this->TimeRange[0] = SVTK_DOUBLE_MAX;
    this->TimeRange[1] = SVTK_DOUBLE_MIN;
    for (auto apair : this->Motions)
    {
      this->TimeRange[0] = std::min(apair.second.front()->tstart_prescribe, this->TimeRange[0]);
      this->TimeRange[1] = std::max(apair.second.back()->tend_prescribe, this->TimeRange[1]);
    }
    return this->TimeRange[0] <= this->TimeRange[1];
  }

  svtkSmartPointer<svtkPolyData> Move(unsigned int bodyIdx, double time) const
  {
    assert(bodyIdx < this->GetNumberOfBodies());

    auto pd = svtkSmartPointer<svtkPolyData>::New();
    pd->ShallowCopy(this->Geometries[bodyIdx].second);

    // deep copy points, since we'll need to modify them.
    svtkNew<svtkPoints> points;
    points->DeepCopy(pd->GetPoints());

    // how let's move!
    const auto iter = this->Motions.find(this->Geometries[bodyIdx].first);
    assert(iter != this->Motions.end());
    for (auto& motion_ptr : iter->second)
    {
      // since motions are sorted by tstart_prescribe and we're assured no
      // overlap, we can simply iterate in order.
      motion_ptr->Move(points, time);
    }
    pd->SetPoints(points);
    pd->Modified();
    return pd;
  }

  std::string GetBodyName(unsigned int bodyIdx) const
  {
    assert(bodyIdx < this->GetNumberOfBodies());
    return svtksys::SystemTools::GetFilenameWithoutExtension(this->Geometries[bodyIdx].first);
  }

  // do not call this before Parse().
  unsigned int GetNumberOfBodies() const
  {
    assert(this->Motions.size() == this->Geometries.size());
    return static_cast<unsigned int>(this->Motions.size());
  }

private:
  svtkInternals(const svtkInternals&) = delete;
  void operator=(const svtkInternals&) = delete;

  impl::MapOfVectorOfMotions Motions;
  svtkVector2d TimeRange;
  std::vector<std::pair<std::string, svtkSmartPointer<svtkPolyData> > > Geometries;
};

svtkStandardNewMacro(svtkMotionFXCFGReader);
//----------------------------------------------------------------------------
svtkMotionFXCFGReader::svtkMotionFXCFGReader()
  : FileName()
  , TimeResolution(100)
  , Internals(nullptr)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkMotionFXCFGReader::~svtkMotionFXCFGReader()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
void svtkMotionFXCFGReader::SetFileName(const char* fname)
{
  const std::string arg(fname ? fname : "");
  if (this->FileName != arg)
  {
    this->FileName = arg;
    this->FileNameMTime.Modified();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int svtkMotionFXCFGReader::RequestInformation(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  if (!this->ReadMetaData())
  {
    return 0;
  }

  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  auto trange = this->Internals->GetTimeRange();
  if (trange[1] > trange[0])
  {
    const double delta = (trange[1] - trange[0]) / this->TimeResolution;
    std::vector<double> timesteps(this->TimeResolution);
    for (int cc = 0; cc < this->TimeResolution - 1; ++cc)
    {
      timesteps[cc] = trange[0] + cc * delta;
    }
    timesteps.back() = trange[1];

    outInfo->Set(
      svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timesteps[0], this->TimeResolution);
    outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), trange.GetData(), 2);
  }
  else
  {
    outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkMotionFXCFGReader::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  if (!this->ReadMetaData())
  {
    return 0;
  }

  svtkMultiBlockDataSet* output = svtkMultiBlockDataSet::GetData(outputVector, 0);

  const auto& internals = (*this->Internals);
  output->SetNumberOfBlocks(internals.GetNumberOfBodies());

  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  double time = internals.GetTimeRange()[0];
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    time = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }

  // clamp the time request.
  time = std::max(time, internals.GetTimeRange()[0]);
  time = std::min(time, internals.GetTimeRange()[1]);

  for (unsigned int cc = 0, max = internals.GetNumberOfBodies(); cc < max; ++cc)
  {
    output->SetBlock(cc, internals.Move(cc, time));
    output->GetMetaData(cc)->Set(svtkMultiBlockDataSet::NAME(), internals.GetBodyName(cc).c_str());
  }
  return 1;
}

//----------------------------------------------------------------------------
bool svtkMotionFXCFGReader::ReadMetaData()
{
  if (this->FileNameMTime < this->MetaDataMTime)
  {
    return (this->Internals != nullptr);
  }

  delete this->Internals;
  this->Internals = nullptr;

  if (svtksys::SystemTools::TestFileAccess(
        this->FileName, svtksys::TEST_FILE_OK | svtksys::TEST_FILE_READ))
  {
    auto* interals = new svtkInternals();
    if (interals->Parse(this->FileName))
    {
      this->Internals = interals;
      this->MetaDataMTime.Modified();
      return true;
    }
    delete interals;
  }
  else
  {
    svtkErrorMacro("Cannot read file '" << this->FileName << "'.");
  }
  return (this->Internals != nullptr);
}

//----------------------------------------------------------------------------
void svtkMotionFXCFGReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << this->FileName << endl;
  os << indent << "TimeResolution: " << this->TimeResolution << endl;
}
