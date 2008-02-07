/* $Header: /home/cvs/bp/oofem/oofemlib/src/datareader.h,v 1.2 2003/05/19 13:03:57 bp Exp $ */
/*

                   *****    *****   ******  ******  ***   ***                            
                 **   **  **   **  **      **      ** *** **                             
                **   **  **   **  ****    ****    **  *  **                              
               **   **  **   **  **      **      **     **                               
              **   **  **   **  **      **      **     **                                
              *****    *****   **      ******  **     **         
            
                                                                   
               OOFEM : Object Oriented Finite Element Code                 
                    
                 Copyright (C) 1993 - 2000   Borek Patzak                                       



         Czech Technical University, Faculty of Civil Engineering,
     Department of Structural Mechanics, 166 29 Prague, Czech Republic
                                                                               
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  
*/


//
// Class DataReader
//

#ifndef datareader_h

#include "cltypes.h"
#include "inputrecord.h"

/**
 Class representing the abstraction for input data source.
 Its role is to provide input records for particular components. 
 The input records are identified by record type and component number.
 The order of input records is in fact determined by the coded sequence of
 component initialization. The input record identification facilitates the
 implementation of database readers with direct or random access.
*/
class DataReader
{
public:

  DataReader() {}
  virtual ~DataReader() {}
  enum InputRecordType {IR_outFileRec, IR_jobRec, IR_domainRec, IR_outManRec, IR_domainCompRec, 
                        IR_emodelRec, IR_mstepRec, IR_expModuleRec, IR_dofmanRec, IR_elemRec, 
                        IR_crosssectRec, IR_matRec, IR_nlocBarRec, IR_bcRec, IR_icRec, IR_ltfRec};

 /**
  Returns input record corresponding to given InputRecordType value and its record_id.
  The returned inputrecord reference is valid only until the next call.
  @param irType determines type of record to be returned
  @param recordID determines the record  number corresponding to component number
  */
 virtual InputRecord* giveInputRecord (InputRecordType irType, int recordId) = 0;

 /**
  Allows to detach all data connections.
  */
 virtual void finish () = 0;

 /// prints the name (shortened) of data source
 virtual const char* giveDataSourceName () const = 0;
 /// Prints the error message 
 void report_error (const char* _class, const char* proc, const char* kwd, IRResultType result, const char* file, int line);

};

#define datareader_h
#endif