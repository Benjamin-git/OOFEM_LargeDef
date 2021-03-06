/*
 * XFEMDebugTools.h
 *
 *  Created on: Jun 5, 2013
 *      Author: svennine
 */

#ifndef XFEMDEBUGTOOLS_H_
#define XFEMDEBUGTOOLS_H_

#include "geometry.h"
#include <fstream>

namespace oofem {
class XFEMDebugTools
{
public:
    XFEMDebugTools();
    virtual ~XFEMDebugTools();

    static void WriteTrianglesToVTK(const std :: string &iName, const std::vector< Triangle > &iTriangles);
    static void WritePointsToVTK(const std :: string &iName, const std :: vector< FloatArray > &iPoints);
};
} /* namespace oofem */
#endif /* XFEMDEBUGTOOLS_H_ */
