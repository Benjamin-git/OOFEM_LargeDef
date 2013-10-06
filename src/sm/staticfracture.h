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

#ifndef staticfracture_h
#define staticfracture_h
#include "nlinearstatic.h"
#include "metastep.h"
#include "xfemmanager.h"
#include "fracturemanager.h"

#define _IFT_StaticFracture_Name "staticfracture"

namespace oofem {
/**
 * This class implements a nonlinear static fracture problem.
 * It provides (or will) functionality for updating the model after each time step according to some propagation model
 */
class StaticFracture : public NonLinearStatic
{
protected:

    // from nlinearstatic
    virtual void solveYourselfAt(TimeStep *tStep);
    virtual void terminate(TimeStep *tStep);
    virtual void updateLoadVectors(TimeStep *tStep);


    virtual void updateYourself(TimeStep *tStep);


    virtual double giveUnknownComponent(ValueModeType mode, TimeStep *tStep, Domain *d, Dof *dof);
    
    // for updating structure
    void setTotalDisplacementFromUnknownsInDictionary(EquationID type, ValueModeType mode, TimeStep *tStep);
    void initializeDofUnknownsDictionary(TimeStep *tStep);
    virtual void updateDofUnknownsDictionary(DofManager *inode, TimeStep *tStep);

    void evaluatePropagationLaw(TimeStep *tStep);
    void evaluatePropagationLawForDelamination(Element *el, EnrichmentDomain *ed, TimeStep *tStep);
    void computeInterLaminarStressesAt(int layer, Element *el, TimeStep *tStep, std::vector < FloatArray > &interLamStresses);
    void evaluateFractureCriterion(std::vector < FloatArray > &interLamStresses, bool &propagateFlag);
    bool updateStructureFlag;

    // Fracture manager stuff
    FractureManager *fMan;

public:
    StaticFracture(int i, EngngModel *_master = NULL);
    virtual ~StaticFracture(){};
    virtual int requiresUnknownsDictionaryUpdate() { return updateStructureFlag; }
    virtual bool requiresEquationRenumbering(TimeStep *) { return updateStructureFlag; }
    void setUpdateStructureFlag(bool flag) { updateStructureFlag = flag; }
    bool needsStructureUpdate() {return updateStructureFlag; };


    // new topology opt

    void optimalityCriteria(int numElX, int numElY, FloatArray &designVarList, double volFrac, FloatArray dCostFunction);
    FloatArray designVarList;
};

} // end namespace oofem
#endif // nlinearstatic_h
