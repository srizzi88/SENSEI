/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFortran.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * The whole pupose of this header file is to allow us to 'wrap' a c library
 * into Fortran. Here are the basic goals we should reach:
 *
 * 1. The symbol mangling in fortran is different than is C. For example if
 * you do this fortran:
 *     CALL Foo()
 * you either need to define a symbol:
 *    - void FOO()
 *    - void foo()
 *    - void FOO_()
 *    - void foo_()
 *
 *  2. Passing array, Fortran start at 1 instead of 0.
 *
 *  3. Passing STRING array from fortran to C is a bit more tricky.
 *
 *  For more information refer to:
 *  [ FORTRAN/C INTEROPERABILITY  ]
 *  http://www.ibiblio.org/pub/languages/fortran/ch1-11.html
 *
 *  [ Data Type Compatibility ]
 *  http://www.ictp.trieste.it/~manuals/programming/sun/fortran/prog_guide/11_cfort.doc.html
 */
#ifndef svtkFortran_h
#define svtkFortran_h

#define SVTK_FORTRAN_NAME(name, NAME) name##__

#define SVTK_FORTRAN_ARG_STRING(__arg) const char *__arg##_string, unsigned int __arg##_length

#define SVTK_FORTRAN_REF_STRING_POINTER(__arg) __arg##_string

#define SVTK_FORTRAN_REF_STRING_LENGTH(__arg) __arg##_length

#define SVTK_FORTRAN_ARG_INTEGER4(data) int* data

#define SVTK_FORTRAN_ARG_REAL4_ARRAY_1D(array) float* array

#define SVTK_FORTRAN_ARG_INTEGER8(size) svtkIdType* size

#define SVTK_FORTRAN_REF_REAL4_ARRAY_1D(array) array

#define SVTK_FORTRAN_ARG_INTEGER8_ARRAY_1D(array) svtkIdType* array

#define SVTK_FORTRAN_REF_INTEGER8_ARRAY_1D(array) array

#define SVTK_FORTRAN_REF_INTEGER4(data) *data

#define SVTK_FORTRAN_REF_INTEGER8(data) *data

#define SVTK_FORTRAN_ARG_INTEGER4_ARRAY_1D(array) int* array

#define SVTK_FORTRAN_ARG_REAL8(t) double* t

#define SVTK_FORTRAN_REF_REAL8(t) *t

#define SVTK_FORTRAN_ARG_INT4(n) int* n

#define SVTK_FORTRAN_REF_INT4(n) *n

#endif
