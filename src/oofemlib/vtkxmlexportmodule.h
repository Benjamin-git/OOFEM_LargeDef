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

#ifndef vtkxmlexportmodule_h
#define vtkxmlexportmodule_h

#include "exportmodule.h"
#include "domain.h"
#include "engngm.h"
#include "intarray.h"
#include "nodalrecoverymodel.h"
#include "interface.h"
#include "internalstatevaluetype.h"
#include "integrationrule.h"
#include "xfemmanager.h"



#ifdef __VTK_MODULE
 #include <vtkUnstructuredGrid.h>
 #include <vtkSmartPointer.h>
#endif

#include <string>
#include <list>

///@name Input fields for VTK XML export module
//@{
#define _IFT_VTKXMLExportModule_Name "vtkxml"
#define _IFT_VTKXMLExportModule_cellvars "cellvars"
#define _IFT_VTKXMLExportModule_vars "vars"
#define _IFT_VTKXMLExportModule_primvars "primvars"
#define _IFT_VTKXMLExportModule_stype "stype"
#define _IFT_VTKXMLExportModule_regionstoskip "regionstoskip"
#define _IFT_VTKXMLExportModule_nvr "nvr"
#define _IFT_VTKXMLExportModule_vrmap "vrmap"
//@}

namespace oofem {


    // New 
class Cell 
{
public:
    Cell(){};
    int cellType;
    std::vector<FloatArray> nodeCoords;
    IntArray connectivity;
    IntArray primVarsToExport;
    std::vector< std::vector<FloatArray> > nodeVars;
    std::vector<FloatArray> elVars;
    int offset;
};

class CompositeCell 
{
public:
    CompositeCell(){}; 
    int numSubEl;
    int numTotalNodes;
    std::vector<Cell> elements;
};

/**
 * Represents VTK (Visualization Toolkit) export module. It uses VTK (.vtu) file format, Unstructured grid dataset.
 * The export of data is done on Region By Region basis, possibly taking care about possible nonsmooth character of
 * some internal variables at region boundaries.
 * Each region is usually exported as a single piece. When region contains composite cells, these are assumed to be
 * exported in individual subsequent pieces after the default one for the particular region.
 */
class VTKXMLExportModule : public ExportModule
{
protected:
    /// List of InternalStateType values, identifying the selected vars for export.
    IntArray internalVarsToExport;
    /// List of primary unknowns to export.
    IntArray primaryVarsToExport;
    /// List of cell data to export.
    IntArray cellVarsToExport;

    /// Map from Voigt to full tensor.
    IntArray redToFull;

    /// Smoother type.
    NodalRecoveryModel::NodalRecoveryModelType stype;
    /// Smoother.
    NodalRecoveryModel *smoother;
    /// Smoother for primary variables.
    NodalRecoveryModel *primVarSmoother;
    /// List of regions to skip.
    IntArray regionsToSkip;
    /// Number of virtual regions.
    int nvr;
    /// Real->virtual region map.
    IntArray vrmap;
    /// Scaling time in output, e.g. conversion from seconds to hours
    double timeScale;

    /// Buffer for earlier time steps exported to *.pvd file.
    std::list< std::string > pvdBuffer;

public:
    /// Constructor. Creates empty Output Manager. By default all components are selected.
    VTKXMLExportModule(int n, EngngModel *e);
    /// Destructor
    virtual ~VTKXMLExportModule();

    virtual IRResultType initializeFrom(InputRecord *ir);
    virtual void doOutput(TimeStep *tStep, bool forcedOutput=false);
    virtual void initialize();
    virtual void terminate();
    virtual const char *giveClassName() const { return "VTKXMLExportModule"; }
    /**
     * Prints point data header.
     */
    void exportPointDataHeader(FILE *fileStream, TimeStep *tStep);
    void giveDataHeaders(std :: string &pointHeader, std :: string &cellHeader, TimeStep *tStep); // returns the headers
    /// Returns the internal smoother.
    NodalRecoveryModel *giveSmoother();
    /// Returns the smoother for primary variables (nodal averaging).
    NodalRecoveryModel *givePrimVarSmoother();

    //Experimental // JB
    #ifdef __VTK_MODULE
        vtkSmartPointer<vtkUnstructuredGrid> fileStream;
    #else
        FILE *fileStream;
    #endif


protected:
    /// Gives the full form of given symmetrically stored tensors, missing components are filled with zeros.
    void makeFullForm(FloatArray &answer, const FloatArray &reducedForm, InternalStateValueType type);
    void makeFullForm(FloatArray &answer, const FloatArray &reducedForm, InternalStateValueType type, const IntArray &redIndx);

    /// Returns the filename for the given time step.
    std::string giveOutputFileName(TimeStep *tStep);

    /// Returns the output stream for given solution step.
    FILE *giveOutputStream(TimeStep *tStep);
    /**
     * Returns corresponding element cell_type.
     * Some common element types are supported, others can be supported via interface concept.
     */
    int giveCellType(Element *element);
    /**
     * Returns the number of elements vtk cells.
     */
    int giveNumberOfElementCells(Element *element);
    /**
     * Returns number of nodes corresponding to cell type
     */
    int giveNumberOfNodesPerCell(int cellType);
    /**
     * Returns the element cell geometry.
     */
    void giveElementCell(IntArray &answer, Element *elem, int cell);
    //void giveElementCell(IntArray &answer, Element_Geometry_Type elemGT, int cell);
     
#ifndef __VTK_MODULE

#endif
    /**
     * Export internal variables by smoothing.
     */
    void exportIntVars(
#ifdef __VTK_MODULE
        vtkSmartPointer<vtkUnstructuredGrid> &this->fileStream,
#else
        FILE *fileStream,
#endif
        IntArray &mapG2L, IntArray &mapL2G, int regionDofMans, int ireg, TimeStep *tStep);

    void exportIntVars(IntArray &mapG2L, IntArray &mapL2G, int regionDofMans, int ireg, TimeStep *tStep);


    /**
     * Export primary variables.
     */
    void exportPrimaryVars(
#ifdef __VTK_MODULE
        vtkSmartPointer<vtkUnstructuredGrid> &this->fileStream,
#else
        FILE *fileStream,
#endif
        IntArray &mapG2L, IntArray &mapL2G, int regionDofMans, int region, TimeStep *tStep);

    void exportPrimaryVars(IntArray &mapG2L, IntArray &mapL2G, int regionDofMans, int region, TimeStep *tStep);


//  Tries to find the value of a primary field on the given DofManager.
//  Some elements have different interpolation of some fields, and requires some additional code to compute node values (if available).
//
    void getPrimaryVariable(FloatArray &answer, DofManager *dman, TimeStep *tStep, UnknownType type, int ireg);


// 
//  Exports single internal variable by smoothing.
//  
void exportIntVarAs(InternalStateType valID, IntArray &mapG2L, IntArray &mapL2G, int regionDofMans, int ireg,
#ifdef __VTK_MODULE
                        vtkSmartPointer<vtkUnstructuredGrid> &this->fileStream,
#else
                        FILE *fileStream,
#endif
                        TimeStep *tStep);

void exportIntVarAs(InternalStateType valID, IntArray &mapG2L, IntArray &mapL2G, int regionDofMans, int ireg, TimeStep *tStep);


void exportXFEMVarAs(XFEMStateType xfemstype, IntArray &mapG2L, IntArray &mapL2G, int regionDofMans, int ireg,
#ifdef __VTK_MODULE
                        vtkSmartPointer<vtkUnstructuredGrid> &this->fileStream,
#else
                        FILE *fileStream,
#endif
                        TimeStep *tStep, EnrichmentItem *ei);

void exportXFEMVarAs(XFEMStateType xfemstype, IntArray &mapG2L, IntArray &mapL2G, int regionDofMans, int ireg, TimeStep *tStep, EnrichmentItem *ei);

void getNodalVariableFromIS(FloatArray &answer, Node *node, IntArray &regionVarMap, TimeStep *tStep, InternalStateType type, int ireg); 

void getNodalVariableFromXFEMST(FloatArray &answer, Node *node, TimeStep *tStep, XFEMStateType xfemstype, int ireg, EnrichmentItem *ei); 

//
//  Exports single primary variable.
// 
void exportPrimVarAs(UnknownType valID, IntArray &mapG2L, IntArray &mapL2G,
                         int regionDofMans, int region,
#ifdef __VTK_MODULE
                         vtkSmartPointer<vtkUnstructuredGrid> &this->fileStream,
#else
                         FILE *fileStream,
#endif
                         TimeStep *tStep);

void exportPrimVarAs(UnknownType valID, IntArray &mapG2L, IntArray &mapL2G,
                         int regionDofMans, int region, TimeStep *tStep);


// 
//  Exports cell variables (typically internal variables).
//
void exportCellVars(
#ifdef __VTK_MODULE
        vtkSmartPointer<vtkUnstructuredGrid> &this->fileStream,
#else
        FILE *fileStream,
#endif
        int region, TimeStep *tStep);

void exportCellVars(int region, TimeStep *tStep);

//
//  Exports a single cell variable (typically an internal variable).
//
void exportCellVarAs(InternalStateType type, int region,
#ifdef __VTK_MODULE
                         vtkSmartPointer<vtkUnstructuredGrid> &this->fileStream,
#else
                         FILE *fileStream,
#endif
                         TimeStep *tStep);

void exportCellVarAs(InternalStateType type, int region, TimeStep *tStep);

    /**
     * Computes a cell average of an InternalStateType varible based on the weights 
     * in the integrationpoints (=> volume/area/length average)
     */
    void computeIPAverage(FloatArray &answer, IntegrationRule *iRule, Element *elem,  InternalStateType isType, TimeStep *tStep);


    /**
     * Assembles the region node map. Also computes the total number of nodes in region.
     * The region are numbered starting from offset+1.
     * If mode == 0 then regionNodalNumbers is array with mapping from global numbering to local region numbering.
     * The i-th value contains the corresponding local region number (or zero, if global number is not in region).
     * If mode == 1 then regionNodalNumbers is array with mapping from local to global numbering.
     * The i-th value contains the corresponding global node number.
     */
    int initRegionNodeNumbering(IntArray &mapG2L, IntArray &mapL2G,
                                int &regionDofMans, int &totalcells,
                                Domain *domain, int reg);
    /**
     * Writes a VTK collection file where time step data is stored.
     */
    void writeVTKCollection();



    // Export of composite elements (built up from several subcells)
    
    CompositeCell compositeCell;
    bool isElementComposite(Element *elem); /// Returns true if element geometry type is composite (not a single cell).

    void exportCompositeElement(FILE *fileStream, VTKXMLExportModule *expModule, IntArray &primaryVarsToExport, IntArray &internalVarsToExport, TimeStep *tStep);
    void exportCompositeElement(FILE *fileStream, Element *el, TimeStep *tStep);
    void exportCompositeElement(Element *el, TimeStep *tStep);

    void exportNodalVarAs(InternalStateType type, int nodeVarNum, FILE *fileStream, TimeStep *tStep);
    void exportNodalVarAs(InternalStateType type, int nodeVarNum, TimeStep *tStep);

    void exportCellVarAs(InternalStateType type, std::vector<FloatArray> &cellVars, FILE *fileStream, TimeStep *tStep);

    void giveCompositeExportData();

};




/**
 * Elements with geometry defined as EGT_Composite are exported using individual pieces.
 * The VTKXMLExportModuleElementInterface serves for this purpose, defining abstract
 * export method, responsible for exporting individual element piece in xml vtk syntax.
 * Elements with geometry defined as EGT_Composite should implement this interface.
 */
class VTKXMLExportModuleElementInterface : public Interface
{
public:
    VTKXMLExportModuleElementInterface() : Interface() {}
    virtual const char *giveClassName() const { return "VTKXMLExportModuleElementInterface"; }
    virtual void giveCompositeExportData(CompositeCell &compositeCell, IntArray &primaryVarsToExport, IntArray &internalVarsToExport, TimeStep *tStep ){};
};



} // end namespace oofem
#endif // vtkxmlexportmodule_h
