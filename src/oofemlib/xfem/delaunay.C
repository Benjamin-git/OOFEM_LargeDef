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

#include "delaunay.h"
#include "floatarray.h"
#include "intarray.h"
#include "alist.h"
#include "geometry.h"
#include "node.h"
#include "mathfem.h"

#include <cstdlib>
#include <map>

namespace oofem {
bool Delaunay :: colinear(FloatArray *p1, FloatArray *p2, FloatArray *p3)
{
    double dist = p1->at(1) * ( p2->at(2) - p3->at(2) ) + p2->at(1) * ( p3->at(2) - p1->at(2) ) +
                  p3->at(1) * ( p1->at(2) - p2->at(2) );

    if ( dist < mTol && dist > -mTol ) {
        return true;
    } else {
        return false;
    }
}

void Delaunay :: printTriangles(AList< Triangle > *triangles)
{
    for ( int i = 1; i <= triangles->giveSize(); i++ ) {
        triangles->at(i)->printYourself();
    }
}

bool Delaunay :: isInsideCC(FloatArray *p, FloatArray *p1,  FloatArray *p2,  FloatArray *p3)
{
    FloatArray *nodesCopy1 = new FloatArray(*p1);
    FloatArray *nodesCopy2 = new FloatArray(*p2);
    FloatArray *nodesCopy3 = new FloatArray(*p3);
    Triangle *tr = new Triangle(nodesCopy1, nodesCopy2, nodesCopy3);
    double r = tr->getRadiusOfCircumCircle();
    FloatArray circumCenter;
    tr->computeCenterOfCircumCircle(circumCenter);
    double distance = circumCenter.distance(p);
    delete tr;
    if ( distance < r ) {
        return true;
    } else {
        return false;
    }
}

void Delaunay :: triangulate(const std :: vector< FloatArray > &iVertices, std::vector< Triangle > &oTriangles)
{
    // 4th order algorithm - four loops, only for testing purposes

    int n = iVertices.size();

    // copy of vertices, since they will be shifted
    std :: vector< FloatArray >vertices(iVertices);

    // small shift of vertices
    const double shift = 1.0e-12;
    for ( int i = 1; i <= n; i++ ) {
        vertices [ i - 1 ].at(1) += vertices [ i - 1 ].at(1) * shift * double ( rand() ) / RAND_MAX;
        vertices [ i - 1 ].at(2) += vertices [ i - 1 ].at(2) * shift * double ( rand() ) / RAND_MAX;
    }

    for ( int i = 1; i <= n; i++ ) {
        for ( int j = i + 1; j <= n; j++ ) {
            for ( int k = j + 1; k <= n; k++ ) {
                bool isTriangle = true;
                if ( colinear(& vertices [ i - 1 ], & vertices [ j - 1 ], & vertices [ k - 1 ]) ) {
                    isTriangle = false;
                } else   {
                    for ( int a = 1; a <= n; a++ ) {
                        if ( a != i && a != j && a != k ) {
                            // checks whether a point a is inside a circumcircle of a triangle ijk
                            if ( isInsideCC(& vertices [ a - 1 ], & vertices [ i - 1 ], & vertices [ j - 1 ],
                                            & vertices [ k - 1 ]) ) {
                                isTriangle = false;
                                break;
                            }
                        }
                    }
                }

                if ( isTriangle ) {

                    // here we switch to old vertices
                    FloatArray *p1 = new FloatArray();
                    * p1 =  iVertices [ i - 1 ];
                    FloatArray *p2 = new FloatArray();
                    * p2 =  iVertices [ j - 1 ];
                    FloatArray *p3 = new FloatArray();
                    * p3 =  iVertices [ k - 1 ];


                    Triangle tri(p1, p2, p3);
                    if ( !tri.isOrientedAnticlockwise() ) {
                        tri.changeToAnticlockwise();
                    }

                    oTriangles.push_back(tri);
                }
            }
        }
    }
}
} // end namespace oofem
