/* $Header: /home/cvs/bp/oofem/oofemlib/src/buffereddatareader.h,v 1.2 2003/05/19 13:03:56 bp Exp $ */
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
// Class BufferedDataReader
//

#ifndef buffereddatareader_h

#ifndef __MAKEDEPEND
#include <stdio.h>
#include <string>
#endif
#include "cltypes.h"
#include "datareader.h"
#include "oofemtxtinputrecord.h"

/**
 Class representing the implemantaion of plain text date reader.
 It reads a sequentional seqence of input records from data file
 and creates the corresponding input records.
 There is no check for record type requested, it is assumed that records are
 written in coorrect order, which determined by the coded sequence of
 component initialization and described in input manual. 
*/
class BufferedDataReader : public DataReader
{
protected:
 OOFEMTXTInputRecord ir;
 dynaList<std::string> buffer;
 dynaList<std::string>::iterator pos;
 
public:
 
 
 /** Constructor. */
 BufferedDataReader ();
 ~BufferedDataReader ();
 
 virtual InputRecord* giveInputRecord (InputRecordType, int recordId);
 virtual void finish ();
 /// prints the name (shortened) of data source
 virtual const char* giveDataSourceName () const {return "BufferedDataReader source";}

 void appendInputString(std::string &str);
 void appendInputString(const char *line);
 void rewind();
 void seek(int position);

 void printYourself();
 void writeToFile(char *fileName);

protected:
 void giveRawLineFromInput (char* line);
 void giveLineFromInput (char* line);
};

#define buffereddatareader_h
#endif