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

#ifndef mixedgradientpressurecneumann_h
#define mixedgradientpressurecneumann_h

#include "mixedgradientpressurebc.h"
#include "boundarycondition.h"
#include "dof.h"
#include "bctype.h"
#include "valuemodetype.h"
#include "floatarray.h"
#include "floatmatrix.h"

#define _IFT_MixedGradientPressureWeakPeriodic_Name "mixedgradientpressureweakperiodic"
#define _IFT_MixedGradientPressureWeakPeriodic_order "order" ///< Order of global polynomial in the unknown tractions. Should be at least 1.

namespace oofem {
class MasterDof;
class Node;
class IntegrationRule;
class SparseMtrx;
class SparseLinearSystemNM;

/**
 * Applies a mean deviatoric shear rate and pressure (Neumann boundary condition) in a weakly periodic way.
 * 
 * @author Mikael Öhman
 */
class MixedGradientPressureWeakPeriodic : public MixedGradientPressureBC
{
protected:
    /// Prescribed gradient @f$ d_{\mathrm{dev},ij} @f$.
    FloatMatrix devGradient;
    /**
     * The volumetric part of what was sent in (needed to return the difference).
     * If caller takes care and sends in a deviatoric gradient, then this will be zero and the return value for the volumetric part will be the true volumetric change.
     */
    double volGradient;

    /// Prescribed pressure.
    double pressure;

    /// Order if polynomials
    int order;

    /// DOF-manager containing the unknown volumetric gradient (always exactly one dof).
    Node *voldman;
    /// DOF-manager containing the unknown tractions (Lagrange mult. for micro-periodic velocity)
    Node *tractionsdman;

public:
    /**
     * Creates boundary condition with given number, belonging to given domain.
     * @param n Boundary condition number.
     * @param d Domain to which new object will belongs.
     */
    MixedGradientPressureWeakPeriodic(int n, Domain *d);

    /// Destructor
    virtual ~MixedGradientPressureWeakPeriodic();

    /**
     * Returns the number of internal DOF managers (=2).
     * This boundary condition stores its own DOF managers, one for tractions and one for @f$ d_{\mathrm{vol}} @f$ which is a single DOF for the volumetric gradient.
     */
    virtual int giveNumberOfInternalDofManagers();
    /**
     * Returns the volumetric DOF manager for i == 1, and the deviatoric manager for i == 2.
     */
    virtual DofManager *giveInternalDofManager(int i);

    /// Not relevant for this boundary condition.
    virtual bcType giveType() const { return UnknownBT; }

    /**
     * Initializes receiver according to object description stored in input record.
     * The input record contains two fields;
     * - devGradient \#columns { d_11 d_22 ... d_21 ... } (required)
     * - pressure p (required)
     * The gradient should be in Voigt notation (only the deviatoric part will be used)
     */
    virtual IRResultType initializeFrom(InputRecord *ir);
    virtual void giveInputRecord(DynamicInputRecord &input);

    virtual void scale(double s);

    virtual void computeFields(FloatArray &sigmaDev, double &vol, EquationID eid, TimeStep *tStep);
    virtual void computeTangents(FloatMatrix &Ed, FloatArray &Ep, FloatArray &Cd, double &Cp, EquationID eid, TimeStep *tStep);

    virtual void setPrescribedPressure(double p) { pressure = p; }
    virtual void setPrescribedDeviatoricGradientFromVoigt(const FloatArray &ddev);

    virtual void assembleVector(FloatArray &answer, TimeStep *tStep, EquationID eid,
                                  CharType type, ValueModeType mode,
                                  const UnknownNumberingScheme &s, FloatArray *eNorm = NULL);
    
    virtual void assemble(SparseMtrx *answer, TimeStep *tStep, EquationID eid,
                          CharType type, const UnknownNumberingScheme &r_s, const UnknownNumberingScheme &c_s);
    
    virtual void giveLocationArrays(std::vector<IntArray> &rows, std::vector<IntArray> &cols, EquationID eid, CharType type,
                                    const UnknownNumberingScheme &r_s, const UnknownNumberingScheme &c_s);

    virtual const char *giveClassName() const { return "MixedGradientPressureWeakPeriodic"; }
    virtual const char *giveInputRecordName() const { return _IFT_MixedGradientPressureWeakPeriodic_Name; }
    
protected:
    void integrateTractionVelocityTangent(FloatMatrix &answer, Element *el, int boundary);
    void integrateTractionXTangent(FloatMatrix &answer, Element *el, int boundary);
    void integrateTractionDev(FloatArray &answer, Element *el, int boundary, const FloatMatrix &ddev);
    void evaluateTractionBasisFunctions(FloatArray &answer, const FloatArray &coords);
    
    void constructFullMatrixForm(FloatMatrix &d, const FloatArray &d_voigt) const;
};
} // end namespace oofem

#endif // mixedgradientpressurecneumann_h

