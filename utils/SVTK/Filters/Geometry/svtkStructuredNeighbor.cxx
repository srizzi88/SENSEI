/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkStructuredNeighbor.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkStructuredNeighbor.h"
#include "svtkStructuredExtent.h"

svtkStructuredNeighbor::svtkStructuredNeighbor()
{
  this->NeighborID = 0;
  this->OverlapExtent[0] = this->OverlapExtent[1] = this->OverlapExtent[2] =
    this->OverlapExtent[3] = this->OverlapExtent[4] = this->OverlapExtent[5] = 0;
  this->Orientation[0] = this->Orientation[1] = this->Orientation[2] =
    svtkStructuredNeighbor::UNDEFINED;

  for (int i = 0; i < 6; ++i)
  {
    this->SendExtent[i] = this->RcvExtent[i] = -1;
  }
}

//------------------------------------------------------------------------------
svtkStructuredNeighbor::svtkStructuredNeighbor(const int neiId, int overlap[6])
{
  this->NeighborID = neiId;
  for (int i = 0; i < 6; ++i)
  {
    this->SendExtent[i] = this->RcvExtent[i] = this->OverlapExtent[i] = overlap[i];
  }
}

//------------------------------------------------------------------------------
svtkStructuredNeighbor::svtkStructuredNeighbor(const int neiId, int overlap[6], int orient[3])
{
  this->NeighborID = neiId;
  for (int i = 0; i < 3; ++i)
  {
    this->RcvExtent[i * 2] = this->SendExtent[i * 2] = this->OverlapExtent[i * 2] = overlap[i * 2];

    this->RcvExtent[i * 2 + 1] = this->SendExtent[i * 2 + 1] = this->OverlapExtent[i * 2 + 1] =
      overlap[i * 2 + 1];

    this->Orientation[i] = orient[i];
  }
}

//------------------------------------------------------------------------------
svtkStructuredNeighbor::~svtkStructuredNeighbor() = default;

//------------------------------------------------------------------------------
void svtkStructuredNeighbor::ComputeSendAndReceiveExtent(int gridRealExtent[6],
  int* svtkNotUsed(gridGhostedExtent[6]), int neiRealExtent[6], int WholeExtent[6], const int N)
{

  for (int i = 0; i < 3; ++i)
  {
    switch (this->Orientation[i])
    {
      case svtkStructuredNeighbor::SUPERSET:
        this->SendExtent[i * 2] -= N;
        this->SendExtent[i * 2 + 1] += N;
        break;
      case svtkStructuredNeighbor::SUBSET_HI:
      case svtkStructuredNeighbor::HI:
        this->RcvExtent[i * 2 + 1] += N;
        this->SendExtent[i * 2] -= N;
        break;
      case svtkStructuredNeighbor::SUBSET_LO:
      case svtkStructuredNeighbor::LO:
        this->RcvExtent[i * 2] -= N;
        this->SendExtent[i * 2 + 1] += N;
        break;
      case svtkStructuredNeighbor::SUBSET_BOTH:
        this->RcvExtent[i * 2] -= N;
        this->SendExtent[i * 2 + 1] += N;
        this->RcvExtent[i * 2 + 1] += N;
        this->SendExtent[i * 2] -= N;
        break;
      default:; /* NO OP */
    }
  } // END for all dimensions

  // Hmm...restricting receive extent to the real extent of the neighbor
  svtkStructuredExtent::Clamp(this->RcvExtent, neiRealExtent);
  svtkStructuredExtent::Clamp(this->SendExtent, gridRealExtent);
  svtkStructuredExtent::Clamp(this->RcvExtent, WholeExtent);
  svtkStructuredExtent::Clamp(this->SendExtent, WholeExtent);
}
