/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkmyEx1.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example creates a couple of class instances and print them to
// the standard output. No rendering window is created.
//

//
// First include the required header files for the svtk classes we are using
//
#include "svtkBar.h"
#include "svtkBar2.h"
#include "svtkImageFoo.h"

int main()
{

  //
  // Next we create an instance of svtkBar
  //
  cout << "Create svtkBar object and print it." << endl;

  svtkBar* bar = svtkBar::New();
  bar->Print(cout);

  //
  // Then we create an instance of svtkBar2
  //
  cout << "Create svtkBar2 object and print it." << endl;

  svtkBar2* bar2 = svtkBar2::New();
  bar2->Print(cout);

  //
  // And we create an instance of svtkImageFoo
  //
  cout << "Create svtkImageFoo object and print it." << endl;

  svtkImageFoo* imagefoo = svtkImageFoo::New();
  imagefoo->Print(cout);

  cout << "Looks good ?" << endl;

  //
  // Free up any objects we created
  //
  bar->Delete();
  bar2->Delete();
  imagefoo->Delete();

  return 0;
}
