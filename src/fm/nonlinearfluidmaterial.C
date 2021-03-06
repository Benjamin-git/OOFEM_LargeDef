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

#include "fluiddynamicmaterial.h"
#include "nonlinearfluidmaterial.h"
#include "domain.h"
#include "floatmatrix.h"
#include "gausspoint.h"
#include "engngm.h"
#include "contextioerr.h"
#include "mathfem.h"
#include "dynamicinputrecord.h"
#include "classfactory.h"

#include <cstdlib>

namespace oofem {

REGISTER_Material( NonlinearFluidMaterial );

int
NonlinearFluidMaterial :: hasMaterialModeCapability(MaterialMode mode)
{
    if ( mode == _2dFlow ) {
        return 1;
    }

    return 0;
}


IRResultType
NonlinearFluidMaterial :: initializeFrom(InputRecord *ir)
{
    const char *__proc = "initializeFrom"; // Required by IR_GIVE_FIELD macro
    IRResultType result;                   // Required by IR_GIVE_FIELD macro

    this->FluidDynamicMaterial :: initializeFrom(ir);

    IR_GIVE_FIELD(ir, viscosity, _IFT_NonlinearFluidMaterial_mu);
    IR_GIVE_FIELD(ir, alpha, _IFT_NonlinearFluidMaterial_alpha);
    IR_GIVE_FIELD(ir, c, _IFT_NonlinearFluidMaterial_C);

    return IRRT_OK;
}


void
NonlinearFluidMaterial :: giveInputRecord(DynamicInputRecord &input)
{
    FluidDynamicMaterial :: giveInputRecord(input);
    input.setField(this->viscosity, _IFT_NonlinearFluidMaterial_mu);
    input.setField(this->alpha, _IFT_NonlinearFluidMaterial_alpha);
    input.setField(this->c, _IFT_NonlinearFluidMaterial_C);
}


double
NonlinearFluidMaterial :: giveEffectiveViscosity(GaussPoint *gp, TimeStep *tStep)
{
    return this->viscosity;
}


double
NonlinearFluidMaterial :: give(int aProperty, GaussPoint *gp)
{
    if ( aProperty == Viscosity ) {
        return viscosity;
    } else {
        return FluidDynamicMaterial :: give(aProperty, gp);
    }
}


MaterialStatus *
NonlinearFluidMaterial :: CreateStatus(GaussPoint *gp) const
{
    return new NonlinearFluidMaterialStatus(1, this->giveDomain(), gp);
}


void
NonlinearFluidMaterial :: computeDeviatoricStressVector(FloatArray &answer, GaussPoint *gp, const FloatArray &eps, TimeStep *tStep)
{
    NonlinearFluidMaterialStatus *status = static_cast< NonlinearFluidMaterialStatus * >( this->giveStatus(gp) );

    status->letTempDeviatoricStrainVectorBe(eps);

    double normeps;

    normeps = eps.at(1) * eps.at(1) + eps.at(2) * eps.at(2) + 0.5 * ( eps.at(3) * eps.at(3) );
    normeps = sqrt(normeps);

    answer = eps;

    answer.at(3) *= 0.5;
    answer.times( 2.0 * viscosity * ( 1.0 + c * pow(normeps, alpha) ) );

    status->letTempDeviatoricStressVectorBe(answer);
}

void
NonlinearFluidMaterial :: giveDeviatoricStiffnessMatrix(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp,
                                                        TimeStep *atTime)
{
    FloatArray eps;
    double normeps = 0;

    FloatMatrix op, t2;

    NonlinearFluidMaterialStatus *status = static_cast< NonlinearFluidMaterialStatus * >( this->giveStatus(gp) );
    eps = status->giveTempDeviatoricStrainVector();

    eps.at(3) *= 0.5;

    normeps = eps.at(1) * eps.at(1) + eps.at(2) * eps.at(2) + 2 * ( eps.at(3) * eps.at(3) );
    normeps = sqrt(normeps);

    op.resize(3, 3);
    op.beDyadicProductOf(eps, eps);
    if ( normeps != 0 ) {
        op.times( 2 * viscosity * c * alpha * pow(normeps, alpha - 2) );
    } else {
        op.times(0);
    }

    t2.resize(3, 3);
    t2.zero();
    for ( int i = 1; i <= 3; i++ ) {
        if ( normeps != 0 ) {
            t2.at(i, i) = 2 * viscosity * ( 1 + c * pow(normeps, alpha) );
        } else {
            t2.at(i, i) = 2 * viscosity;
        }
    }

    t2.at(3, 3) *= 0.5;

    answer.resize(3, 3);
    answer.zero();
    answer.add(op);
    answer.add(t2);
}

int
NonlinearFluidMaterial :: checkConsistency()
{
    if ( domain->giveEngngModel()->giveEquationScalingFlag() ) {
        double scale;
        scale = domain->giveEngngModel()->giveVariableScale(VST_Density);
        propertyDictionary->at('d') /= scale;

        scale = domain->giveEngngModel()->giveVariableScale(VST_Viscosity);
        this->viscosity /= scale;
    }

    return 1;
}

NonlinearFluidMaterialStatus :: NonlinearFluidMaterialStatus(int n, Domain *d, GaussPoint *g) :
    FluidDynamicMaterialStatus(n, d, g)
{
    temp_deviatoricStrainVector.resize(3);
    temp_deviatoricStrainVector.zero();
}

void
NonlinearFluidMaterialStatus :: initTempStatus()
{
    FluidDynamicMaterialStatus :: initTempStatus();

    temp_deviatoricStrainVector = deviatoricStrainVector;
}

void
NonlinearFluidMaterialStatus :: updateYourself(TimeStep *tStep)
{
    FluidDynamicMaterialStatus :: updateYourself(tStep);

    deviatoricStrainVector = temp_deviatoricStrainVector;
}
} // end namespace oofem
