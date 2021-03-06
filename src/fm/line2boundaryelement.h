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

#ifndef line2boundaryelement_h
#define line2boundaryelement_h

#include "fmelement.h"
#include "spatiallocalizer.h"
#include "eleminterpmapperinterface.h"

#define _IFT_Line2BoundaryElement_Name "line2boundary"

namespace oofem {
class FEI2dLineQuad;

/**
 * Boundary element used for tracking solutions on arbitrary sections.
 * Also convenient for computing the RVE volume.
 * @author Mikael Öhman
 */
class Line2BoundaryElement :
    public FMElement,
    public SpatialLocalizerInterface,
    public EIPrimaryUnknownMapperInterface
{
protected:
    static FEI2dLineQuad fei;

public:
    /**
     * Constructor.
     * @param n Element's number.
     * @param d Pointer to the domain to which element belongs.
     */
    Line2BoundaryElement(int n, Domain *d);
    /// Destructor.
    virtual ~Line2BoundaryElement();

    virtual IRResultType initializeFrom(InputRecord *ir);

    virtual void giveCharacteristicVector(FloatArray &answer, CharType type, ValueModeType mode, TimeStep *tStep) { answer.resize(0); }
    virtual void giveCharacteristicMatrix(FloatMatrix &answer, CharType type, TimeStep *tStep) { answer.resize(0,0); }

    virtual void giveDofManDofIDMask(int i, EquationID eid, IntArray &nodeDofIDMask) const;

    virtual FEInterpolation *giveInterpolation() const;
    virtual int computeNumberOfDofs(EquationID eid) { return 6; }

    virtual const char *giveClassName() const { return "Line2BoundaryElement"; }
    virtual const char *giveInputRecordName() const { return _IFT_Line2BoundaryElement_Name; }

    // Interfaces
    virtual Interface* giveInterface(InterfaceType it);

    virtual Element *SpatialLocalizerI_giveElement() { return this; }
    virtual int SpatialLocalizerI_containsPoint(const FloatArray &coords) { return false; }
    virtual double SpatialLocalizerI_giveDistanceFromParametricCenter(const FloatArray &gcoords);

    virtual int EIPrimaryUnknownMI_computePrimaryUnknownVectorAt(ValueModeType mode,
        TimeStep *tStep, const FloatArray &gcoords, FloatArray &answer);
    virtual void EIPrimaryUnknownMI_computePrimaryUnknownVectorAtLocal(ValueModeType mode,
        TimeStep *tStep, const FloatArray &lcoords, FloatArray &answer);
    virtual void EIPrimaryUnknownMI_givePrimaryUnknownVectorDofID(IntArray &answer);
};
}

#endif // line2boundaryelement_h
