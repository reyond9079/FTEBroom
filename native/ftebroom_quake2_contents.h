// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <sstream>
#include <string>
#include <iomanip>
namespace ftebroom {
// Older editor builds ORed inferred SOLID into explicit Q2 visible contents.
// Remove it only alongside one unambiguous WINDOW/AUX/liquid/MIST type.
// Multiple non-solid visible types remain an error for the compiler to report.
inline std::string q2CompileContents(const std::string& source,size_t& corrected){
  corrected=0;std::istringstream lines(source);std::ostringstream output;std::string line;
  while(std::getline(lines,line)){
    std::istringstream row(line);row>>std::ws;
    if(row.peek()=='('){
      bool valid=true;char c;double number;
      for(int p=0;p<3&&valid;p++){if(!(row>>c)||c!='('){valid=false;break;}for(int a=0;a<3;a++)if(!(row>>number))valid=false;if(!(row>>c)||c!=')')valid=false;}
      std::string material;if(valid&&!(row>>std::quoted(material)))valid=false;
      row>>std::ws;
      if(valid&&row.peek()=='['){
        for(int axis=0;axis<2&&valid;axis++){if(!(row>>c)||c!='['){valid=false;break;}for(int a=0;a<4;a++)if(!(row>>number))valid=false;if(!(row>>c)||c!=']')valid=false;}
        for(int a=0;a<3&&valid;a++)if(!(row>>number))valid=false;
      }else for(int a=0;a<5&&valid;a++)if(!(row>>number))valid=false;
      if(valid){
        row>>std::ws;auto begin=row.tellg();std::int64_t contents,flags,value;
        if(row>>contents){auto end=row.tellg();if(row>>flags>>value){
          const auto visible=contents&127,nonSolid=visible&~std::int64_t(1);
          if((visible&1)&&nonSolid&&!(nonSolid&(nonSolid-1))&&begin>=0&&end>=begin){line.replace(size_t(begin),size_t(end-begin),std::to_string(contents&~std::int64_t(1)));corrected++;}
        }}
      }
    }
    output<<line<<'\n';
  }
  return corrected?output.str():source;
}
}
