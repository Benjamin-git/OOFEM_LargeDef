/*
 *
 *                 #####    #####   ######  ######  ###   ###
 *               ##   ##  ##   ##  ##      ##      ## ### ##
 *              ##   ##  ##   ##  ####    ####    ##  #  ##
 *             ##   ##  ##   ##  ##      ##      ##     ##
 *            ##   ##  ##   ##  ##      ##      ##     ##
 *            #####    #####   ##      ######  ##     ##
 *
 *
 *             OOFEM : Object Oriented Finite Element Code
 *
 *               Copyright (C) 1993 - 2008   Borek Patzak
 *
 *
 *
 *       Czech Technical University, Faculty of Civil Engineering,
 *   Department of Structural Mechanics, 166 29 Prague, Czech Republic
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

//   ****************************
//   *** CLASS SPRING ELEMENT ***
//   ****************************


#ifndef springelement_h
#define springelement_h


#include "structuralelement.h"

namespace oofem {


/*
* This class implements a simple spring element. Its purpose is to introduce
* a longitudinal or torsional spring between two nodes. The spring element is defined
* by its spring constant and orientation.
*/
class SpringElement : public StructuralElement
{
 public:
  /// Defines type of spring element (longitudial/rotational) spring
  enum SpringElementType {
    SE_1D_SPRING = 0, // 1D spring element along x-axis 
    SE_2D_SPRING_XY = 1, // 2D spring element in xy plane, requires D_u and D_v DOFs in each node (orientation vector should be in this plane)
    SE_2D_TORSIONALSPRING_XZ = 2, // 2D torsional spring element in xz plane, requires R_v DOFs in each node
    SE_3D_SPRING = 3,    // 3D spring element in space, requires D_u, D_v, and D_w DOFs in each node
    SE_3D_TORSIONALSPRING = 4 // 3D torsional spring in space, requires R_u, R_v, and R_w DOFs in each node
  };
 
protected:
  /// The longitudinal spring constant [Force/Length], torsional spring constant [Force*Length/Radian]
  double springConstant;
  /**
   * Orientation vector.  Defines orientation of spring element- for spring it defines the direction of spring, 
   * for torsional spring it defines the axis of rotation.
   */
  FloatArray dir;
  /// mode
  SpringElementType mode;
  
public:
    SpringElement(int, Domain *);                         // constructor
    ~SpringElement()   { }                                // destructor

    void          computeLumpedMassMatrix(FloatMatrix &answer, TimeStep *tStep) {}
    void          computeMassMatrix(FloatMatrix &answer, TimeStep *tStep)
    { computeLumpedMassMatrix(answer, tStep); }
    void          computeStiffnessMatrix(FloatMatrix &answer, MatResponseMode rMode, TimeStep *tStep);
    void          computeInitialStressMatrix(FloatMatrix &answer, TimeStep *tStep)
    { answer.resize(0, 0); }
    void          computeNonForceLoadVector(FloatArray &answer, TimeStep *, ValueModeType mode)
    { answer.resize(0); }
    void computeForceLoadVector(FloatArray &answer, TimeStep *, ValueModeType)
    { answer.resize(0); }
    void giveInternalForcesVector(FloatArray &answer, TimeStep *, int useUpdatedGpRecord = 0);

    int           computeNumberOfDofs(EquationID ut);
    void giveDofManDofIDMask(int inode, EquationID, IntArray &) const;

    void          updateInternalState(TimeStep *) {}
    void          updateYourself(TimeStep *tStep) {}
    int           checkConsistency() {return 1;}
    void          printOutputAt(FILE *, TimeStep *);

#ifdef __OOFEG
    //void          drawRawGeometry(oofegGraphicContext &);
    //void drawDeformedGeometry(oofegGraphicContext &, UnknownType);
    //void          drawScalar(oofegGraphicContext &context);
#endif
    //
    // definition & identification
    //
    const char *giveClassName() const { return "SpringElement"; }
    classType            giveClassID() const { return SpringElementClass; }
    IRResultType initializeFrom(InputRecord *ir);
    Element_Geometry_Type giveGeometryType() const { return EGT_point; }
    
protected:
    void  computeBmatrixAt(GaussPoint *, FloatMatrix &answer,
                           int lowerIndx = 1, int upperIndx = ALL_STRAINS)
    {}
    void  computeNmatrixAt(GaussPoint *, FloatMatrix &) {}
    int           computeGtoLRotationMatrix(FloatMatrix &); // giveRotationMatrix () ;
    double computeSpringInternalForce (TimeStep *stepN);
};
} // end namespace oofem
#endif // springelement_h