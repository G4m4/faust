/************************************************************************
 ************************************************************************
    FAUST compiler
    Copyright (C) 2003-2018 GRAME, Centre National de Creation Musicale
    ---------------------------------------------------------------------
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
 ************************************************************************
 ************************************************************************/

#ifndef _STRUCT_MANAGER_H
#define _STRUCT_MANAGER_H

#include <string>
#include <utility>
#include <vector>

#include "instructions.hh"

// Describe a field memory location in the DSP structure
struct MemoryDesc {
    enum memType { kLocal, kExternal };
    int fIndex;             // Index
    int fOffset;            // Offset in bytes in a mixed int/real zone
    int fIntOffset;         // Offset in bytes in a separated int zone
    int fRealOffset;        // Offset in bytes in a separated real zone
    int fRAccessCount;      // Read access counter
    int fWAccessCount;      // Write access counter
    int fSizeBytes;         // Size in bytes
    Typed::VarType fType;   // FIR type
    memType fMemType;       // Memory type

    MemoryDesc() : fIndex(-1), fOffset(-1),
        fIntOffset(-1), fRealOffset(-1),
        fRAccessCount(0), fWAccessCount(0),
        fSizeBytes(-1), fType(Typed::kNoType), fMemType(kLocal) {}

    MemoryDesc(int index, int offset, int size_bytes, Typed::VarType type)
    : fIndex(index), fOffset(offset),
        fIntOffset(-1), fRealOffset(-1),
        fRAccessCount(0), fWAccessCount(0),
        fSizeBytes(size_bytes), fType(type), fMemType(kLocal) {}
 
    MemoryDesc(int index, int offset, int int_offset, int read_offset, int size_bytes, Typed::VarType type, memType mem_type = kLocal)
    : fIndex(index), fOffset(offset),
        fIntOffset(int_offset), fRealOffset(read_offset),
        fRAccessCount(0), fWAccessCount(0),
        fSizeBytes(size_bytes), fType(type), fMemType(mem_type) {}
    
    Typed* getTyped()
    {
        if (fSizeBytes > 1) {
            return InstBuilder::genArrayTyped(InstBuilder::genBasicTyped(fType), fSizeBytes);
        } else {
            return InstBuilder::genBasicTyped(fType);
        }
    }
};

/*
 Compute all field info, the DSP size, and separate 'int' and 'real' types
 */
struct StructInstVisitor : public DispatchVisitor {
    int        fStructIntOffset;    // Keep the int offset in bytes
    int        fStructRealOffset;   // Keep the real offset in bytes
    int        fFieldIndex;         // Keep the field index
    MemoryDesc fDefault;
    
    // Vector is used so that field names are ordered in 'getStructType'
    typedef vector<pair<string, MemoryDesc> > field_table_type;
    
    field_table_type fFieldTable;  // Table: field_name, { index, offset, size, type }
    
    StructInstVisitor() : fStructIntOffset(0), fStructRealOffset(0), fFieldIndex(0) {}
    
    // Check if the field name exists
    bool hasField(const string& name, Typed::VarType& type)
    {
        for (const auto& field : fFieldTable) {
            if (field.first == name) {
                type = field.second.fType;
                return true;
            }
        }
        return false;
    }
    
    // Return the offset of a given field in bytes
    int getFieldOffset(const string& name)
    {
        for (const auto& field : fFieldTable) {
            if (field.first == name) return field.second.fOffset;
        }
        std::cerr << "ERROR in getFieldOffset : " << name << std::endl;
        faustassert(false);
        return -1;
    }
    
    // Return the int offset of a given field in bytes
    int getFieldIntOffset(const string& name)
    {
        for (const auto& field : fFieldTable) {
            if (field.first == name) return field.second.fIntOffset;
        }
        std::cerr << "ERROR in getFieldIntOffset : " << name << std::endl;
        faustassert(false);
        return -1;
    }
    
    // Return the real offset of a given field in bytes
    int getFieldRealOffset(const string& name)
    {
        for (const auto& field : fFieldTable) {
            if (field.first == name) return field.second.fRealOffset;
        }
        std::cerr << "ERROR in getFieldRealOffset : " << name << std::endl;
        faustassert(false);
        return -1;
    }
    
    // Return the index of a given field
    int getFieldIndex(const string& name)
    {
        for (const auto& field : fFieldTable) {
            if (field.first == name) return field.second.fIndex;
        }
        std::cerr << "ERROR in getFieldIndex : " << name << std::endl;
        faustassert(false);
        return -1;
    }
    
    // Return the FIR type of a given field
    Typed::VarType getFieldType(const string& name)
    {
        for (const auto& field : fFieldTable) {
            if (field.first == name) return field.second.fType;
        }
        std::cerr << "ERROR in getFieldType : " << name << std::endl;
        faustassert(false);
        return Typed::kNoType;
    }
    
    // Return the memory type of a given field
    MemoryDesc::memType getFieldMemoryType(const string& name)
    {
        for (const auto& field : fFieldTable) {
            if (field.first == name) return field.second.fMemType;
        }
        std::cerr << "ERROR in getFieldMemoryType : " << name << std::endl;
        faustassert(false);
        return MemoryDesc::kLocal;
    }
    
    // Return the memory description of a given field
    MemoryDesc& getMemoryDesc(const string& name)
    {
        for (auto& field : fFieldTable) {
            if (field.first == name) return field.second;
        }
        return fDefault;
    }
    
    // Return the struct 'int' size in bytes
    int getStructIntSize() { return fStructIntOffset; }
    
    // Return the struct 'real' size in bytes
    int getStructRealSize() { return fStructRealOffset; }
    
    // Return the struct size in bytes
    int getStructSize() { return fStructIntOffset + fStructRealOffset; }
    
    field_table_type& getFieldTable() { return fFieldTable; }
    
    // Return the struct type
    DeclareStructTypeInst* getStructType(const string& name)
    {
        vector<NamedTyped*> dsp_type_fields;
        for (auto& field : fFieldTable) {
            dsp_type_fields.push_back(InstBuilder::genNamedTyped(field.first, field.second.getTyped()));
        }
        return InstBuilder::genDeclareStructTypeInst(InstBuilder::genStructTyped(name, dsp_type_fields));
    }
    
    // Declarations
    void visit(DeclareVarInst* inst)
    {
        string              name   = inst->fAddress->getName();
        Address::AccessType access = inst->fAddress->getAccess();
        
        bool        is_struct   = (access & Address::kStruct) || (access & Address::kStaticStruct);
        ArrayTyped* array_typed = dynamic_cast<ArrayTyped*>(inst->fType);
        
        if (array_typed && array_typed->fSize > 1) {
            if (is_struct) {
                fFieldTable.push_back(make_pair(name, MemoryDesc(fFieldIndex++,
                                                                 getStructSize(),
                                                                 getStructIntSize(),
                                                                 getStructRealSize(),
                                                                 array_typed->fSize,
                                                                 array_typed->fType->getType())));
                if (array_typed->fType->getType() == Typed::kInt32) {
                    fStructIntOffset += array_typed->getSizeBytes();
                } else {
                    fStructRealOffset += array_typed->getSizeBytes();
                }
            } else {
                // Should never happen...
                faustassert(false);
            }
        } else {
            if (is_struct) {
                fFieldTable.push_back(make_pair(name, MemoryDesc(fFieldIndex++,
                                                                 getStructSize(),
                                                                 getStructIntSize(),
                                                                 getStructRealSize(),
                                                                 1,
                                                                 inst->fType->getType())));
                if (inst->fType->getType() == Typed::kInt32) {
                    fStructIntOffset += inst->fType->getSizeBytes();
                } else {
                    fStructRealOffset += inst->fType->getSizeBytes();
                }
            } else {
                // Local variables declared by [var_num, type] pairs
            }
        }
    
        if (inst->fValue) getMemoryDesc(inst->getName()).fWAccessCount++;
        DispatchVisitor::visit(inst);
    }
       
    void visit(LoadVarInst* inst)
    {
        getMemoryDesc(inst->getName()).fRAccessCount++;
        DispatchVisitor::visit(inst);
    }
     
    void visit(StoreVarInst* inst)
    {
        getMemoryDesc(inst->getName()).fWAccessCount++;
        DispatchVisitor::visit(inst);
    }
    
};

// A version that separates some of the fields for the iZone/fZone model
// and keep the others in the DSP struct.

struct StructInstVisitor1 : public StructInstVisitor {

    int fExternalMemory;
    int fDLThreshold;
    
    // To be computed with dsp_struct_size and max_size_bytes
    StructInstVisitor1(int external_memory, int dl_threshold = 4)
    : StructInstVisitor(), fExternalMemory(external_memory), fDLThreshold(dl_threshold)
    {}
    
    // Declarations
    void visit(DeclareVarInst* inst)
    {
        string              name   = inst->fAddress->getName();
        Address::AccessType access = inst->fAddress->getAccess();
        
        bool        is_struct   = (access & Address::kStruct) || (access & Address::kStaticStruct);
        ArrayTyped* array_typed = dynamic_cast<ArrayTyped*>(inst->fType);
        
        if (array_typed && array_typed->fSize > 1) {
            if (is_struct) {
                // Array is allocated in iZone/fZone until fExternalMemory reaches 0
                // kStaticStruct are always allocated in kExternal
                if ((access & Address::kStaticStruct) || (fExternalMemory > 0 && array_typed->fSize > fDLThreshold)) {
                    fFieldTable.push_back(make_pair(name, MemoryDesc(fFieldIndex++,
                                                                     getStructSize(),
                                                                     getStructIntSize(),
                                                                     getStructRealSize(),
                                                                     array_typed->fSize,
                                                                     array_typed->fType->getType(),
                                                                     MemoryDesc::kExternal)));
                    
                    if (array_typed->fType->getType() == Typed::kInt32) {
                        fStructIntOffset += array_typed->getSizeBytes();
                    } else {
                        fStructRealOffset += array_typed->getSizeBytes();
                    }
                    fExternalMemory -= array_typed->getSizeBytes();
                } else {
                    // Keep arrays in local struct memory
                    fFieldTable.push_back(make_pair(name, MemoryDesc(fFieldIndex++,
                                                                     getStructSize(),
                                                                     getStructIntSize(),
                                                                     getStructRealSize(),
                                                                     array_typed->fSize,
                                                                     array_typed->fType->getType(),
                                                                     MemoryDesc::kLocal)));
                    
                }
            } else {
                // Should never happen...
                faustassert(false);
            }
        } else {
            if (is_struct) {
                fFieldTable.push_back(make_pair(name, MemoryDesc(fFieldIndex++,
                                                                 getStructSize(),
                                                                 getStructIntSize(),
                                                                 getStructRealSize(),
                                                                 1,
                                                                 inst->fType->getType(),
                                                                 MemoryDesc::kLocal)));
                // Scalar variable always stay in local struct memory (TO CHECK)
            } else {
                // Local variables declared by [var_num, type] pairs
            }
        }
    
        if (inst->fValue) getMemoryDesc(inst->getName()).fWAccessCount++;
        DispatchVisitor::visit(inst);
    }
    
};

#endif
