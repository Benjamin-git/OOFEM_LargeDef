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

#ifndef slepcsolver_h
#define slepcsolver_h

#include "sparsegeneigenvalsystemnm.h"
#include "sparsemtrx.h"
#include "floatarray.h"

#include <slepceps.h>

namespace oofem {
class Domain;
class EngngModel;
class FloatMatrix;
class PetscSparseMtrx;

class SLEPcSolver : public SparseGeneralEigenValueSystemNM
{
private:
    PetscSparseMtrx *A;
    PetscSparseMtrx *B;
    /// Eigenvalue solver context.
    EPS eps;
    /// Flag if context initialized.
    bool epsInit;

public:
    SLEPcSolver(Domain *d, EngngModel *m);
    virtual ~SLEPcSolver();

    virtual NM_Status solve(SparseMtrx *a, SparseMtrx *b, FloatArray *v, FloatMatrix *x, double rtol, int nroot);
    virtual const char *giveClassName() const { return "SLEPcSolver"; }
};
} // end namespace oofem
#endif // slepcsolver_h
