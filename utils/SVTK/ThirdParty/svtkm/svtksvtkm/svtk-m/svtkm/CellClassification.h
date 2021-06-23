//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_CellClassification_h
#define svtk_m_CellClassification_h

namespace svtkm
{

enum CellClassification : svtkm::UInt8
{
  NORMAL = 0,       //Valid cell
  GHOST = 1 << 0,   //Ghost cell
  INVALID = 1 << 1, //Cell is invalid
  UNUSED0 = 1 << 2,
  UNUSED1 = 1 << 3,
  UNUSED3 = 1 << 4,
  UNUSED4 = 1 << 5,
  UNUSED5 = 1 << 6,
};
}

#endif // svtk_m_CellClassification_h
