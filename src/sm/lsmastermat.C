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

#include "lsmastermat.h"
#include "isolinearelasticmaterial.h"
#include "gausspoint.h"
#include "floatmatrix.h"
#include "floatarray.h"
#include "intarray.h"
#include "stressvector.h"
#include "strainvector.h"
#include "structuralcrosssection.h"
#include "mathfem.h"
#include "contextioerr.h"
#include "datastream.h"
#include "classfactory.h"

namespace oofem {
REGISTER_Material(LargeStrainMasterMaterial);

// constructor
LargeStrainMasterMaterial :: LargeStrainMasterMaterial(int n, Domain *d) : StructuralMaterial(n, d)
{
    slaveMat = 0;
}

// destructor
LargeStrainMasterMaterial :: ~LargeStrainMasterMaterial()
{ }

// specifies whether a given material mode is supported by this model
int
LargeStrainMasterMaterial :: hasMaterialModeCapability(MaterialMode mode)
{
    return mode == _3dMat;
}

// reads the model parameters from the input file
IRResultType
LargeStrainMasterMaterial :: initializeFrom(InputRecord *ir)
{
    const char *__proc = "initializeFrom"; // required by IR_GIVE_FIELD macro
    IRResultType result;                 // required by IR_GIVE_FIELD macro



    IR_GIVE_OPTIONAL_FIELD(ir, slaveMat, _IFT_LargeStrainMasterMaterial_slaveMat); // number of slave material
    IR_GIVE_OPTIONAL_FIELD(ir, m, _IFT_LargeStrainMasterMaterial_m); // type of Set-Hill strain tensor

    return IRRT_OK;
}

// creates a new material status  corresponding to this class
MaterialStatus *
LargeStrainMasterMaterial :: CreateStatus(GaussPoint *gp) const
{
    LargeStrainMasterMaterialStatus *status;
    status = new LargeStrainMasterMaterialStatus(1, this->giveDomain(), gp, slaveMat);
    return status;
}


void
LargeStrainMasterMaterial :: giveFirstPKStressVector_3d(FloatArray &answer, GaussPoint *gp, const FloatArray &vF, TimeStep *tStep)
{
    LargeStrainMasterMaterialStatus *status = static_cast< LargeStrainMasterMaterialStatus * >( this->giveStatus(gp) );
    this->initTempStatus(gp);
    MaterialMode mode = gp->giveMaterialMode();
    if  ( mode == _3dMat ) {
        StructuralMaterial *sMat = dynamic_cast< StructuralMaterial * >( domain->giveMaterial(slaveMat) );
        if ( sMat == NULL ) {
            _warning2("checkConsistency: material %d has no Structural support", slaveMat);
            return;
        }

        double lambda1, lambda2, lambda3, E1, E2, E3;
        FloatArray eVals, SethHillStrainVector, stressVector, stressM;
        FloatMatrix F, Ft, C, eVecs, SethHillStrain, stress(3, 3);
        FloatMatrix L1, L2, T, tT;
        //store of deformation gradient into 3x3 matrix
        F.beMatrixForm(vF);
        //compute right Cauchy-Green tensor(C), its eigenvalues and eigenvectors
        Ft.beTranspositionOf(F);
        C.beProductOf(Ft, F);
        // compute eigen values and eigen vectors of C
        C.jaco_(eVals, eVecs, 40);
        // compute Seth - Hill's strain measure, it depends on mParameter
        lambda1 = eVals.at(1);
        lambda2 = eVals.at(2);
        lambda3 = eVals.at(3);
        if ( m == 0 ) {
            E1 = 1. / 2. * log(lambda1);
            E2 = 1. / 2. * log(lambda2);
            E3 = 1. / 2. * log(lambda3);
        } else   {
            E1 = 1. / ( 2. * m ) * ( pow(lambda1, m) - 1. );
            E2 = 1. / ( 2. * m ) * ( pow(lambda2, m) - 1. );
            E3 = 1. / ( 2. * m ) * ( pow(lambda3, m) - 1. );
        }

        SethHillStrain.resize(3, 3);
        for ( int i = 1; i < 4; i++ ) {
            for ( int j = 1; j < 4; j++ ) {
                SethHillStrain.at(i, j) = E1 * eVecs.at(i, 1) * eVecs.at(j, 1) + E2 *eVecs.at(i, 2) * eVecs.at(j, 2) + E3 *eVecs.at(i, 3) * eVecs.at(j, 3);
            }
        }

        SethHillStrainVector.beSymVectorFormOfStrain(SethHillStrain);
        sMat->giveRealStressVector_3d(stressVector, gp, SethHillStrainVector, tStep);
        this->constructTransformationMatrix(T, eVecs);
        tT.beTranspositionOf(T);

        stressVector.at(4) = 2 * stressVector.at(4);
        stressVector.at(5) = 2 * stressVector.at(5);
        stressVector.at(6) = 2 * stressVector.at(6);


        stressM.beProductOf(T, stressVector);
        stressM.at(4) = 1. / 2. *  stressM.at(4);
        stressM.at(5) = 1. / 2. *  stressM.at(5);
        stressM.at(6) = 1. / 2. *  stressM.at(6);

        this->constructL1L2TransformationMatrices(L1, L2, eVecs, stressM, E1, E2, E3);

        FloatMatrix junk, P, TL;
        FloatArray secondPK;
        junk.beProductOf(L1, T);
        P.beProductOf(tT, junk);
        //transformation of the stress to the 2PK stress and then to 1PK
        stressVector.at(4) = 0.5 * stressVector.at(4);
        stressVector.at(5) = 0.5 * stressVector.at(5);
        stressVector.at(6) = 0.5 * stressVector.at(6);
        secondPK.beProductOf(P, stressVector);
        this->convert_S_2_P( answer, secondPK, vF, gp->giveMaterialMode() );
        junk.zero();
        junk.beProductOf(L2, T);
        TL.beProductOf(tT, junk);
        status->setPmatrix(P);
        status->setTLmatrix(TL);
        status->letTempStressVectorBe(answer);
    } else {
        OOFEM_ERROR("LargeStrainMasterMaterial :: giveFirstPKStressVector - Unknown material mode.");
    }
}



void
LargeStrainMasterMaterial :: constructTransformationMatrix(FloatMatrix &answer, const FloatMatrix &eVecs)
{
    answer.resize(6, 6);
    answer.at(1, 1) = eVecs.at(1, 1) * eVecs.at(1, 1);
    answer.at(1, 2) = eVecs.at(2, 1) * eVecs.at(2, 1);
    answer.at(1, 3) = eVecs.at(3, 1) * eVecs.at(3, 1);
    answer.at(1, 4) = eVecs.at(2, 1) * eVecs.at(3, 1);
    answer.at(1, 5) = eVecs.at(1, 1) * eVecs.at(3, 1);
    answer.at(1, 6) = eVecs.at(1, 1) * eVecs.at(2, 1);

    answer.at(2, 1) = eVecs.at(1, 2) * eVecs.at(1, 2);
    answer.at(2, 2) = eVecs.at(2, 2) * eVecs.at(2, 2);
    answer.at(2, 3) = eVecs.at(3, 2) * eVecs.at(3, 2);
    answer.at(2, 4) = eVecs.at(2, 2) * eVecs.at(3, 2);
    answer.at(2, 5) = eVecs.at(1, 2) * eVecs.at(3, 2);
    answer.at(2, 6) = eVecs.at(1, 2) * eVecs.at(2, 2);

    answer.at(3, 1) = eVecs.at(1, 3) * eVecs.at(1, 3);
    answer.at(3, 2) = eVecs.at(2, 3) * eVecs.at(2, 3);
    answer.at(3, 3) = eVecs.at(3, 3) * eVecs.at(3, 3);
    answer.at(3, 4) = eVecs.at(2, 3) * eVecs.at(3, 3);
    answer.at(3, 5) = eVecs.at(1, 3) * eVecs.at(3, 3);
    answer.at(3, 6) = eVecs.at(1, 3) * eVecs.at(2, 3);

    answer.at(4, 1) = 2 * eVecs.at(1, 2) * eVecs.at(1, 3);
    answer.at(4, 2) = 2 * eVecs.at(2, 2) * eVecs.at(2, 3);
    answer.at(4, 3) = 2 * eVecs.at(3, 2) * eVecs.at(3, 3);
    answer.at(4, 4) = eVecs.at(2, 2) * eVecs.at(3, 3) + eVecs.at(3, 2) * eVecs.at(2, 3);
    answer.at(4, 5) = eVecs.at(1, 2) * eVecs.at(3, 3) + eVecs.at(3, 2) * eVecs.at(1, 3);
    answer.at(4, 6) = eVecs.at(1, 2) * eVecs.at(2, 3) + eVecs.at(2, 2) * eVecs.at(1, 3);

    answer.at(5, 1) = 2 * eVecs.at(1, 1) * eVecs.at(1, 3);
    answer.at(5, 2) = 2 * eVecs.at(2, 1) * eVecs.at(2, 3);
    answer.at(5, 3) = 2 * eVecs.at(3, 1) * eVecs.at(3, 3);
    answer.at(5, 4) = eVecs.at(2, 1) * eVecs.at(3, 3) + eVecs.at(3, 1) * eVecs.at(2, 3);
    answer.at(5, 5) = eVecs.at(1, 1) * eVecs.at(3, 3) + eVecs.at(3, 1) * eVecs.at(1, 3);
    answer.at(5, 6) = eVecs.at(1, 1) * eVecs.at(2, 3) + eVecs.at(2, 1) * eVecs.at(1, 3);

    answer.at(6, 1) = 2 * eVecs.at(1, 1) * eVecs.at(1, 2);
    answer.at(6, 2) = 2 * eVecs.at(2, 1) * eVecs.at(2, 2);
    answer.at(6, 3) = 2 * eVecs.at(3, 1) * eVecs.at(3, 2);
    answer.at(6, 4) = eVecs.at(2, 1) * eVecs.at(3, 2) + eVecs.at(3, 1) * eVecs.at(2, 2);
    answer.at(6, 5) = eVecs.at(1, 1) * eVecs.at(3, 2) + eVecs.at(3, 1) * eVecs.at(1, 2);
    answer.at(6, 6) = eVecs.at(1, 1) * eVecs.at(2, 2) + eVecs.at(2, 1) * eVecs.at(1, 2);
}


void
LargeStrainMasterMaterial :: constructL1L2TransformationMatrices(FloatMatrix &answer1, FloatMatrix &answer2, const FloatMatrix &eigenVectors, FloatArray &stressM, double E1, double E2, double E3)
{
    double gamma12, gamma13, gamma23, gamma112, gamma221, gamma113, gamma331, gamma223, gamma332, gamma;
    double lambda1 = eigenVectors.at(1, 1);
    double lambda2 = eigenVectors.at(2, 2);
    double lambda3 = eigenVectors.at(3, 3);
    double lambda1P =  pow(lambda1, m - 1);
    double lambda2P =  pow(lambda2, m - 1);
    double lambda3P =  pow(lambda3, m - 1);
    if ( ( lambda1 == lambda2 ) && ( lambda2 == lambda3 ) ) {     // three equal eigenvalues
        gamma12 = gamma13 = gamma23  = 1. / 2. * lambda1P;

        answer2.at(1, 1) = 2 * stressM.at(1) * ( m - 1 ) * pow(lambda1, m - 2);
        answer2.at(2, 2) = 2 * stressM.at(2) * ( m - 1 ) * pow(lambda2, m - 2);
        answer2.at(3, 3) = 2 * stressM.at(3) * ( m - 1 ) * pow(lambda3, m - 2);
        answer2.at(4, 4) = 1. / 2. * ( stressM.at(2) + stressM.at(3) ) * ( m - 1 ) * pow(lambda2, m - 2);
        answer2.at(5, 5) = 1. / 2. * ( stressM.at(1) + stressM.at(3) ) * ( m - 1 ) * pow(lambda3, m - 2);
        answer2.at(6, 6) = 1. / 2. * ( stressM.at(1) + stressM.at(2) ) * ( m - 1 ) * pow(lambda1, m - 2);

        answer2.at(1, 5) = answer2.at(5, 1) = stressM.at(5) * ( m - 1 ) * pow(lambda3, m - 2);
        answer2.at(1, 6) = answer2.at(6, 1) = stressM.at(6) * ( m - 1 ) * pow(lambda1, m - 2);
        answer2.at(2, 4) = answer2.at(4, 2) = stressM.at(4) * ( m - 1 ) * pow(lambda2, m - 2);
        answer2.at(2, 6) = answer2.at(6, 2) = stressM.at(6) * ( m - 1 ) * pow(lambda1, m - 2);
        answer2.at(3, 4) = answer2.at(4, 3) = stressM.at(4) * ( m - 1 ) * pow(lambda2, m - 2);
        answer2.at(3, 5) = answer2.at(5, 3) = stressM.at(5) * ( m - 1 ) * pow(lambda3, m - 2);

        answer2.at(4, 5) = answer2.at(5, 4) =  1. / 2. * stressM.at(6) * ( m - 1 ) * pow(lambda1, m - 2);
        answer2.at(4, 6) = answer2.at(6, 4) = 1. / 2. * stressM.at(5) * ( m - 1 ) * pow(lambda1, m - 2);
        ;
        answer2.at(5, 6) = answer2.at(6, 5) = 1. / 2. * stressM.at(4) * ( m - 1 ) * pow(lambda1, m - 2);
        ;
    } else if ( lambda1 == lambda2 ) {     //two equal eigenvalues
        gamma12  = 1. / 2. * lambda1P;
        gamma13 = ( E1 - E3 ) / ( lambda1 - lambda3 );
        gamma23 = ( E2 - E3 ) / ( lambda2 - lambda3 );
        gamma113 = ( pow(lambda1, m - 1) * ( lambda1 - lambda3 ) - 2 * ( E1 - E3 ) ) / ( ( lambda1 - lambda3 ) * ( lambda1 - lambda3 ) );
        gamma331 = ( pow(lambda3, m - 1) * ( lambda3 - lambda1 ) - 2 * ( E3 - E1 ) ) / ( ( lambda3 - lambda1 ) * ( lambda3 - lambda1 ) );


        answer2.at(1, 1) = 2 * stressM.at(1) * ( m - 1 ) * pow(lambda1, m - 2);
        answer2.at(2, 2) = 2 * stressM.at(2) * ( m - 1 ) * pow(lambda2, m - 2);
        answer2.at(3, 3) = 2 * stressM.at(3) * ( m - 1 ) * pow(lambda3, m - 2);

        answer2.at(4, 4) = stressM.at(2) * gamma113 + stressM.at(3) * gamma331;
        answer2.at(5, 5) = stressM.at(1) * gamma113 + stressM.at(3) * gamma331;
        answer2.at(6, 6) = 1. / 2. * ( stressM.at(1) + stressM.at(2) ) * ( m - 1 ) * pow(lambda1, m - 2);

        answer2.at(1, 5) = answer2.at(5, 1) = 2. * stressM.at(5) * gamma113;
        answer2.at(1, 6) = answer2.at(6, 1) = stressM.at(6) * ( m - 1 ) * pow(lambda1, m - 2);
        answer2.at(2, 4) = answer2.at(4, 2) = 2. * stressM.at(4) * gamma113;
        answer2.at(2, 6) = answer2.at(6, 2) = stressM.at(6) * ( m - 1 ) * pow(lambda1, m - 2);
        answer2.at(3, 4) = answer2.at(4, 3) = 2. * stressM.at(4) * gamma331;
        answer2.at(3, 5) = answer2.at(5, 3) = 2. * stressM.at(5) * gamma331;
        answer2.at(4, 5) = answer2.at(5, 4) = stressM.at(6) * gamma113;
        answer2.at(4, 6) = answer2.at(6, 4) = stressM.at(5) * gamma113;
        answer2.at(5, 6) = answer2.at(6, 5) = stressM.at(4) * gamma113;
    } else if ( lambda2 == lambda3 ) {
        gamma23  = 1. / 2. * lambda2P;
        gamma12 = ( E1 - E2 ) / ( lambda1 - lambda2 );
        gamma13 = ( E1 - E3 ) / ( lambda1 - lambda3 );
        gamma112 = ( pow(lambda1, m - 1) * ( lambda1 - lambda2 ) - 2 * ( E1 - E2 ) ) / ( ( lambda1 - lambda2 ) * ( lambda1 - lambda2 ) );
        gamma221 = ( pow(lambda2, m - 1) * ( lambda2 - lambda1 ) - 2 * ( E2 - E1 ) ) / ( ( lambda2 - lambda1 ) * ( lambda2 - lambda1 ) );

        answer2.at(1, 1) = 2 * stressM.at(1) * ( m - 1 ) * pow(lambda1, m - 2);
        answer2.at(2, 2) = 2 * stressM.at(2) * ( m - 1 ) * pow(lambda2, m - 2);
        answer2.at(3, 3) = 2 * stressM.at(3) * ( m - 1 ) * pow(lambda3, m - 2);

        answer2.at(4, 4) = 1. / 2. * ( stressM.at(2) + stressM.at(3) ) * ( m - 1 ) * pow(lambda2, m - 2);
        answer2.at(5, 5) = stressM.at(1) * gamma112 + stressM.at(3) * gamma221;
        answer2.at(6, 6) = stressM.at(1) * gamma112 + stressM.at(2) * gamma221;

        answer2.at(1, 5) = answer2.at(5, 1) = 2. *  stressM.at(5) * gamma112;
        answer2.at(1, 6) = answer2.at(6, 1) = 2. *  stressM.at(6) * gamma112;
        answer2.at(2, 4) = answer2.at(4, 2) = stressM.at(4) * ( m - 1 ) * pow(lambda2, m - 2);
        answer2.at(2, 6) = answer2.at(6, 2) = 2. * stressM.at(6) * gamma221;
        answer2.at(3, 4) = answer2.at(4, 3) = stressM.at(4) * ( m - 1 ) * pow(lambda2, m - 2);
        answer2.at(3, 5) = answer2.at(5, 3) = 2. * stressM.at(5) * gamma221;
        answer2.at(4, 5) = answer2.at(5, 4) = stressM.at(6) * gamma221;
        answer2.at(4, 6) = answer2.at(6, 4) = stressM.at(5) * gamma221;
        answer2.at(5, 6) = answer2.at(6, 5) = stressM.at(4) * gamma221;
    } else if ( lambda1 == lambda3 )   {
        gamma13 = 1. / 2. * lambda1P;
        gamma12 = ( E1 - E2 ) / ( lambda1 - lambda2 );
        gamma23 = ( E2 - E3 ) / ( lambda2 - lambda3 );
        gamma223 = ( pow(lambda2, m - 1) * ( lambda2 - lambda3 ) - 2 * ( E2 - E3 ) ) / ( ( lambda2 - lambda3 ) * ( lambda2 - lambda3 ) );
        gamma332 = ( pow(lambda3, m - 1) * ( lambda3 - lambda2 ) - 2 * ( E3 - E2 ) ) / ( ( lambda3 - lambda2 ) * ( lambda3 - lambda2 ) );

        answer2.at(1, 1) = 2 * stressM.at(1) * ( m - 1 ) * pow(lambda1, m - 2);
        answer2.at(2, 2) = 2 * stressM.at(2) * ( m - 1 ) * pow(lambda2, m - 2);
        answer2.at(3, 3) = 2 * stressM.at(3) * ( m - 1 ) * pow(lambda3, m - 2);

        answer2.at(4, 4) = stressM.at(2) * gamma223 + stressM.at(3) * gamma332;
        answer2.at(5, 5) = 1. / 2. * ( stressM.at(1) + stressM.at(3) ) * ( m - 1 ) * pow(lambda3, m - 2);
        answer2.at(6, 6) = stressM.at(1) * gamma332 + stressM.at(2) * gamma223;

        answer2.at(1, 5) = answer2.at(5, 1) = stressM.at(5) * ( m - 1 ) * pow(lambda3, m - 2);
        answer2.at(1, 6) = answer2.at(6, 1) = 2. * stressM.at(6) * gamma332;
        answer2.at(2, 4) = answer2.at(4, 2) = 2. * stressM.at(4) * gamma223;
        answer2.at(2, 6) = answer2.at(6, 2) = 2. * stressM.at(4) * gamma223;
        answer2.at(3, 4) = answer2.at(4, 3) = 2. * stressM.at(4) * gamma332;
        answer2.at(3, 5) = answer2.at(5, 3) = stressM.at(5) * ( m - 1 ) * pow(lambda3, m - 2);
        answer2.at(4, 5) = answer2.at(5, 4) = stressM.at(6) * gamma332;
        answer2.at(4, 6) = answer2.at(6, 4) = stressM.at(5) * gamma332;
        answer2.at(5, 6) = answer2.at(6, 5) = stressM.at(4) * gamma332;
    } else {             //three different eigenvalues
        gamma12 = ( E1 - E2 ) / ( lambda1 - lambda2 );
        gamma13 = ( E1 - E3 ) / ( lambda1 - lambda3 );
        gamma23 = ( E2 - E3 ) / ( lambda2 - lambda3 );

        gamma112 = ( pow(lambda1, m - 1) * ( lambda1 - lambda2 ) - 2 * ( E1 - E2 ) ) / ( ( lambda1 - lambda2 ) * ( lambda1 - lambda2 ) );
        gamma221 = ( pow(lambda2, m - 1) * ( lambda2 - lambda1 ) - 2 * ( E2 - E1 ) ) / ( ( lambda2 - lambda1 ) * ( lambda2 - lambda1 ) );
        gamma113 = ( pow(lambda1, m - 1) * ( lambda1 - lambda3 ) - 2 * ( E1 - E3 ) ) / ( ( lambda1 - lambda3 ) * ( lambda1 - lambda3 ) );
        gamma331 = ( pow(lambda3, m - 1) * ( lambda3 - lambda1 ) - 2 * ( E3 - E1 ) ) / ( ( lambda3 - lambda1 ) * ( lambda3 - lambda1 ) );
        gamma223 = ( pow(lambda2, m - 1) * ( lambda2 - lambda3 ) - 2 * ( E2 - E3 ) ) / ( ( lambda2 - lambda3 ) * ( lambda2 - lambda3 ) );
        gamma332 = ( pow(lambda3, m - 1) * ( lambda3 - lambda2 ) - 2 * ( E3 - E2 ) ) / ( ( lambda3 - lambda2 ) * ( lambda3 - lambda2 ) );

        gamma = ( lambda1 * ( E2 - E3 ) + lambda2 * ( E3 - E1 ) + lambda3 * ( E1 - E2 ) ) / ( ( lambda1 - lambda2 ) * ( lambda2 - lambda3 ) * ( lambda3 - lambda1 ) );

        answer2.at(1, 1) = 2 * stressM.at(1) * ( m - 1 ) * pow(lambda1, m - 2);
        answer2.at(2, 2) = 2 * stressM.at(2) * ( m - 1 ) * pow(lambda2, m - 2);
        answer2.at(3, 3) = 2 * stressM.at(3) * ( m - 1 ) * pow(lambda3, m - 2);

        answer2.at(4, 4) = stressM.at(2) * gamma223 + stressM.at(3) * gamma332;
        answer2.at(5, 5) = stressM.at(1) * gamma113 + stressM.at(3) * gamma331;
        answer2.at(6, 6) = stressM.at(1) * gamma112 + stressM.at(2) * gamma221;

        answer2.at(1, 5) = answer2.at(5, 1) = 2. * stressM.at(5) * gamma113;
        answer2.at(1, 6) = answer2.at(6, 1) = 2. * stressM.at(6) * gamma112;
        answer2.at(2, 4) = answer2.at(4, 2) = 2. * stressM.at(4) * gamma223;
        answer2.at(2, 6) = answer2.at(6, 2) = 2. * stressM.at(6) * gamma221;
        answer2.at(3, 4) = answer2.at(4, 3) = 2. * stressM.at(4) * gamma332;
        answer2.at(3, 5) = answer2.at(5, 3) = 2. * stressM.at(5) * gamma331;
        answer2.at(4, 5) = answer2.at(5, 4) = 2. * stressM.at(6) * gamma;
        answer2.at(4, 6) = answer2.at(6, 4) = 2. * stressM.at(5) * gamma;
        answer2.at(5, 6) = answer2.at(6, 5) = 2. * stressM.at(4) * gamma;
    }


    answer1.at(1, 1) = lambda1P;

    answer1.at(2, 2) = lambda2P;
    answer1.at(3, 3) = lambda3P;
    answer1.at(4, 4) = gamma23;
    answer1.at(5, 5) = gamma13;
    answer1.at(6, 6) = gamma12;
}

void
LargeStrainMasterMaterial :: give3dMaterialStiffnessMatrix(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *atTime)
{
    LargeStrainMasterMaterialStatus *status = static_cast< LargeStrainMasterMaterialStatus * >( this->giveStatus(gp) );
    Material *mat;
    StructuralMaterial *sMat;
    FloatMatrix stiffness;
    MaterialMode mMode = gp->giveMaterialMode();
    if ( mMode == _3dMat ) {
        Material *mat;
        StructuralMaterial *sMat;
        mat = domain->giveMaterial(slaveMat);
        sMat = dynamic_cast< StructuralMaterial * >(mat);
        if ( sMat == NULL ) {
            _warning2("checkConsistency: material %d has no Structural support", slaveMat);
            return;
        }

        sMat->give3dMaterialStiffnessMatrix(answer, mode, gp, atTime);
    } else {
        mat = domain->giveMaterial(slaveMat);
        sMat = dynamic_cast< StructuralMaterial * >(mat);
        if ( sMat == NULL ) {
            _warning2("checkConsistency: material %d has no Structural support", slaveMat);
            return;
        }

        sMat->give3dMaterialStiffnessMatrix(stiffness, mode, gp, atTime);
        FloatMatrix P, TL, F, junk;
        FloatArray stress;
        stress =   status->giveTempStressVector();
        status->giveTransformationMatrix(F);
        ///////////////////////////////////////////////////////////
        stiffness.at(1, 4) = 2. * stiffness.at(1, 4);
        stiffness.at(4, 1) = 2. * stiffness.at(4, 1);
        stiffness.at(1, 5) = 2. * stiffness.at(1, 5);
        stiffness.at(5, 1) = 2. * stiffness.at(5, 1);
        stiffness.at(1, 6) = 2. * stiffness.at(1, 6);
        stiffness.at(6, 1) = 2. * stiffness.at(6, 1);
        stiffness.at(2, 4) = 2. * stiffness.at(2, 4);
        stiffness.at(4, 2) = 2. * stiffness.at(4, 2);
        stiffness.at(2, 5) = 2. * stiffness.at(2, 5);
        stiffness.at(5, 2) = 2. * stiffness.at(5, 2);
        stiffness.at(2, 6) = 2. * stiffness.at(2, 6);
        stiffness.at(6, 2) = 2. * stiffness.at(6, 2);
        stiffness.at(3, 4) = 2. * stiffness.at(3, 4);
        stiffness.at(4, 3) = 2. * stiffness.at(4, 3);
        stiffness.at(3, 5) = 2. * stiffness.at(3, 5);
        stiffness.at(5, 3) = 2. * stiffness.at(5, 3);
        stiffness.at(3, 6) = 2. * stiffness.at(3, 6);
        stiffness.at(6, 3) = 2. * stiffness.at(6, 3);
        stiffness.at(4, 4) = 4. * stiffness.at(4, 4);
        stiffness.at(4, 5) = 4. * stiffness.at(4, 5);
        stiffness.at(5, 4) = 4. * stiffness.at(5, 4);
        stiffness.at(4, 6) = 4. * stiffness.at(4, 6);
        stiffness.at(6, 4) = 4. * stiffness.at(6, 4);
        stiffness.at(5, 5) = 4. * stiffness.at(5, 5);
        stiffness.at(5, 6) = 4. * stiffness.at(5, 6);
        stiffness.at(6, 5) = 4. * stiffness.at(6, 5);
        stiffness.at(6, 6) = 4. * stiffness.at(6, 6);
        /////////////////////////////////////////////////////////////
        status->givePmatrix(P);
        status->giveTLmatrix(TL);
        junk.resize(6, 6);
        junk.zero();
        junk.beProductOf(stiffness, P);
        stiffness.beProductOf(P, junk);
        stiffness.add(TL);


        FloatArray vF = status->giveTempFVector();
        FloatArray vS = status->giveTempStressVector();
        this->convert_dSdE_2_dPdF( answer, stiffness, vS, vF, gp->giveMaterialMode() );
    }
}


int
LargeStrainMasterMaterial :: giveIPValue(FloatArray &answer, GaussPoint *aGaussPoint, InternalStateType type, TimeStep *atTime)
{
    LargeStrainMasterMaterialStatus *status = static_cast< LargeStrainMasterMaterialStatus * >( this->giveStatus(aGaussPoint) );

    if ( type == IST_StressTensor ) {
        answer = status->giveStressVector();
        return 1;
    } else if ( type == IST_StrainTensor ) {
        answer = status->giveStrainVector();
        return 1;
    } else       {
        Material *mat;
        StructuralMaterial *sMat;
        mat = domain->giveMaterial(slaveMat);
        sMat = dynamic_cast< StructuralMaterial * >(mat);
        if ( sMat == NULL ) {
            _warning2("checkConsistency: material %d has no Structural support", slaveMat);
            return 0;
        }

        int result = sMat->giveIPValue(answer, aGaussPoint, type, atTime);
        return result;
    }
}


InternalStateValueType
LargeStrainMasterMaterial :: giveIPValueType(InternalStateType type)
{
    Material *mat;
    StructuralMaterial *sMat;
    FloatMatrix stiffness;
    mat = domain->giveMaterial(slaveMat);
    sMat = dynamic_cast< StructuralMaterial * >(mat);
    if ( sMat == NULL ) {
        _warning2("checkConsistency: material %d has no Structural support", slaveMat);
    }

    InternalStateValueType result = sMat->giveIPValueType(type);
    return result;
}



//=============================================================================

LargeStrainMasterMaterialStatus :: LargeStrainMasterMaterialStatus(int n, Domain *d, GaussPoint *g, int s) : StructuralMaterialStatus(n, d, g),
    Pmatrix(6,6),
    TLmatrix(6,6),
    transformationMatrix(6,6),
    slaveMat(s)
{
    Pmatrix.beUnitMatrix();
}


LargeStrainMasterMaterialStatus :: ~LargeStrainMasterMaterialStatus()
{ }


void
LargeStrainMasterMaterialStatus :: printOutputAt(FILE *file, TimeStep *tStep)
{
    Material *mat;
    StructuralMaterial *sMat;
    mat = domain->giveMaterial(slaveMat);
    sMat = static_cast< StructuralMaterial * >(mat);
    MaterialStatus *mS = sMat->giveStatus(gp);

    mS->printOutputAt(file, tStep);
    //  StructuralMaterialStatus :: printOutputAt(file, tStep);
}


// initializes temporary variables based on their values at the previous equlibrium state
void LargeStrainMasterMaterialStatus :: initTempStatus()
{
    Material *mat;
    StructuralMaterial *sMat;
    mat = domain->giveMaterial(slaveMat);
    sMat = static_cast< StructuralMaterial * >(mat);
    MaterialStatus *mS = sMat->giveStatus(gp);
    mS->initTempStatus();
    //StructuralMaterialStatus :: initTempStatus();
}


// updates internal variables when equilibrium is reached
void
LargeStrainMasterMaterialStatus :: updateYourself(TimeStep *atTime)
{
    Material *mat;
    StructuralMaterial *sMat;
    mat = domain->giveMaterial(slaveMat);
    sMat = static_cast< StructuralMaterial * >(mat);
    MaterialStatus *mS = sMat->giveStatus(gp);
    mS->updateYourself(atTime);
    //  StructuralMaterialStatus :: updateYourself(atTime);
}


// saves full information stored in this status
// temporary variables are NOT stored
contextIOResultType
LargeStrainMasterMaterialStatus :: saveContext(DataStream *stream, ContextMode mode, void *obj)
{
    contextIOResultType iores;
    Material *mat;
    StructuralMaterial *sMat;
    mat = domain->giveMaterial(slaveMat);
    sMat = dynamic_cast< StructuralMaterial * >(mat);
    MaterialStatus *mS = sMat->giveStatus(gp);
    // save parent class status
    if ( ( iores = mS->saveContext(stream, mode, obj) ) != CIO_OK ) {
        THROW_CIOERR(iores);
    }

    // write raw data

    return CIO_OK;
}


contextIOResultType
LargeStrainMasterMaterialStatus :: restoreContext(DataStream *stream, ContextMode mode, void *obj)
//
// restores full information stored in stream to this Status
//
{
    contextIOResultType iores;

    // read parent class status
    if ( ( iores = StructuralMaterialStatus :: restoreContext(stream, mode, obj) ) != CIO_OK ) {
        THROW_CIOERR(iores);
    }

    return CIO_OK; // return succes
}
} // end namespace oofem
