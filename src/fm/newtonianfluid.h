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

#ifndef newtonianfluid_h
#define newtonianfluid_h

#include "fluiddynamicmaterial.h"
#include "floatarray.h"
#include "floatmatrix.h"
#include "matconst.h"
#include "matstatus.h"

///@name Input fields for NewtonianFluidMaterial
//@{
#define _IFT_NewtonianFluidMaterial_Name "newtonianfluid"
#define _IFT_NewtonianFluidMaterial_mu "mu"
//@}

namespace oofem {
class GaussPoint;

/**
 * Constitutive model of Newtonian fluid
 */
class NewtonianFluidMaterial : public FluidDynamicMaterial
{
protected:
    double viscosity;

public:
    /**
     * Constructor. Creates material with given number, belonging to given domain.
     * @param n Material number.
     * @param d Domain to which new material will belong.
     */
    NewtonianFluidMaterial(int n, Domain *d) : FluidDynamicMaterial(n, d) { }
    /// Destructor.
    virtual ~NewtonianFluidMaterial() { }

    virtual double giveEffectiveViscosity(GaussPoint *gp, TimeStep *tStep);

    virtual void computeDeviatoricStressVector(FloatArray &answer, GaussPoint *gp, const FloatArray &eps, TimeStep *tStep);
    virtual void giveDeviatoricStiffnessMatrix(FloatMatrix &answer, MatResponseMode, GaussPoint *gp, TimeStep *tStep);

    virtual double give(int aProperty, GaussPoint *gp);
    virtual IRResultType initializeFrom(InputRecord *ir);
    virtual void giveInputRecord(DynamicInputRecord &input);
    virtual int hasMaterialModeCapability(MaterialMode mode);
    virtual const char *giveClassName() const { return "NewtonianFluidMaterial"; }
    virtual const char *giveInputRecordName() const { return _IFT_NewtonianFluidMaterial_Name; }
    virtual classType giveClassID() const { return NewtonianFluidMaterialClass; }
    virtual int checkConsistency();
    virtual MaterialStatus *CreateStatus(GaussPoint *gp) const;
};
} // end namespace oofem
#endif // newtonianfluid_h
