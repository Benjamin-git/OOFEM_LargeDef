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

#include "idmnl1.h"
#include "gausspoint.h"
#include "floatmatrix.h"
#include "floatarray.h"
#include "structuralcrosssection.h"
#include "mathfem.h"
#include "structuralelement.h"
#include "sparsemtrx.h"
#include "error.h"
#include "nonlocalmaterialext.h"
#include "contextioerr.h"
#include "stressvector.h"
#include "strainvector.h"
#include "classfactory.h"
#include "dynamicinputrecord.h"

#ifdef __PARALLEL_MODE
 #include "combuff.h"
#endif

#ifdef __OOFEG
 #include "oofeggraphiccontext.h"
 #include "connectivitytable.h"
#endif

namespace oofem {

REGISTER_Material( IDNLMaterial );

IDNLMaterial :: IDNLMaterial(int n, Domain *d) : IsotropicDamageMaterial1(n, d), StructuralNonlocalMaterialExtensionInterface(d), NonlocalMaterialStiffnessInterface()
    //
    // constructor
    //
{
    Rf = 0.;
    exponent = 1.;
    averType = 0;
}


IDNLMaterial :: ~IDNLMaterial()
//
// destructor
//
{ }

void
IDNLMaterial :: updateBeforeNonlocAverage(const FloatArray &strainVector, GaussPoint *gp, TimeStep *atTime)
{
    /* Implements the service updating local variables in given integration points,
     * which take part in nonlocal average process. Actually, no update is necessary,
     * because the value used for nonlocal averaging is strain vector used for nonlocal secant stiffness
     * computation. It is therefore necessary only to store local strain in corresponding status.
     * This service is declared at StructuralNonlocalMaterial level.
     */
    FloatArray SDstrainVector, fullSDStrainVector;
    double equivStrain;
    IDNLMaterialStatus *nlstatus = static_cast< IDNLMaterialStatus * >( this->giveStatus(gp) );

    this->initTempStatus(gp);
    this->initGpForNewStep(gp);

    // subtract stress-independent part
    // note: eigenStrains (temperature) is not contained in mechanical strain stored in gp
    // therefore it is necessary to subtract always the total eigenstrain value
    this->giveStressDependentPartOfStrainVector(SDstrainVector, gp, strainVector, atTime, VM_Total);

    // compute and store the local variable to be averaged
    // (typically the local equivalent strain)
    nlstatus->letTempStrainVectorBe(SDstrainVector);
    this->computeLocalEquivalentStrain(equivStrain, SDstrainVector, gp, atTime);

    // nonstandard formulation based on averaging of compliance parameter gamma
    // note: gamma is stored in a variable named localEquivalentStrainForAverage, which can be misleading
    //       perhaps this variable should later be renamed
    if ( averagedVar == AVT_Compliance ) {
        double gamma = complianceFunction(equivStrain, gp);
        nlstatus->setLocalEquivalentStrainForAverage(gamma);
    }
    // standard formulation based on averaging of equivalent strain
    else {
        nlstatus->setLocalEquivalentStrainForAverage(equivStrain);
    }

    // influence of damage on weight function
    if ( averType >= 2 && averType <= 5 ) {
        this->modifyNonlocalWeightFunctionAround(gp);
    }
}

void
IDNLMaterial :: modifyNonlocalWeightFunctionAround(GaussPoint *gp)
{
    IDNLMaterialStatus *nonlocStatus, *status = static_cast< IDNLMaterialStatus * >( this->giveStatus(gp) );
    std::list< localIntegrationRecord > *list = this->giveIPIntegrationList(gp);
    std::list< localIntegrationRecord > :: iterator pos, postarget;

    // find the current Gauss point (target) in the list of it neighbors
    for ( pos = list->begin(); pos != list->end(); ++pos ) {
        if ( pos->nearGp == gp ) {
            postarget = pos;
        }
    }

    Element *elem = gp->giveElement();
    FloatArray coords;
    elem->computeGlobalCoordinates( coords, * ( gp->giveCoordinates() ) );
    double xtarget = coords.at(1);

    double w, wsum = 0., x, xprev, damage, damageprev = 0.;
    Element *nearElem;

    // process the list from the target to the end
    double distance = 0.; // distance modified by damage
    xprev = xtarget;
    for ( pos = postarget; pos != list->end(); ++pos ) {
        nearElem = ( pos->nearGp )->giveElement();
        nearElem->computeGlobalCoordinates( coords, * ( ( pos->nearGp )->giveCoordinates() ) );
        x = coords.at(1);
        nonlocStatus = static_cast< IDNLMaterialStatus * >( this->giveStatus( pos->nearGp ) );
        damage = nonlocStatus->giveTempDamage();
        if ( damage == 0. ) {
            damage = nonlocStatus->giveDamage();
        }

        if ( pos != postarget ) {
            distance += ( x - xprev ) * 0.5 * ( computeDistanceModifier(damage) + computeDistanceModifier(damageprev) );
        }

        w = computeWeightFunction(distance) * nearElem->computeVolumeAround( pos->nearGp );
        pos->weight = w;
        wsum += w;
        xprev = x;
        damageprev = damage;
    }

    // process the list from the target to the beginning
    distance = 0.;
    for ( pos = postarget; pos != list->begin(); --pos ) {
        nearElem = ( pos->nearGp )->giveElement();
        nearElem->computeGlobalCoordinates( coords, * ( ( pos->nearGp )->giveCoordinates() ) );
        x = coords.at(1);
        nonlocStatus = static_cast< IDNLMaterialStatus * >( this->giveStatus( pos->nearGp ) );
        damage = nonlocStatus->giveTempDamage();
        if ( damage == 0. ) {
            damage = nonlocStatus->giveDamage();
        }

        if ( pos != postarget ) {
            distance += ( xprev - x ) * 0.5 * ( computeDistanceModifier(damage) + computeDistanceModifier(damageprev) );
            w = computeWeightFunction(distance) * nearElem->computeVolumeAround( pos->nearGp );
            pos->weight = w;
            wsum += w;
        }

        xprev = x;
        damageprev = damage;
    }

    // the beginning must be treated separately
    pos = list->begin();
    if ( pos != postarget ) {
        nearElem = ( pos->nearGp )->giveElement();
        nearElem->computeGlobalCoordinates( coords, * ( ( pos->nearGp )->giveCoordinates() ) );
        x = coords.at(1);
        nonlocStatus = static_cast< IDNLMaterialStatus * >( this->giveStatus( pos->nearGp ) );
        damage = nonlocStatus->giveTempDamage();
        if ( damage == 0. ) {
            damage = nonlocStatus->giveDamage();
        }

        distance += ( xprev - x ) * 0.5 * ( computeDistanceModifier(damage) + computeDistanceModifier(damageprev) );
        w = computeWeightFunction(distance) * nearElem->computeVolumeAround( pos->nearGp );
        pos->weight = w;
        wsum += w;
    }

    status->setIntegrationScale(wsum);

    // print it
    /*
     * if (fabs(fabs(xtarget)-0.049835)<1e-6 ||
     *  fabs(fabs(xtarget)-0.039934)<1e-6 ||
     *  fabs(fabs(xtarget)-0.030033)<1e-6 ||0.0250825
     *  fabs(fabs(xtarget)-0.020132)<1e-6 ||
     *  fabs(fabs(xtarget)-0.009901)<1e-6 ||
     *  fabs(fabs(xtarget)-0.00)<1e-6)
     */
    /*
     * if (fabs(fabs(xtarget)-0.049835)<1e-6 ||
     *  fabs(fabs(xtarget)-0.030033)<1e-6 ||
     *  fabs(fabs(xtarget)-0.009901)<1e-6 ||
     *  fabs(fabs(xtarget)-0.00)<1e-6)
     * {
     *
     * for ( pos = list->begin(); pos != list->end(); ++pos ) {
     * nearElem = ((*pos).nearGp)->giveElement();
     * nearElem->computeGlobalCoordinates (coords, *(((*pos).nearGp)->giveCoordinates()));
     * x = coords.at(1);
     * w = ((*pos).weight/wsum)/(nearElem->computeVolumeAround((*pos).nearGp));
     * nonlocStatus = static_cast< IDNLMaterialStatus * >( this->giveStatus((*pos).nearGp) );
     * damage = nonlocStatus->giveDamage();
     * printf("%g %g %g\n",x,w,damage);
     * }
     * printf("\n");
     * }
     */
    /*
     * printf("%g %g\n",xtarget,wsum);
     * if (fabs(xtarget-0.049835)<1e-6)
     * exit(0);
     */
}

double
IDNLMaterial :: computeDistanceModifier(double damage)
{
    switch ( averType ) {
    case 2: return 1. / ( Rf / cl + ( 1. - Rf / cl ) * pow(1. - damage, exponent) );

    case 3: if ( damage == 0. ) {
            return 1.;
    } else {
            return 1. / ( 1. - ( 1. - Rf / cl ) * pow(damage, exponent) );
    }

    case 4: return 1. / pow(Rf / cl, damage);

    case 5: return ( 2. * cl ) / ( cl + Rf + ( cl - Rf ) * cos(3.1415926 * damage) );

    default: return 1.;
    }
}

void
IDNLMaterial :: computeAngleAndSigmaRatio(double &angle, double &ratio, GaussPoint *gp, double &flag)
{
    IDNLMaterialStatus *status = static_cast< IDNLMaterialStatus * >( this->giveStatus(gp) );
    MaterialMode matMode;
    matMode = gp->giveMaterialMode();
    if ( ( matMode == _3dMat ) || ( matMode == _1dMat ) ) { //Check if the stress-based approach can be applied
        _error("computeAngleAndSigmaRatio: 3D or 1D realisation for Stress-based averaging not supported");
    }

    //Get the temporary strain vector
    FloatArray strainFloatArray;
    strainFloatArray = status->giveTempStrainVector();
    //Check if strain vector is zero. In this case this function is not going to modify nonlocal radius
    if ( strainFloatArray.computeNorm() == 0 ) {
        flag = 0;
        return;
    }

    //Convert the FloatArray to StrainVector
    StrainVector strain(strainFloatArray, matMode);
    //Compute effective Stress tensor
    StressVector effectiveStress(matMode);
    const double E = this->giveLinearElasticMaterial()->give('E', gp);
    const double nu = this->giveLinearElasticMaterial()->give('n', gp);
    strain.applyElasticStiffness(effectiveStress, E, nu);
    //Compute principal values and eigenvectors of effective stress tensor
    FloatArray principalStress;
    FloatMatrix princDir;
    effectiveStress.computePrincipalValDir(principalStress, princDir);
    //Calculate angle and ratio according to the cases
    //compute angle of the first eigenvector
    if ( princDir.at(1, 1) == 0. ) { //Check if angle = 90 degrees
        angle = 3.141592 / 2;
    } else {
        angle = atan( princDir.at(2, 1) / princDir.at(1, 1) );
    }

    if ( principalStress.at(2) < 0. && principalStress.at(1) < 0. ) { //Check limit case both eigenvalues negative
        angle = 0.; //Set angle equal to 0
        ratio = 1.; //Set ratio equal to 1
    } else if ( principalStress.at(2) < 0. ) { //Check if only one eigenvalue is positive
        ratio = 0.; //Set ratio equal to 0
    } else {
        ratio = principalStress.at(2) / principalStress.at(1); //compute ratio
    }
}

double
IDNLMaterial :: computeStressBasedWeight(double &angle, double &ratio, GaussPoint *gp, GaussPoint *jGp, double weight)
{
    //Compute Distance between source and receiver point
    FloatArray gpCoords, jGpCoords;
    gp->giveElement()->computeGlobalCoordinates( gpCoords, * ( gp->giveCoordinates() ) );
    jGp->giveElement()->computeGlobalCoordinates( jGpCoords, * ( jGp->giveCoordinates() ) );
    FloatArray distance(jGpCoords); // Line End jGP Point
    distance.subtract(gpCoords); // Line Begin gP Point
    if ( distance.computeNorm() == 0 ) { //Check if source and receiver point coincide
        return weight;
    }

    //Compute Rotation matrix Distance by angle
    FloatMatrix rotation(2, 2);
    rotation.at(1, 1) = cos(angle);
    rotation.at(1, 2) = -sin(angle);
    rotation.at(2, 1) = sin(angle);
    rotation.at(2, 2) = cos(angle);
    //Rotate distance vector
    FloatArray distanceRotated(distance);
    distanceRotated.rotatedWith(rotation, 't'); // Operation distanceRotated= rotation^T *distance
    // Compute axis of ellipse and scale/stretch weak axis so that ellipse is converted to circle
    double gamma = this->beta + ( 1. - beta ) * pow(ratio, 2);
    distanceRotated.at(2) = distanceRotated.at(2) / gamma;
    //Get new weight
    double updatedWeight = this->computeWeightFunction( distanceRotated.computeNorm() );
    updatedWeight = updatedWeight * jGp->giveElement()->computeVolumeAround(jGp); //weight * (Volume where the weight is applied)
    return updatedWeight;
}

void
IDNLMaterial :: computeEquivalentStrain(double &kappa, const FloatArray &strain, GaussPoint *gp, TimeStep *atTime)
{
    double nonlocalContribution, nonlocalEquivalentStrain = 0.0;
    IDNLMaterialStatus *nonlocStatus, *status = static_cast< IDNLMaterialStatus * >( this->giveStatus(gp) );

    this->buildNonlocalPointTable(gp);
    this->updateDomainBeforeNonlocAverage(atTime);

    // compute nonlocal equivalent strain
    // or nonlocal compliance variable gamma (depending on averagedVar)

    std::list< localIntegrationRecord > *list = this->giveIPIntegrationList(gp); // !
    std::list< localIntegrationRecord > :: iterator pos;

    double sigmaRatio = 0.; //ratio sigma2/sigma 1used for stress-based averaging
    double eigenVectorAngle = 0.; //angle betwen the first eigenvector and the x-axis used for stress-based averaging
    double updatedIntegrationVolume = 0.; //new integration volume. Sum of all new weights used for stress-based averaging
    //Flag to deactivate stress-based nonlocal averaging for zero stress states.
    // When flag=0 no Stress-based averaging takes place.
    // When flag=1  Stress-based averaging takes place.
    double flag = 1;
    //Check if Stress based averaging is enforced and calculate the angle of the first eigenvector and the sigmaratio
    if ( this->nlvar == NLVT_StressBased ) {
        computeAngleAndSigmaRatio(eigenVectorAngle, sigmaRatio, gp, flag);
    }

    //Loop over all Gauss points which are in gp's integration domain
    for ( pos = list->begin(); pos != list->end(); ++pos ) {
        GaussPoint *neargp = pos->nearGp;
        nonlocStatus = static_cast< IDNLMaterialStatus * >( neargp->giveMaterialStatus( neargp->giveMaterial()->giveNumber() ) );
        nonlocalContribution = nonlocStatus->giveLocalEquivalentStrainForAverage();
        if ( this->nlvar == NLVT_StressBased && flag == 1 ) { //Check if Stress Based Averaging is requested and calculate nonlocal contribution
            double stressBasedWeight = computeStressBasedWeight(eigenVectorAngle, sigmaRatio, gp, neargp, pos->weight); //Compute new weight
            updatedIntegrationVolume +=  stressBasedWeight;
            nonlocalContribution *= stressBasedWeight;
        } else {
            nonlocalContribution *= pos->weight;
        }

        nonlocalEquivalentStrain += nonlocalContribution;
    }

    if ( this->nlvar == NLVT_StressBased && flag == 1 ) { // Nonlocal weights are modified in stress-based averaging. Thus the integration volume needs to be modified
        status->setIntegrationScale(updatedIntegrationVolume);
    }

    if ( scaling == ST_Standard ) { // standard rescaling
        nonlocalEquivalentStrain *= 1. / status->giveIntegrationScale();
    } else if ( scaling == ST_Borino ) { // Borino modification
        double scale = status->giveIntegrationScale();
        if ( scale > 1. ) {
            nonlocalEquivalentStrain *= 1. / scale;
        } else {
            nonlocalEquivalentStrain += ( 1. - scale ) * status->giveLocalEquivalentStrainForAverage();
        }
    }


    // undernonlocal or overnonlocal formulation
    if ( mm != 1. ) {
        double localEquivalentStrain = status->giveLocalEquivalentStrainForAverage();
        if ( mm >= 0. ) { // harmonic averaging
            if ( localEquivalentStrain > 0. && nonlocalEquivalentStrain > 0. ) {
                nonlocalEquivalentStrain = 1. / ( mm / nonlocalEquivalentStrain + ( 1. - mm ) / localEquivalentStrain );
            } else {
                nonlocalEquivalentStrain = 0.;
            }
        } else {   // arithmetic averaging, -mm is used instead of mm
            nonlocalEquivalentStrain = -mm * nonlocalEquivalentStrain + ( 1. + mm ) * localEquivalentStrain;
        }
    }

    this->endIPNonlocalAverage(gp);  // ???????????????????

    kappa = nonlocalEquivalentStrain;
}

Interface *
IDNLMaterial :: giveInterface(InterfaceType type)
{
    if ( type == NonlocalMaterialExtensionInterfaceType ) {
        return static_cast< StructuralNonlocalMaterialExtensionInterface * >( this );
    } else if ( type == NonlocalMaterialStiffnessInterfaceType ) {
        return static_cast< NonlocalMaterialStiffnessInterface * >( this );
    } else if ( type == MaterialModelMapperInterfaceType ) {
        return static_cast< MaterialModelMapperInterface * >( this );
    } else {
        return NULL;
    }
}


IRResultType
IDNLMaterial :: initializeFrom(InputRecord *ir)
{
    const char *__proc = "initializeFrom"; // Required by IR_GIVE_FIELD macro
    IRResultType result;                // Required by IR_GIVE_FIELD macro

    IsotropicDamageMaterial1 :: initializeFrom(ir);
    StructuralNonlocalMaterialExtensionInterface :: initializeFrom(ir);

    averType = 0;
    IR_GIVE_OPTIONAL_FIELD(ir, averType, _IFT_IDNLMaterial_averagingtype);
    if ( averType == 2 ) {
        exponent = 0.5; // default value for averaging type 2
    }

    if ( averType == 3 ) {
        exponent = 1.; // default value for averaging type 3
    }

    if ( averType == 2 || averType == 3 ) {
        IR_GIVE_OPTIONAL_FIELD(ir, exponent, _IFT_IDNLMaterial_exp);
    }

    if ( averType >= 2 && averType <= 5 ) {
        IR_GIVE_OPTIONAL_FIELD(ir, Rf, _IFT_IDNLMaterial_rf);
    }

    return IRRT_OK;
}


void
IDNLMaterial :: giveInputRecord(DynamicInputRecord &input)
{
    IsotropicDamageMaterial1 :: giveInputRecord(input);
    StructuralNonlocalMaterialExtensionInterface :: giveInputRecord(input);

    input.setField(this->averType, _IFT_IDNLMaterial_averagingtype);

    if ( averType == 2 || averType == 3 ) {
        input.setField(this->exponent, _IFT_IDNLMaterial_exp);
    }
    if ( averType >= 2 && averType <= 5 ) {
        input.setField(this->Rf, _IFT_IDNLMaterial_rf);
    }
}


/*
 * // old implementation - now implemented in local model
 * void
 * IDNLMaterial :: computeDamageParam(double &omega, double kappa, const FloatArray &strain, GaussPoint *gp)
 * {
 * const double e0 = this->give(e0_ID, gp);
 * if ( kappa <= e0 ) {
 *  omega = 0.0;
 *  return;
 * }
 *
 * double ef;
 * switch(this->softType){
 * case ST_Linear:
 *  ef = this->give(ef_ID, gp);
 *  if ( kappa >= ef ) {
 *    omega = 1.0;
 *  } else {
 *    omega = (ef/kappa) * (kappa-e0) / (ef-e0);
 *  }
 *  return;
 * case ST_Exponential:
 *  ef = this->give(ef_ID, gp);
 *  omega = 1.0 - ( e0 / kappa ) * exp( -( kappa - e0 ) / ( ef - e0 ) );
 *  return;
 * case ST_Mazars:
 *  //At = this->give(At_ID, gp);
 *  //Bt = this->give(Bt_ID, gp);
 *  omega = 1.0 - (1.0-At)*e0/kappa - At*exp(-Bt*(kappa-e0));
 *  return;
 * default:
 *  omega = 0.0;
 *  return;
 * }
 * }
 */

void
IDNLMaterial :: computeDamageParam(double &omega, double kappa, const FloatArray &strain, GaussPoint *g)
{
    if ( averagedVar == AVT_Compliance ) {
        // formulation based on nonlocal gamma (here formally called kappa)
        omega = kappa / ( 1. + kappa );
    } else {
        // formulation based on nonlocal equivalent strain
        omega = damageFunction(kappa, g);
    }
}

void
IDNLMaterial :: NonlocalMaterialStiffnessInterface_addIPContribution(SparseMtrx &dest, const UnknownNumberingScheme &s,
                                                                     GaussPoint *gp, TimeStep *atTime)
{
    double coeff;
    IDNLMaterialStatus *status = static_cast< IDNLMaterialStatus * >( this->giveStatus(gp) );
    std::list< localIntegrationRecord > *list = status->giveIntegrationDomainList();
    std::list< localIntegrationRecord > :: iterator pos;
    IDNLMaterial *rmat;
    FloatArray rcontrib, lcontrib;
    IntArray loc, rloc;

    FloatMatrix contrib;

    if ( this->giveLocalNonlocalStiffnessContribution(gp, loc, s, lcontrib, atTime) == 0 ) {
        return;
    }

    for ( pos = list->begin(); pos != list->end(); ++pos ) {
        rmat = dynamic_cast< IDNLMaterial * > ( pos->nearGp->giveMaterial() );
        if ( rmat ) {
            rmat->giveRemoteNonlocalStiffnessContribution( pos->nearGp, rloc, s, rcontrib, atTime );
            coeff = gp->giveElement()->computeVolumeAround(gp) * pos->weight / status->giveIntegrationScale();
            //   printf ("\nelement %d:", gp->giveElement()->giveNumber());
            //   lcontrib.printYourself();
            //   rcontrib.printYourself();
            // assemble the contribution
            // dest.checkSizeTowards (loc, rloc);
            // dest.assemble (lcontrib,loc, rcontrib,rloc);

            /* local effective assembly
             * int i,j, r, c;
             * for (i=1; i<= loc.giveSize(); i++)
             *  for (j=1; j<=rloc.giveSize(); j++) {
             *   r = loc.at(i);
             *   c = rloc.at(j);
             *   if ((r != 0) && (c!=0)) dest.at(r,c) -= (double) (lcontrib.at(i)*rcontrib.at(j)*coeff);
             *  }
             */
            int dim1 = loc.giveSize(), dim2 = rloc.giveSize();
            contrib.resize(dim1, dim2);
            for ( int i = 1; i <= dim1; i++ ) {
                for ( int j = 1; j <= dim2; j++ ) {
                    contrib.at(i, j) = -1.0 * lcontrib.at(i) * rcontrib.at(j) * coeff;
                }
            }

            dest.assemble(loc, rloc, contrib);
        }
    }
}

std::list< localIntegrationRecord > *
IDNLMaterial :: NonlocalMaterialStiffnessInterface_giveIntegrationDomainList(GaussPoint *gp)
{
    IDNLMaterialStatus *status = static_cast< IDNLMaterialStatus * >( this->giveStatus(gp) );
    this->buildNonlocalPointTable(gp);
    return status->giveIntegrationDomainList();
}


#ifdef __OOFEG
void
IDNLMaterial :: NonlocalMaterialStiffnessInterface_showSparseMtrxStructure(GaussPoint *gp, oofegGraphicContext &gc, TimeStep *atTime)
{
    IntArray loc, rloc;
    FloatArray strain;
    double f, equivStrain;
    IDNLMaterialStatus *status = static_cast< IDNLMaterialStatus * >( this->giveStatus(gp) );
    IDNLMaterial *rmat;

    const double e0 = this->give(e0_ID, gp);
    //const double ef = this->give(ef_ID, gp);

    strain = status->giveTempStrainVector();
    // compute equivalent strain
    this->computeEquivalentStrain(equivStrain, strain, gp, atTime);
    f = equivStrain - status->giveTempKappa();

    if ( ( equivStrain <= e0 ) || ( f < 0.0 ) ) {
        return;
    }

    EASValsSetLineWidth(OOFEG_SPARSE_PROFILE_WIDTH);
    EASValsSetColor( gc.getExtendedSparseProfileColor() );
    EASValsSetLayer(OOFEG_SPARSE_PROFILE_LAYER);
    EASValsSetFillStyle(FILL_SOLID);

    WCRec p [ 4 ];
    GraphicObj *go;

    gp->giveElement()->giveLocationArray( loc, EID_MomentumBalance, EModelDefaultEquationNumbering() );

    int n, m;
    std::list< localIntegrationRecord > *list = status->giveIntegrationDomainList();
    std::list< localIntegrationRecord > :: iterator pos;
    for ( pos = list->begin(); pos != list->end(); ++pos ) {
        rmat = dynamic_cast< IDNLMaterial *>( pos->nearGp->giveMaterial() );
        if ( rmat ) {
            ( pos->nearGp )->giveElement()->giveLocationArray( rloc, EID_MomentumBalance, EModelDefaultEquationNumbering() );
        } else {
            continue;
        }

        n = loc.giveSize();
        m = rloc.giveSize();
        for ( int i = 1; i <= n; i++ ) {
            if ( loc.at(i) == 0 ) {
                continue;
            }

            for ( int j = 1; j <= m; j++ ) {
                if ( rloc.at(j) == 0 ) {
                    continue;
                }

                if ( gc.getSparseProfileMode() == 0 ) {
                    p [ 0 ].x = ( FPNum ) loc.at(i) - 0.5;
                    p [ 0 ].y = ( FPNum ) rloc.at(j) - 0.5;
                    p [ 0 ].z = 0.;
                    p [ 1 ].x = ( FPNum ) loc.at(i) + 0.5;
                    p [ 1 ].y = ( FPNum ) rloc.at(j) - 0.5;
                    p [ 1 ].z = 0.;
                    p [ 2 ].x = ( FPNum ) loc.at(i) + 0.5;
                    p [ 2 ].y = ( FPNum ) rloc.at(j) + 0.5;
                    p [ 2 ].z = 0.;
                    p [ 3 ].x = ( FPNum ) loc.at(i) - 0.5;
                    p [ 3 ].y = ( FPNum ) rloc.at(j) + 0.5;
                    p [ 3 ].z = 0.;
                    go =  CreateQuad3D(p);
                    EGWithMaskChangeAttributes(WIDTH_MASK | FILL_MASK | COLOR_MASK | LAYER_MASK, go);
                    EMAddGraphicsToModel(ESIModel(), go);
                } else {
                    p [ 0 ].x = ( FPNum ) loc.at(i);
                    p [ 0 ].y = ( FPNum ) rloc.at(j);
                    p [ 0 ].z = 0.;

                    EASValsSetMType(SQUARE_MARKER);
                    go = CreateMarker3D(p);
                    EGWithMaskChangeAttributes(COLOR_MASK | LAYER_MASK | VECMTYPE_MASK, go);
                    EMAddGraphicsToModel(ESIModel(), go);
                }
            }
        }
    }
}
#endif




int
IDNLMaterial :: giveLocalNonlocalStiffnessContribution(GaussPoint *gp, IntArray &loc, const UnknownNumberingScheme &s,
                                                       FloatArray &lcontrib, TimeStep *atTime)
{
    int nrows, nsize;
    double sum, f, equivStrain;
    IDNLMaterialStatus *status = static_cast< IDNLMaterialStatus * >( this->giveStatus(gp) );
    StructuralElement *elem = static_cast< StructuralElement * >( gp->giveElement() );
    FloatMatrix b, de;
    FloatArray stress, strain;

    const double e0 = this->give(e0_ID, gp);
    const double ef = this->give(ef_ID, gp);

    /*
     * if (fabs(status->giveTempDamage()) <= 1.e-10) {
     * // already eleastic regime
     * loc.resize(0);
     * return 0;
     * }
     */
    strain = status->giveTempStrainVector();

    // compute equivalent strain
    this->computeEquivalentStrain(equivStrain, strain, gp, atTime);
    f = equivStrain - status->giveTempKappa();

    if ( ( equivStrain <= e0 ) || ( f < 0.0 ) ) {
        /*
         * if (status->lst == IDNLMaterialStatus::LST_elastic)
         * printf (" ");
         * else printf ("_");
         * status->lst = IDNLMaterialStatus::LST_elastic;
         */
        loc.resize(0);
        return 0;
    } else {
        if ( status->giveDamage() >= 1.00 ) {
            //   printf ("f");
            return 0;
        }

        /*
         * if (status->lst == IDNLMaterialStatus::LST_loading)
         * printf ("o");
         * else printf ("O");
         * status->lst = IDNLMaterialStatus::LST_loading;
         */

        // no support for reduced integration now
        elem->computeBmatrixAt(gp, b);

        LinearElasticMaterial *lmat = this->giveLinearElasticMaterial();
        lmat->giveStiffnessMatrix(de, SecantStiffness, gp, atTime);
        stress.beProductOf(de, strain);

        f = ( e0 / ( equivStrain * equivStrain ) ) * exp( -( equivStrain - e0 ) / ( ef - e0 ) )
            + ( e0 / equivStrain ) * exp( -( equivStrain - e0 ) / ( ef - e0 ) ) * 1.0 / ( ef - e0 );

        nrows = b.giveNumberOfColumns();
        nsize = stress.giveSize();
        lcontrib.resize(nrows);
        for ( int i = 1; i <= nrows; i++ ) {
            sum = 0.0;
            for ( int j = 1; j <= nsize; j++ ) {
                sum += b.at(j, i) * stress.at(j);
            }

            lcontrib.at(i) = sum * f;
        }
    }

    // request element code numbers
    elem->giveLocationArray(loc, EID_MomentumBalance, s);

    return 1;
}


void
IDNLMaterial :: giveRemoteNonlocalStiffnessContribution(GaussPoint *gp, IntArray &rloc, const UnknownNumberingScheme &s,
                                                        FloatArray &rcontrib, TimeStep *atTime)
{
    int ncols, nsize;
    double coeff = 0.0, sum;
    IDNLMaterialStatus *status = static_cast< IDNLMaterialStatus * >( this->giveStatus(gp) );
    StructuralElement *elem = static_cast< StructuralElement * >( gp->giveElement() );
    FloatMatrix b, de, den, princDir(3, 3), t;
    FloatArray stress, fullStress, strain, principalStress, help, nu;

    elem->giveLocationArray(rloc, EID_MomentumBalance, s);
    // no support for reduced integration now
    elem->computeBmatrixAt(gp, b);

    if ( this->equivStrainType == EST_Rankine_Standard ) {
        FloatArray fullHelp, fullNu;
        LinearElasticMaterial *lmat = this->giveLinearElasticMaterial();

        lmat->giveStiffnessMatrix(de, SecantStiffness, gp, atTime);
        strain = status->giveTempStrainVector();
        stress.beProductOf(de, strain);
        StructuralMaterial :: giveFullSymVectorForm(fullStress, stress, gp->giveMaterialMode());
        if ( gp->giveMaterialMode() == _1dMat ) {
            principalStress = fullStress;
        } else {
            this->computePrincipalValDir(principalStress, princDir, fullStress, principal_stress);
            if ( ( gp->giveMaterialMode() == _3dMat ) || ( gp->giveMaterialMode() == _PlaneStrain ) ) {
                ;
            } else if ( gp->giveMaterialMode() == _PlaneStress ) {
                // force the out-of-plane princ. dir to be last one
                int indx = 0, zeroFlag = 1;
                double swap;
                for ( int i = 1; i <= 3; i++ ) {
                    if ( fabs( principalStress.at(i) ) > 1.e-10 ) {
                        zeroFlag = 0;
                    }

                    if ( princDir.at(3, i) > 0.90 ) {
                        indx = i;
                    }
                }

                if ( indx ) {
                    for ( int i = 1; i <= 3; i++ ) {
                        swap = princDir.at(i, indx);
                        princDir.at(i, indx) = princDir.at(i, 3);
                        princDir.at(i, 3) = swap;
                    }

                    swap = principalStress.at(indx);
                    principalStress.at(indx) = principalStress.at(3);
                    principalStress.at(3) = swap;
                } else if ( zeroFlag == 0 ) {
                    _error("giveRemoteNonlocalStiffnessContribution: internal error");
                }
            } else {
                _error("giveRemoteNonlocalStiffnessContribution: equivStrainType not supported");
            }
        }

        sum = 0.0;
        for ( int i = 1; i <= 3; i++ ) {
            if ( principalStress.at(i) > 0.0 ) {
                sum += principalStress.at(i) * principalStress.at(i);
            }
        }

        if ( sum > 1.e-15 ) {
            coeff = 1. / ( lmat->give('E', gp) * sqrt(sum) );
        } else {
            coeff = 0.0;
        }

        //
        if ( gp->giveMaterialMode() != _1dMat ) {
            this->giveStressVectorTranformationMtrx(t, princDir, 0);
        }

        //
        //  if (gp->giveMaterialMode() != _1dMat) this->giveStressVectorTranformationMtrx (t, princDir,1);

        // extract positive part
        for ( int i = 1; i <= principalStress.giveSize(); i++ ) {
            principalStress.at(i) = max(principalStress.at(i), 0.0);
        }

#if 0
        this->giveNormalElasticStiffnessMatrix(den, SecantStiffness, gp, atTime);
        help.beProductOf(den, principalStress);
        fullHelp.resize(6);
        for ( i = 1; i <= 3; i++ ) {
            fullHelp.at(i) = help.at(i);
        }

        if ( gp->giveMaterialMode() != _1dMat ) {
            fullNu.beProductOf(t, fullHelp);
            //fullNu.beTProductOf (t, fullHelp);
            crossSection->giveReducedCharacteristicVector(nu, gp, fullNu);
        } else {
            nu = help;
        }

#endif

        /* Plane stress optimized version
         *
         *
         * help.resize (3); help.zero();
         * for (i=1; i<=3; i++) {
         * help.at(1) += t.at(i,1)*principalStress.at(i);
         * help.at(2) += t.at(i,2)*principalStress.at(i);
         * help.at(3) += t.at(i,6)*principalStress.at(i);
         * }
         */
        FloatArray fullPrincStress(6);
        fullPrincStress.zero();
        for ( int i = 1; i <= 3; i++ ) {
            fullPrincStress.at(i) = principalStress.at(i);
        }

        fullHelp.beTProductOf(t, fullPrincStress);
        StructuralMaterial :: giveReducedSymVectorForm(help, fullHelp, gp->giveMaterialMode());

        nu.beProductOf(de, help);
    } else if ( this->equivStrainType == EST_ElasticEnergy ) {
        double equivStrain;

        LinearElasticMaterial *lmat = this->giveLinearElasticMaterial();
        lmat->giveStiffnessMatrix(de, SecantStiffness, gp, atTime);
        strain = status->giveTempStrainVector();
        stress.beProductOf(de, strain);
        this->computeLocalEquivalentStrain(equivStrain, strain, gp, atTime);

        nu = stress;
        coeff = 1.0 / ( lmat->give('E', gp) * equivStrain );
    } else {
        _error("giveRemoteNonlocalStiffnessContribution: equivStrainType not supported");
    }


    ncols = b.giveNumberOfColumns();
    nsize = nu.giveSize();
    rcontrib.resize(ncols);
    for ( int i = 1; i <= ncols; i++ ) {
        sum = 0.0;
        for ( int j = 1; j <= nsize; j++ ) {
            sum += nu.at(j) * b.at(j, i);
        }

        rcontrib.at(i) = sum * coeff;
    }
}


void
IDNLMaterial :: giveNormalElasticStiffnessMatrix(FloatMatrix &answer,
                                                 MatResponseMode rMode,
                                                 GaussPoint *gp, TimeStep *atTime)
{
    //
    // return Elastic Stiffness matrix for normal Stresses
    LinearElasticMaterial *lMat = this->giveLinearElasticMaterial();
    FloatMatrix deRed, de;

    lMat->give3dMaterialStiffnessMatrix(de, rMode, gp, atTime);
    // This isn't used? Do we need one with zeroed entries (below) or the general 3d stiffness (above)?
    //lMat->giveCharacteristicMatrix(de, rMode, gp, atTime);
    //StructuralMaterial :: giveFullSymMatrixForm( de, deRed, gp->giveMaterialMode());

    answer.resize(3, 3);
    // copy first 3x3 submatrix to answer
    for ( int i = 1; i <= 3; i++ ) {
        for ( int j = 1; j <= 3; j++ ) {
            answer.at(i, j) = de.at(i, j);
        }
    }
}


IDNLMaterialStatus :: IDNLMaterialStatus(int n, Domain *d, GaussPoint *g) :
    IsotropicDamageMaterial1Status(n, d, g), StructuralNonlocalMaterialStatusExtensionInterface()
{
    localEquivalentStrainForAverage = 0.0;
}


IDNLMaterialStatus :: ~IDNLMaterialStatus()
{ }


void
IDNLMaterialStatus :: printOutputAt(FILE *file, TimeStep *tStep)
{
    StructuralMaterialStatus :: printOutputAt(file, tStep);
    fprintf(file, "status { ");
    if ( this->damage > 0.0 ) {
        fprintf(file, "nonloc-kappa %f, damage %f ", this->kappa, this->damage);

#ifdef keep_track_of_dissipated_energy
        fprintf(file, ", dissW %f, freeE %f, stressW %f ", this->dissWork, ( this->stressWork ) - ( this->dissWork ), this->stressWork);
    } else {
        fprintf(file, "stressW %f ", this->stressWork);
#endif
    }

    fprintf(file, "}\n");
}

void
IDNLMaterialStatus :: initTempStatus()
//
// initializes temp variables according to variables form previous equilibrium state.
// builds new crackMap
//
{
    IsotropicDamageMaterial1Status :: initTempStatus();
}



void
IDNLMaterialStatus :: updateYourself(TimeStep *atTime)
//
// updates variables (nonTemp variables describing situation at previous equilibrium state)
// after a new equilibrium state has been reached
// temporary variables are having values corresponding to newly reched equilibrium.
//
{
    IsotropicDamageMaterial1Status :: updateYourself(atTime);
}



contextIOResultType
IDNLMaterialStatus :: saveContext(DataStream *stream, ContextMode mode, void *obj)
//
// saves full information stored in this Status
// no temp variables stored
//
{
    contextIOResultType iores;
    // save parent class status
    if ( ( iores = IsotropicDamageMaterial1Status :: saveContext(stream, mode, obj) ) != CIO_OK ) {
        THROW_CIOERR(iores);
    }

    //if (!stream->write(&localEquivalentStrainForAverage,1)) THROW_CIOERR(CIO_IOERR);
    return CIO_OK;
}

contextIOResultType
IDNLMaterialStatus :: restoreContext(DataStream *stream, ContextMode mode, void *obj)
//
// restores full information stored in stream to this Status
//
{
    contextIOResultType iores;
    // read parent class status
    if ( ( iores = IsotropicDamageMaterial1Status :: restoreContext(stream, mode, obj) ) != CIO_OK ) {
        THROW_CIOERR(iores);
    }

    // read raw data
    //if (!stream->read (&localEquivalentStrainForAverage,1)) THROW_CIOERR(CIO_IOERR);

    return CIO_OK;
}

Interface *
IDNLMaterialStatus :: giveInterface(InterfaceType type)
{
    if ( type == NonlocalMaterialStatusExtensionInterfaceType ) {
        return static_cast< StructuralNonlocalMaterialStatusExtensionInterface * >( this );
    } else {
        return IsotropicDamageMaterial1Status :: giveInterface(type);
    }
}



#ifdef __PARALLEL_MODE
int
IDNLMaterial :: packUnknowns(CommunicationBuffer &buff, TimeStep *stepN, GaussPoint *ip)
{
    IDNLMaterialStatus *status = static_cast< IDNLMaterialStatus * >( this->giveStatus(ip) );

    this->buildNonlocalPointTable(ip);
    this->updateDomainBeforeNonlocAverage(stepN);

    return buff.packDouble( status->giveLocalEquivalentStrainForAverage() );
}

int
IDNLMaterial :: unpackAndUpdateUnknowns(CommunicationBuffer &buff, TimeStep *stepN, GaussPoint *ip)
{
    int result;
    IDNLMaterialStatus *status = static_cast< IDNLMaterialStatus * >( this->giveStatus(ip) );
    double localEquivalentStrainForAverage;

    result = buff.unpackDouble(localEquivalentStrainForAverage);
    status->setLocalEquivalentStrainForAverage(localEquivalentStrainForAverage);
    return result;
}

int
IDNLMaterial :: estimatePackSize(CommunicationBuffer &buff, GaussPoint *ip)
{
    //
    // Note: status localStrainVectorForAverage memeber must be properly sized!
    //

    //IDNLMaterialStatus *status = (IDNLMaterialStatus*) this -> giveStatus (ip);

    return buff.givePackSize(MPI_DOUBLE, 1);
}

double
IDNLMaterial :: predictRelativeComputationalCost(GaussPoint *gp)
{
    //
    // The values returned come from measurement
    // do not change them unless you know what are you doing
    //
    double cost = 1.2;


    if ( gp->giveMaterialMode() == _3dMat ) {
        cost = 1.5;
    }

    IDNLMaterialStatus *status = static_cast< IDNLMaterialStatus * >( this->giveStatus(gp) );
    int size = status->giveIntegrationDomainList()->size();
    // just a guess (size/10) found optimal
    // cost *= (1.0 + (size/10)*0.5);
    cost *= ( 1.0 + size / 15.0 );

    return cost;
}

#endif
} // end namespace oofem
