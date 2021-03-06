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

#ifndef hydratinghemomat_h
#define hydratinghemomat_h

#include "hemotkmat.h"
#include "../sm/hydram.h"

///@name Input fields for HydratingHeMoMaterial
//@{
#define _IFT_HydratingHeMoMaterial_Name "hhemotk"
#define _IFT_HydratingHeMoMaterial_hydration "hydration"
#define _IFT_HydratingHeMoMaterial_mix "mix"
#define _IFT_HydratingHeMoMaterial_noHeat "noheat"
#define _IFT_HydratingHeMoMaterial_noLHS "nolhs"
//@}

namespace oofem {

/**
 * Heat and moisture transport material with hydration.
 */
class HydratingHeMoMaterial : public HeMoTKMaterial, public HydrationModelInterface
{
protected:
    int hydration, hydrationHeat, hydrationLHS, teplotaOut;

public:
    HydratingHeMoMaterial(int n, Domain *d) : HeMoTKMaterial(n, d), HydrationModelInterface() { }
    virtual ~HydratingHeMoMaterial() { }

    void setMixture(MixtureType mix);

    virtual int hasInternalSource(); // return true if hydration heat source is present
    virtual void computeInternalSourceVector(FloatArray &val, GaussPoint *gp, TimeStep *atTime, ValueModeType mode);
    virtual void updateInternalState(const FloatArray &state, GaussPoint *gp, TimeStep *tStep);

    virtual double giveCharacteristicValue(MatResponseMode mode,
                                           GaussPoint *gp,
                                           TimeStep *atTime);

    // saves current context(state) into stream
    virtual contextIOResultType saveIPContext(DataStream *stream, ContextMode mode, GaussPoint *gp);
    virtual contextIOResultType restoreIPContext(DataStream *stream, ContextMode mode, GaussPoint *gp);

    // identification and auxiliary functions
    virtual const char *giveInputRecordName() const { return _IFT_HydratingHeMoMaterial_Name; }
    virtual const char *giveClassName() const { return "HydratingHeMoMaterial"; }
    virtual classType giveClassID() const { return HydratingHeMoMaterialClass; }

    virtual IRResultType initializeFrom(InputRecord *ir);

    // post-processing
    virtual int giveIPValue(FloatArray &answer, GaussPoint *aGaussPoint, InternalStateType type, TimeStep *atTime);
    virtual InternalStateValueType giveIPValueType(InternalStateType type);

protected:
    virtual MaterialStatus *CreateStatus(GaussPoint *gp) const;
};
} // end namespace oofem
#endif // hydratinghemomat_h
