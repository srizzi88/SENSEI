/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmf3ArrayKeeper.cxx
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkXdmf3ArrayKeeper.h"

// clang-format off
#include "svtk_xdmf3.h"
#include SVTKXDMF3_HEADER(core/XdmfArray.hpp)
// clang-format on

//------------------------------------------------------------------------------
svtkXdmf3ArrayKeeper::svtkXdmf3ArrayKeeper()
{
  generation = 0;
}

//------------------------------------------------------------------------------
svtkXdmf3ArrayKeeper::~svtkXdmf3ArrayKeeper()
{
  this->Release(true);
}

//------------------------------------------------------------------------------
void svtkXdmf3ArrayKeeper::BumpGeneration()
{
  this->generation++;
}

//------------------------------------------------------------------------------
void svtkXdmf3ArrayKeeper::Insert(XdmfArray* val)
{
  this->operator[](val) = this->generation;
}

//------------------------------------------------------------------------------
void svtkXdmf3ArrayKeeper::Release(bool force)
{
  svtkXdmf3ArrayKeeper::iterator it = this->begin();
  // int cnt = 0;
  // int total = 0;
  while (it != this->end())
  {
    // total++;
    svtkXdmf3ArrayKeeper::iterator current = it++;
    if (force || (current->second != this->generation))
    {
      XdmfArray* atCurrent = current->first;
      atCurrent->release();
      this->erase(current);
      // cnt++;
    }
  }
  // cerr << "released " << cnt << "/" << total << " arrays" << endl;
}
