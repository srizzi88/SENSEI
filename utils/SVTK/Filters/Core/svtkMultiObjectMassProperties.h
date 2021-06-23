/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiObjectMassProperties.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMultiObjectMassProperties
 * @brief   compute volume and area of objects in a polygonal mesh
 *
 * svtkMultiObjectMassProperties estimates the volume and the surface area of
 * a polygonal mesh. Multiple, valid closed objects may be represented, and
 * each object is assumed to be defined as a polyhedron defined by polygonal
 * faces (i.e., the faces do not have to be triangles). The algorithm
 * computes the total volume and area, as well as per object values which are
 * placed in data arrays. Note that an object is valid only if it is manifold
 * and closed (i.e., each edge is used exactly two times by two different
 * polygons). Invalid objects are processed but may produce inaccurate
 * results. Inconsistent polygon ordering is also allowed.
 *
 * The algorithm is composed of two basic parts. First a connected traversal
 * is performed to identify objects, detect whether the objects are valid,
 * and ensure that the composing polygons are ordered consistently. Next, in
 * threaded execution, a parallel process of computing areas and volumes is
 * performed. It is possible to skip the first part if the SkipValidityCheck
 * is enabled, AND a svtkIdTypeArray data array named "ObjectIds" is
 * associated with the polygon input (i.e., cell data) that enumerates which
 * object every polygon belongs to (i.e., indictaes that it is a boundary
 * polygon of a specified object).
 *
 * The algorithm implemented here is inspired by this paper:
 * http://chenlab.ece.cornell.edu/Publication/Cha/icip01_Cha.pdf. Also see
 * the Stackflow entry: https://stackoverflow.com/questions/1406029/
 * how-to-calculate-the-volume-of-a-3d-mesh-object-the-surface-of-which-is-made-up.
 * The general assumption here is that the model is of closed surface.  Also,
 * this approach requires triangulating the polygons so triangle meshes are
 * processed much faster. Finally, the volume and area calculations are done
 * in paraellel (threaded) after a connectivity pass is made (used to
 * identify objects and verify that they are manifold and closed).
 *
 * The output contains six additional data arrays. The arrays
 * "ObjectValidity", "ObjectVolumes" and "ObjectAreas" are placed in the
 * output field data.  These are arrays which indicate which objects are
 * valid; the volume of each object; and the surface area of each
 * object. Three additional arrays are placed in the output cell data, and
 * indicate, on a per polygons basis, which object the polygon bounds
 * "ObjectIds"; the polygon area "Areas"; and the contribution of volume
 * "Volumes".  Additionally, the TotalVolume and TotalArea is available after
 * the filter executes (i.e., the sum of the ObjectVolumes and ObjectAreas
 * arrays).
 *
 * Per object validity, as mentioned previously, is reported in the
 * ObjectValidity array. However another variable, AllValid, is set after
 * filter execution which indicates whether all objects are valid (!=0) or
 * not. This information can be used as a shortcut in case you want to skip
 * validity checking on an object-by-object basis.
 *
 * @warning
 * This filter operates on the polygonal data contained in the input
 * svtkPolyData. Other types (verts, lines, triangle strips) are ignored and
 * not passed to the output. The input polys and points, as well as
 * associated point and cell data, are passed through to the output.
 *
 * @warning
 * This filter is similar to svtkMassProperties. However svtkMassProperties
 * operates on triangle meshes and assumes only a single, closed, properly
 * oriented surface is represented. svtkMultiObjectMassProperties performs
 * additional topological and connectivity operations to identify separate
 * objects, and confirms that they are manifold. It also accommodates
 * inconsistent ordering.
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkMassProperties
 */

#ifndef svtkMultiObjectMassProperties_h
#define svtkMultiObjectMassProperties_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkDoubleArray;
class svtkUnsignedCharArray;
class svtkIdTypeArray;

class SVTKFILTERSCORE_EXPORT svtkMultiObjectMassProperties : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for construction, type and printing.
   */
  static svtkMultiObjectMassProperties* New();
  svtkTypeMacro(svtkMultiObjectMassProperties, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Indicate whether to skip the validity check (the first part of the
   * algorithm). By default this is off; however even if enabled validity
   * skipping will only occur if a svtkIdTypeArray named "ObjectIds" is also
   * provided on input to the filter.
   */
  svtkSetMacro(SkipValidityCheck, svtkTypeBool);
  svtkGetMacro(SkipValidityCheck, svtkTypeBool);
  svtkBooleanMacro(SkipValidityCheck, svtkTypeBool);
  //@}

  /**
   * Return the number of objects identified. This is valid only after the
   * filter executes. Check the ObjectValidity array which indicates which of
   * these identified objects are valid. Invalid objects may have incorrect
   * volume and area values.
   */
  svtkIdType GetNumberOfObjects() { return this->NumberOfObjects; }

  /**
   * Return whether all objects are valid or not. This is valid only after the
   * filter executes.
   */
  svtkTypeBool GetAllValid() { return this->AllValid; }

  /**
   * Return the summed volume of all objects. This is valid only after the
   * filter executes.
   */
  double GetTotalVolume() { return this->TotalVolume; }

  /**
   * Return the summed area of all objects. This is valid only after the
   * filter executes.
   */
  double GetTotalArea() { return this->TotalArea; }

protected:
  svtkMultiObjectMassProperties();
  ~svtkMultiObjectMassProperties() override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  // Data members supporting API
  svtkTypeBool SkipValidityCheck;
  svtkTypeBool AllValid;
  double TotalVolume;
  double TotalArea;

  // Internal data members supporting algorithm execution
  svtkIdType NumberOfObjects; // number of objects identified
  svtkIdTypeArray* ObjectIds; // for each input polygon, the object id that the polygon is in

  svtkUnsignedCharArray* ObjectValidity; // is it a valid object?
  svtkDoubleArray* ObjectVolumes;        // what is the object volume (if valid)?
  svtkDoubleArray* ObjectAreas;          // what is the total object area?

  svtkIdList* CellNeighbors; // avoid repetitive new/delete
  svtkIdList* Wave;          // processing wave
  svtkIdList* Wave2;

  // Connected traversal to identify objects
  void TraverseAndMark(
    svtkPolyData* output, svtkIdType* objectIds, svtkDataArray* valid, unsigned char* orient);

private:
  svtkMultiObjectMassProperties(const svtkMultiObjectMassProperties&) = delete;
  void operator=(const svtkMultiObjectMassProperties&) = delete;
};

#endif
