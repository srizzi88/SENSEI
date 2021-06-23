/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUpdateCellsV8toV9.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkUpdateCellsV8toV9
 * @brief   Update cells from v8 node layout to v9 node layout
 */

#ifndef svtkUpdateCellsV8toV9_h
#define svtkUpdateCellsV8toV9_h

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellType.h"
#include "svtkCellTypes.h"
#include "svtkHigherOrderHexahedron.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkNew.h"
#include "svtkUnstructuredGrid.h"

inline void svtkUpdateCellsV8toV9(svtkUnstructuredGrid* output)
{
  svtkNew<svtkIdList> oldpts, newpts;

  for (svtkIdType i = 0; i < output->GetNumberOfCells(); ++i)
  {
    svtkIdType type = output->GetCellTypesArray()->GetTypedComponent(i, 0);
    if (type == SVTK_HIGHER_ORDER_HEXAHEDRON || type == SVTK_LAGRANGE_HEXAHEDRON ||
      type == SVTK_BEZIER_HEXAHEDRON)
    {
      output->GetCells()->GetCellAtId(i, oldpts);
      newpts->DeepCopy(oldpts);

      int degs[3];
      if (output->GetCellData()->SetActiveAttribute(
            "HigherOrderDegrees", svtkDataSetAttributes::AttributeTypes::HIGHERORDERDEGREES) != -1)
      {
        svtkDataArray* v = output->GetCellData()->GetHigherOrderDegrees();
        double degs_double[3];
        v->GetTuple(i, degs_double);
        for (int ii = 0; ii < 3; ii++)
          degs[ii] = static_cast<int>(degs_double[ii]);
      }
      else
      {
        int order =
          static_cast<int>(round(std::cbrt(static_cast<int>(oldpts->GetNumberOfIds())))) - 1;
        degs[0] = degs[1] = degs[2] = order;
      }
      for (int j = 0; j < oldpts->GetNumberOfIds(); j++)
      {
        int newid = svtkHigherOrderHexahedron::NodeNumberingMappingFromSVTK8To9(degs, j);
        if (j != newid)
        {
          newpts->SetId(j, oldpts->GetId(newid));
        }
      }
      output->GetCells()->ReplaceCellAtId(i, newpts);
    }
  }
}

inline bool svtkNeedsNewFileVersionV8toV9(svtkCellTypes* cellTypes)
{
  int nCellTypes = cellTypes->GetNumberOfTypes();
  for (svtkIdType i = 0; i < nCellTypes; ++i)
  {
    unsigned char type = cellTypes->GetCellType(i);
    if (type == SVTK_HIGHER_ORDER_HEXAHEDRON || type == SVTK_LAGRANGE_HEXAHEDRON ||
      type == SVTK_BEZIER_HEXAHEDRON)
    {
      return true;
    }
  }
  return false;
}

#endif // svtkUpdateCellsV8toV9_h
// SVTK-HeaderTest-Exclude: svtkUpdateCellsV8toV9.h
