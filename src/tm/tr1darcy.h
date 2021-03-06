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
 *               Copyright (C) 1993 - 2013   Borek Patzak
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

#ifndef tr1darcy_h_
#define tr1darcy_h_

#include "transportelement.h"
#include "fei2dtrlin.h"
#include "domain.h"
#include "nodalaveragingrecoverymodel.h"
#include "transportmaterial.h"

#define _IFT_Tr1Darcy_Name "tr1darcy"

namespace oofem {
/**
 * Element class for the DarcyFlow engineering model. Linear description of seepage
 * @author Carl Sandström
 */
class Tr1Darcy : public TransportElement, public NodalAveragingRecoveryModelInterface
{
protected:
    static FEI2dTrLin interpolation_lin;

public:
    Tr1Darcy(int, Domain *);
    virtual ~Tr1Darcy();
    virtual IRResultType initializeFrom(InputRecord *ir);

    virtual FEInterpolation *giveInterpolation() const { return &interpolation_lin; }

    virtual MaterialMode giveMaterialMode() { return _2dHeat; } ///@todo This isn't actually correct.

    virtual void giveDofManDofIDMask(int inode, EquationID ut, IntArray &answer) const;
    virtual void giveCharacteristicVector(FloatArray &answer, CharType mtrx, ValueModeType mode, TimeStep *tStep);
    virtual void giveCharacteristicMatrix(FloatMatrix &answer, CharType mtrx, TimeStep *tStep);
    virtual void computeStiffnessMatrix(FloatMatrix &answer, TimeStep *atTime);
    virtual void computeLoadVector(FloatArray &answer, TimeStep *atTime);
    virtual void computeGaussPoints();
    virtual int computeNumberOfDofs(EquationID ut);

    void computeEdgeBCSubVectorAt(FloatArray &answer, Load *load, int iEdge, TimeStep *tStep);
    void computeInternalForcesVector(FloatArray &answer, TimeStep *atTime);

    virtual double computeEdgeVolumeAround(GaussPoint *gp, int iEdge);
    virtual double giveThicknessAt(const FloatArray &gcoords);

    // From NodalAveragingRecoveryModelInterface
    virtual const char *giveInputRecordName() const { return _IFT_Tr1Darcy_Name; }
    virtual const char *giveClassName() const { return "Tr1Darcy"; };
    virtual void NodalAveragingRecoveryMI_computeNodalValue(FloatArray &answer, int node, InternalStateType type, TimeStep *tStep);
    virtual void NodalAveragingRecoveryMI_computeSideValue(FloatArray &answer, int side, InternalStateType type, TimeStep *tStep);
    virtual Interface *giveInterface(InterfaceType interface);
};
}

#endif // tr1darcy_h_
