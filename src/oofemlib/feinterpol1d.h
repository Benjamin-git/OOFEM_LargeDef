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

#ifndef feinterpol1d_h
#define feinterpol1d_h

#include "feinterpol.h"

namespace oofem {
/**
 * Class representing a general abstraction for finite element interpolation class.
 */
class FEInterpolation1d : public FEInterpolation
{
public:
    FEInterpolation1d(int o) : FEInterpolation(o) { }
    virtual int giveNsd() { return 1; }

    virtual void boundaryEdgeGiveNodes(IntArray &answer, int boundary)
    { OOFEM_ERROR("FEInterpolation1d :: boundaryEdge... - Functions not supported for this interpolator."); }
    virtual void boundaryEdgeEvalN(FloatArray &answer, int boundary, const FloatArray &lcoords, const FEICellGeometry &cellgeo)
    { OOFEM_ERROR("FEInterpolation1d :: boundaryEdge... - Functions not supported for this interpolator."); }
    virtual double boundaryEdgeGiveTransformationJacobian(int boundary, const FloatArray &lcoords, const FEICellGeometry &cellgeo)
    { OOFEM_ERROR("FEInterpolation1d :: boundaryEdge... - Functions not supported for this interpolator."); return 0.; }
    virtual void boundaryEdgeLocal2Global(FloatArray &answer, int boundary, const FloatArray &lcoords, const FEICellGeometry &cellgeo)
    { OOFEM_ERROR("FEInterpolation1d :: boundaryEdge... - Functions not supported for this interpolator."); }

    virtual void boundaryGiveNodes(IntArray &answer, int boundary);
    virtual void boundaryEvalN(FloatArray &answer, int boundary, const FloatArray &lcoords, const FEICellGeometry &cellgeo);
    virtual double boundaryEvalNormal(FloatArray &answer, int boundary, const FloatArray &lcoords, const FEICellGeometry &cellgeo);
    virtual double boundaryGiveTransformationJacobian(int boundary, const FloatArray &lcoords, const FEICellGeometry &cellgeo);
    virtual void boundaryLocal2Global(FloatArray &answer, int boundary, const FloatArray &lcoords, const FEICellGeometry &cellgeo);
    /**
     * Computes the exact length.
     * @param cellgeo Cell geometry for the element.
     * @return Length of geometry.
     */
    virtual double giveLength(const FEICellGeometry &cellgeo) const
    { OOFEM_ERROR("FEInterpolation1d :: giveLength - Not implemented in subclass."); return 0; }

    virtual IntegrationRule *giveIntegrationRule(int order);
    virtual IntegrationRule *giveBoundaryIntegrationRule(int order, int boundary);
    virtual IntegrationRule *giveBoundaryEdgeIntegrationRule(int order, int boundary);
};
} // end namespace oofem
#endif // feinterpol1d_h






