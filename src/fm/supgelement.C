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

#include "supgelement.h"
#include "domain.h"
#include "load.h"
#include "gausspoint.h"
#include "intarray.h"
#include "floatarray.h"
#include "floatmatrix.h"
#include "fluiddynamicmaterial.h"

#ifdef __OOFEG
 #include "oofeggraphiccontext.h"
 #include "connectivitytable.h"
#endif

namespace oofem {
SUPGElement :: SUPGElement(int n, Domain *aDomain) :
    FMElement(n, aDomain), t_supg(0), t_pspg(0), t_lsic(0)
{ }


SUPGElement :: ~SUPGElement()
// Destructor.
{ }

IRResultType
SUPGElement :: initializeFrom(InputRecord *ir)
{
    const char *__proc = "initializeFrom"; // Required by IR_GIVE_FIELD macro
    IRResultType result;                   // Required by IR_GIVE_FIELD macro

    FMElement :: initializeFrom(ir);

    IR_GIVE_OPTIONAL_FIELD(ir, boundarySides, _IFT_SUPGElement_bsides);
    if ( !boundarySides.isEmpty() ) {
        IR_GIVE_FIELD(ir, boundaryCodes, _IFT_SUPGElement_bcodes);
    }

    return IRRT_OK;
}



void
SUPGElement :: giveCharacteristicMatrix(FloatMatrix &answer,
                                         CharType mtrx, TimeStep *tStep)
//
// returns characteristics matrix of receiver according to mtrx
//
{
    _error("giveCharacteristicMatrix: Unknown Type of characteristic mtrx.");
}


void
SUPGElement :: giveCharacteristicVector(FloatArray &answer, CharType mtrx, ValueModeType mode,
                                         TimeStep *tStep)
//
// returns characteristics vector of receiver according to requested type
//
{
    if ( mtrx == ExternalForcesVector) {
        // stokes flow
        IntArray vloc, ploc;
        FloatArray h;
        int size = this->computeNumberOfDofs(EID_MomentumBalance_ConservationEquation);
        this->giveLocalVelocityDofMap(vloc);
        this->giveLocalPressureDofMap(ploc);
        answer.resize(size); answer.zero();
        this->computeBCRhsTerm_MB(h, tStep); answer.assemble(h, vloc);
        this->computeBCRhsTerm_MC(h, tStep); answer.assemble(h, ploc);
    }
#if 0 
    else if ( mtrx == InternalForcesVector) {
        // stokes flow
        IntArray vloc, ploc;
        FloatArray h;
        int size = this->computeNumberOfDofs(EID_MomentumBalance_ConservationEquation);
        this->giveLocalVelocityDofMap(vloc);
        this->giveLocalPressureDofMap(ploc);
        answer.resize(size); answer.zero();
        //this->computeAdvectionTerm_MB(h, tStep); answer.assemble(h, vloc);
        this->computeAdvectionTerm_MC(h, tStep); answer.assemble(h, ploc);
        this->computeDiffusionTerm_MB(h, tStep); answer.assemble(h, vloc);
        this->computeDiffusionTerm_MC(h, tStep); answer.assemble(h, ploc);

        FloatMatrix m1;
        FloatArray v,p;
        // add lsic stabilization term
        //this->giveCharacteristicMatrix(m1, LSICStabilizationTerm_MB, tStep);
        //m1.times( lscale / ( dscale * uscale * uscale ) );
        this->computeVectorOf(EID_MomentumBalance, VM_Total, tStep, v);
        //h.beProductOf(m1, v);
        //answer.assemble(h, vloc);
        this->giveCharacteristicMatrix(m1, LinearAdvectionTerm_MC, tStep);
        //m1.times( 1. / ( dscale * uscale ) );
        h.beProductOf(m1, v);
        answer.assemble(h,ploc);

        // add pressure term
        this->giveCharacteristicMatrix(m1, PressureTerm_MB, tStep);
        this->computeVectorOf(EID_ConservationEquation, VM_Total, tStep, v);
        h.beProductOf(m1, v);
        answer.assemble(h, vloc);

        // pressure term
        this->giveCharacteristicMatrix(m1, PressureTerm_MC, tStep);
        this->computeVectorOf(EID_ConservationEquation, VM_Total, tStep, v);
        h.beProductOf(m1, v);
        answer.assemble(h, ploc);

    } 
#endif
    else {
        _error("giveCharacteristicVector: Unknown Type of characteristic mtrx.");
    }
}





void
SUPGElement :: computeDeviatoricStress(FloatArray &answer, GaussPoint *gp, TimeStep *tStep)
{
    FloatArray eps;

    // compute deviatoric strain
    this->computeDeviatoricStrain(eps, gp, tStep);
    // call material to compute stress
    static_cast< FluidDynamicMaterial * >( this->giveMaterial() )->computeDeviatoricStressVector(answer, gp, eps, tStep);
}



void
SUPGElement :: computeBCLhsTerm_MB(FloatMatrix &answer, TimeStep *atTime)
{
    bcType boundarytype;
    int nLoads = 0;
    int undofs = this->computeNumberOfDofs(EID_MomentumBalance);
    Load *load;
    //bcType loadtype;
    FloatMatrix helpMatrix;
    // loop over boundary load array
    helpMatrix.resize(undofs, undofs);
    helpMatrix.zero();

    answer.resize(undofs, undofs);
    answer.zero();

    nLoads = this->giveBoundaryLoadArray()->giveSize() / 2;
    if ( nLoads ) {
        for ( int i = 1; i <= nLoads; i++ ) {
            int n = boundaryLoadArray.at(1 + ( i - 1 ) * 2);
            int side = boundaryLoadArray.at(i * 2);
            load = domain->giveLoad(n);
            boundarytype = load->giveType();
            if ( boundarytype == SlipWithFriction ) {
                this->computeSlipWithFrictionBCTerm_MB(helpMatrix, load, side, atTime);
            } else if ( boundarytype == PenetrationWithResistance ) {
                this->computePenetrationWithResistanceBCTerm_MB(helpMatrix, load, side, atTime);
            } else {
                helpMatrix.resize(undofs, undofs);
                helpMatrix.zero();
                // _error("computeForceLoadVector : unsupported load type class");
            }

            answer.add(helpMatrix);
        }
    }
    nLoads = this->giveBodyLoadArray()->giveSize();

    if ( nLoads ) { 
        bcGeomType ltype;
        for ( int i = 1; i <= nLoads; i++ ) {
            load  = domain->giveLoad( bodyLoadArray.at(i) );
            ltype = load->giveBCGeoType();
            if ( ( ltype == BodyLoadBGT ) && ( load->giveBCValType() == ReinforceBVT ) ) {
                this->computeHomogenizedReinforceTerm_MB(helpMatrix, load, atTime);
            }
        }
    }
}

void
SUPGElement :: computeBCLhsPressureTerm_MB(FloatMatrix &answer, TimeStep *atTime)
{
    bcType boundarytype;
    int nLoads = 0;
    int undofs = this->computeNumberOfDofs(EID_MomentumBalance);
    int pndofs = this->computeNumberOfDofs(EID_ConservationEquation);
    Load *load;
    //bcType loadtype;
    FloatMatrix helpMatrix;
    // loop over boundary load array
    helpMatrix.resize(undofs, pndofs);
    helpMatrix.zero();

    answer.resize(undofs, pndofs);
    answer.zero();

    nLoads = this->giveBoundaryLoadArray()->giveSize() / 2;

    if ( nLoads ) {
        for ( int i = 1; i <= nLoads; i++ ) {
            int n = boundaryLoadArray.at(1 + ( i - 1 ) * 2);
            int side = boundaryLoadArray.at(i * 2);
            load = domain->giveLoad(n);
            boundarytype = load->giveType();
            if ( boundarytype == OutFlowBC ) {
                this->computeOutFlowBCTerm_MB(helpMatrix, side, atTime);
            } else {
                helpMatrix.resize(undofs, pndofs);
                helpMatrix.zero();
                //_warning("computeForceLoadVector : unsupported load type class");
            }

            answer.add(helpMatrix);
        }
    }
}

void
SUPGElement :: computeBCLhsPressureTerm_MC(FloatMatrix &answer, TimeStep *atTime)
{
 
    int nLoads = 0;
    int undofs = this->computeNumberOfDofs(EID_MomentumBalance);
    int pndofs = this->computeNumberOfDofs(EID_ConservationEquation);
    Load *load;
    //bcType loadtype;
    FloatMatrix helpMatrix;

    nLoads = this->giveBodyLoadArray()->giveSize();
    answer.resize(pndofs, undofs);
    answer.zero();
    helpMatrix.resize(pndofs, undofs);
    helpMatrix.zero();
    if ( nLoads ) { 
        bcGeomType ltype;
        for ( int i = 1; i <= nLoads; i++ ) {
            load  = domain->giveLoad( bodyLoadArray.at(i) );
            ltype = load->giveBCGeoType();
            if ( ( ltype == BodyLoadBGT ) && ( load->giveBCValType() == ReinforceBVT ) ) {
                this->computeHomogenizedReinforceTerm_MC(helpMatrix, load, atTime);
            }
            answer.add(helpMatrix);
        }
    }
}


double
SUPGElement :: giveCharacteristicValue(CharType mtrx, TimeStep *tStep)
{
    if ( mtrx == CriticalTimeStep ) {
        return this->computeCriticalTimeStep(tStep);
    } else {
        _error("giveCharacteristicValue: Unknown Type of characteristic mtrx.");
    }

    return 0.0;
}



int
SUPGElement :: checkConsistency()
//
// check internal consistency
// mainly tests, whether material and crossSection data
// are safe for conversion to "Structural" versions
//
{
    int result = 1;
    /*
     * if (!this->giveMaterial()->testMaterialExtension(Material_TransportCapability)) {
     * _warning("checkConsistency : material without support for transport problems");
     * result =0;
     * }
     */
    return result;
}

void
SUPGElement :: updateInternalState(TimeStep *stepN)
{
    IntegrationRule *iRule;
    FloatArray stress;

    // force updating strains & stresses
    for ( int i = 0; i < numberOfIntegrationRules; i++ ) {
        iRule = integrationRulesArray [ i ];
        for ( int j = 0; j < iRule->giveNumberOfIntegrationPoints(); j++ ) {
            computeDeviatoricStress(stress, iRule->getIntegrationPoint(j), stepN);
        }
    }
}

void
SUPGElement :: printOutputAt(FILE *file, TimeStep *stepN)
// Performs end-of-step operations.
{
#ifdef __PARALLEL_MODE
    fprintf( file, "element %d [%8d] :\n", this->giveNumber(), this->giveGlobalNumber() );
#else
    fprintf(file, "element %d :\n", number);
#endif

    for ( int i = 0; i < numberOfIntegrationRules; i++ ) {
        integrationRulesArray [ i ]->printOutputAt(file, stepN);
    }
}


#ifdef __OOFEG
int
SUPGElement :: giveInternalStateAtNode(FloatArray &answer, InternalStateType type, InternalStateMode mode,
                                       int node, TimeStep *atTime)
{
    int indx = 1;
    Node *n = this->giveNode(node);

    if ( type == IST_Velocity ) {
        answer.resize( this->giveSpatialDimension() );
        int dofindx;
        if ( ( dofindx = n->findDofWithDofId(V_u) ) ) {
            answer.at(indx++) = n->giveDof(dofindx)->giveUnknown(VM_Total, atTime);
        } else if ( ( dofindx = n->findDofWithDofId(V_v) ) ) {
            answer.at(indx++) = n->giveDof(dofindx)->giveUnknown(VM_Total, atTime);
        } else if ( ( dofindx = n->findDofWithDofId(V_w) ) ) {
            answer.at(indx++) = n->giveDof(dofindx)->giveUnknown(VM_Total, atTime);
        }

        return 1;
    } else if ( type == IST_Pressure ) {
        int dofindx;
        if ( ( dofindx = n->findDofWithDofId(P_f) ) ) {
            answer.resize(1);
            answer.at(1) = n->giveDof(dofindx)->giveUnknown(VM_Total, atTime);
            return 1;
        } else {
            return 0;
        }
    } else {
        return Element :: giveInternalStateAtNode(answer, type, mode, node, atTime);
    }
}

#endif


#if 0
void
SUPGElement::computeVectorOfPrescribed (EquationID ut, ValueModeType type, TimeStep* stepN, FloatArray& answer)
{
    double scale;
    Element::computeVectorOfPrescribed (ut, type, stepN, answer);
    if (domain->giveEngngModel()->giveEquationScalingFlag()) {
    if (ut == EID_MomentumBalance) {
        scale = domain->giveEngngModel()->giveVariableScale(VST_Velocity);
    } else if (ut == EID_ConservationEquation) {
        scale = domain->giveEngngModel()->giveVariableScale(VST_Pressure);
    } else scale = 1.0;
        answer.times (1.0/scale);
    }
}
#endif
} // end namespace oofem
