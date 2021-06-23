#!/bin/env svtkpython
import svtk
from svtk.test import Testing

class  kMeansDistanceCalculator( Testing.svtkTest ):
  def testDistanceCalculator(self):
    "See whether the distance calculator evaluates functions properly."
    xv = svtk.svtkDoubleArray()
    yv = svtk.svtkDoubleArray()
    xv.SetName( 'x' )
    yv.SetName( 'y' )
    tab = svtk.svtkTable()
    xv.InsertNextValue( 0.0 )
    xv.InsertNextValue( 1.0 )
    xv.InsertNextValue( 0.0 )
    xv.InsertNextValue( 1.0 )
    xv.InsertNextValue( 5.0 )
    yv.InsertNextValue( 0.0 )
    yv.InsertNextValue( 0.0 )
    yv.InsertNextValue( 1.0 )
    yv.InsertNextValue( 1.0 )
    yv.InsertNextValue( 12.0 )
    tab.AddColumn( xv )
    tab.AddColumn( yv )
    km = svtk.svtkKMeansStatistics()
    km.SetDefaultNumberOfClusters( 1 )
    df = svtk.svtkKMeansDistanceFunctorCalculator()
    km.SetDistanceFunctor( df )
    df.SetDistanceExpression( 'sqrt( (x0-y0)*(x0-y0) + (x1-y1)*(x1-y1) )' )
    km.SetInputData( tab )
    km.SetColumnStatus( 'x', 1 )
    km.SetColumnStatus( 'y', 1 )
    km.RequestSelectedColumns()
    km.SetLearnOption( 1 )
    km.SetDeriveOption( 1 )
    km.SetAssessOption( 1 )
    km.Update()
    av = km.GetOutput( 0 )
    # We should always converge to a cluster center at [ 1.4, 2.8 ]
    # These distances are the distances of each input observation to that cluster center:
    dists = [ 3.1304951684997055, 2.8284271247461898, 2.2803508501982757, 1.8439088914585773, 9.879271228182775 ]
    for i in range(5):
      self.failUnlessAlmostEqual( dists[i], av.GetColumn( 2 ).GetValue( i ) )

  def testParse(self):
    "Test if svtkKMeansDistanceFunctorCalculator is parseable"
    tg = svtk.svtkKMeansDistanceFunctorCalculator()
    self._testParse(tg)

  def testGetSet(self):
    "Testing Get/Set methods of svtkKMeansDistanceFunctorCalculator"
    tg = svtk.svtkKMeansDistanceFunctorCalculator()
    self._testGetSet(tg)

  def testParse(self):
    "Testing Boolean methods of svtkKMeansDistanceFunctorCalculator"
    tg = svtk.svtkKMeansDistanceFunctorCalculator()
    self._testBoolean(tg)

if __name__ == "__main__":
    Testing.main([(kMeansDistanceCalculator, 'test')])
