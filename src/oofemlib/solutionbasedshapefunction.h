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

#ifndef SOLUTIONBASEDSHAPEFUNCTION_H_
#define SOLUTIONBASEDSHAPEFUNCTION_H_

#include "activebc.h"
#include "node.h"

#define _IFT_SolutionbasedShapeFunction_Name "solutionbasedshapefunction"
#define _IFT_SolutionbasedShapeFunction_Set "set"
#define _IFT_SolutionbasedShapeFunction_ShapeFunctionFile "shapefunctionfile"


namespace oofem {
class SolutionbasedShapeFunction : public ActiveBoundaryCondition {
private:
	Node *myNode;
	int set;
	std::string filename;
	bool useConstantBase;
	bool isLoaded;
    EngngModel *myEngngModel;
    TimeStep *thisTimestep;

    FloatArray maxCoord, minCoord;

	double computeBaseFunctionValueAt(FloatArray *coords, Dof *dof);

	void setLoads();
	void loadProblem();
	void init();

public:
	SolutionbasedShapeFunction(int n, Domain *d);
	virtual ~SolutionbasedShapeFunction();

	IRResultType initializeFrom(InputRecord *ir);

	virtual bool requiresActiveDofs() {return true;};
    virtual int giveNumberOfInternalDofManagers() { return 1; }
    virtual DofManager *giveInternalDofManager(int i) { return myNode; }

    virtual double giveUnknown(PrimaryField &field, ValueModeType mode, TimeStep *tStep, ActiveDof *dof);
    virtual double giveUnknown(ValueModeType mode, TimeStep *tStep, ActiveDof *dof);

    virtual bool hasBc(ActiveDof *dof, TimeStep *tStep) { return false; }

    virtual bool isPrimaryDof(ActiveDof *dof) { return false; };

    virtual void computeDofTransformation(ActiveDof *dof, FloatArray &masterContribs);

    virtual void giveLocationArrays(std::vector<IntArray> &rows, std::vector<IntArray> &cols, EquationID eid, CharType type,
                                    const UnknownNumberingScheme &r_s, const UnknownNumberingScheme &c_s);

    virtual int giveNumberOfMasterDofs(ActiveDof *dof) {return 1; };
    virtual Dof *giveMasterDof(ActiveDof *dof, int mdof);

    virtual const char *giveClassName() const { return "SolutionbasedShapeFunction"; }
    virtual const char *giveInputRecordName() const { return _IFT_SolutionbasedShapeFunction_Name; }
};
}

#endif /* SOLUTIONBASEDSHAPEFUNCTION_H_ */
