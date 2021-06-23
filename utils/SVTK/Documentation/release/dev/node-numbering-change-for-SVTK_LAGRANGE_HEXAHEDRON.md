# node-numbering-change-for-SVTK_LAGRANGE_HEXAHEDRON

The node numbering for SVTK_LAGRANGE_HEXAHEDRON has been corrected,
to match the numbering of SVTK_QUADRATIC_HEXAHEDRON when the Lagrange
cell is quadratic.
The change consists in inverting the node numbering on the edge (x=0, y=1)
with the edge (x=1, y=1), has shown below for a quadratic cell.


       quadratic                         SVTK_QUADRATIC_HEXAHEDRON
SVTK_LAGRANGE_HEXAHEDRON                  SVTK_LAGRANGE_HEXAHEDRON
    before SVTK 9.0                           after SVTK 9.0

       +_____+_____+                        +_____+_____+
       |\          :\                       |\          :\
       | +         : +                      | +         : +
       |  \     19 +  \                     |  \     18 +  \
    18 +   +-----+-----+                 19 +   +-----+-----+
       |   |       :   |                    |   |       :   |
       |__ | _+____:   |                    |__ | _+____:   |
       \   +        \  +                    \   +        \  +
        +  |         + |                     +  |         + |
         \ |          \|                      \ |          \|
           +_____+_____+                        +_____+_____+

In order to maintain the compatibility, the readers can convert internally
existing files to the new numbering. To this end, The XML file version has been
bumped from 2.1 to 2.2 and the legacy file version has been bumped from 5.0 to 5.1.
