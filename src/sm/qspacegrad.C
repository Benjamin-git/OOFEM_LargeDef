/* $Header: /home/cvs/bp/oofem/sm/src/qspacegrad.C,v 1.3.4.1 20011/05/10 15:19:47 bp Exp $ */
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
 *               Copyright (C) 1993 - 2008   Borek Patzak
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

//   file QSPACE.CC

//

#include "qspacegrad.h"
#include "node.h"
#include "material.h"
#include "gausspnt.h"
#include "gaussintegrationrule.h"
#include "flotmtrx.h"
#include "flotarry.h"
#include "intarray.h"
#include "domain.h"
#include "cltypes.h"
#include "structuralms.h"
#include "mathfem.h"
#include "structuralcrosssection.h"

#ifndef __MAKEDEPEND
#include <stdio.h>
#endif

namespace oofem {

FEI3dHexaLin QSpaceGrad :: interpolation;

  QSpaceGrad :: QSpaceGrad (int n, Domain* aDomain) :  QSpace(n, aDomain),GradDpElement()
  // Constructor.
{
  nPrimNodes = 8; 
  nPrimVars = 2;
  nSecNodes = 4;
  nSecVars = 1;
  totalSize = nPrimVars*nPrimNodes+nSecVars*nSecNodes;
  locSize   = nPrimVars*nPrimNodes;
  nlSize    = nSecVars*nSecNodes;
 
}


IRResultType
QSpaceGrad :: initializeFrom (InputRecord* ir)
{
  const char *__proc = "initializeFrom"; // Required by IR_GIVE_FIELD macro
  IRResultType result;                   // Required by IR_GIVE_FIELD macro

  this->StructuralElement :: initializeFrom (ir);
  IR_GIVE_OPTIONAL_FIELD (ir, numberOfGaussPoints, IFT_QSpaceGrad_nip, "nip"); // Macro

  if ((numberOfGaussPoints != 8) && (numberOfGaussPoints != 14) && (numberOfGaussPoints != 27) && (numberOfGaussPoints != 64)) numberOfGaussPoints = 27;

  // set - up Gaussian integration points
  this -> computeGaussPoints();

  return IRRT_OK;
}


void
QSpaceGrad :: giveDofManDofIDMask (int inode, EquationID ut, IntArray& answer) const
  // returns DofId mask array for inode element node.
  // DofId mask array determines the dof ordering requsted from node.
  // DofId mask array contains the DofID constants (defined in cltypes.h)
  // describing physical meaning of particular DOFs.
{
  if(inode<=nSecNodes)
    {
      answer.resize (4);
      answer.at(1) = D_u;
      answer.at(2) = D_v;
      answer.at(3) = D_w; 
      answer.at(4) = G_0;
    }
  else
  {
      answer.resize (3);
      answer.at(1) = D_u;
      answer.at(2) = D_v;
      answer.at(3) = D_w; 
  }
 
  return ;
}

void
QSpaceGrad :: computeGaussPoints ()
  // Sets up the array containing the four Gauss points of the receiver.
{
  numberOfIntegrationRules = 1;
  integrationRulesArray = new IntegrationRule* [numberOfIntegrationRules];
  integrationRulesArray[0] = new GaussIntegrationRule (1,this,1, 7);
  integrationRulesArray[0]->setUpIntegrationPoints (_Cube, numberOfGaussPoints, _3dMatGrad);
}


void
QSpaceGrad :: computeNkappaMatrixAt (GaussPoint* aGaussPoint,FloatMatrix& answer)
  // Returns the displacement interpolation matrix {N} of the receiver, eva-
  // luated at aGaussPoint.
{
  int i;
  FloatArray n(8);
  this->interpolation.evalN (n, *aGaussPoint->giveCoordinates(),FEIElementGeometryWrapper(this), 0.0);
  answer.resize(1,8);
  answer.zero();

  for ( int i = 1; i <= 8; i++ ) {
        answer.at(1, i) = n.at(i);
  }
  
}

void
QSpaceGrad :: computeBkappaMatrixAt(GaussPoint *aGaussPoint, FloatMatrix& answer)
 {
  FloatMatrix dnx;
  IntArray a(8);
  for(int i =1; i<9;i++){
    a.at(i) = dofManArray.at(i);
  }
  answer.resize(3,8);
  answer.zero();

  this->interpolation.evaldNdx (dnx, *aGaussPoint->giveCoordinates(),FEIElementGeometryWrapper(this), 0.0);
   for ( int i = 1; i <= 8; i++ ) {
     answer.at(1, i) = dnx.at(i,1);
     answer.at(2, i) = dnx.at(i,2);
     answer.at(3, i) = dnx.at(i,3);
  }
  
 }


}