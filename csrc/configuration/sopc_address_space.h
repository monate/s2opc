/*
 *  Copyright (C) 2018 Systerel and others.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SOPC_ADDRESS_SPACE_H_
#define SOPC_ADDRESS_SPACE_H_

#include <stdint.h>

#include "sopc_array.h"
#include "sopc_dict.h"
#include "sopc_types.h"

#define FOR_EACH_ELEMENT_TYPE(x, extra)                                                                               \
    x(DataType, data_type, extra) x(Method, method, extra) x(Object, object, extra) x(ObjectType, object_type, extra) \
        x(ReferenceType, reference_type, extra) x(Variable, variable, extra) x(VariableType, variable_type, extra)    \
            x(View, view, extra)

#define ELEMENT_ATTRIBUTE_GETTER_DECL(ret_type, lowercase_name, camel_case_name) \
    ret_type* current_element_##lowercase_name(SOPC_AddressSpace_Item* item);

typedef struct
{
    OpcUa_NodeClass node_class;
    SOPC_StatusCode value_status;
    union {
        OpcUa_DataTypeNode data_type;
        OpcUa_MethodNode method;
        OpcUa_ObjectNode object;
        OpcUa_ObjectTypeNode object_type;
        OpcUa_ReferenceTypeNode reference_type;
        OpcUa_VariableNode variable;
        OpcUa_VariableTypeNode variable_type;
        OpcUa_ViewNode view;
    } data;
} SOPC_AddressSpace_Item;

/* Address space structure */
/* Maps NodeId to SOPC_AddressSpace_Item */
typedef SOPC_Dict SOPC_AddressSpace;

void SOPC_AddressSpace_Item_Initialize(SOPC_AddressSpace_Item* item, uint32_t element_type);

SOPC_NodeId* SOPC_AddressSpace_Item_Get_NodeId(SOPC_AddressSpace_Item* item);
SOPC_QualifiedName* SOPC_AddressSpace_Item_Get_BrowseName(SOPC_AddressSpace_Item* item);
SOPC_LocalizedText* SOPC_AddressSpace_Item_Get_DisplayName(SOPC_AddressSpace_Item* item);
SOPC_LocalizedText* SOPC_AddressSpace_Item_Get_Description(SOPC_AddressSpace_Item* item);
int32_t* SOPC_AddressSpace_Item_Get_NoOfReferences(SOPC_AddressSpace_Item* item);
OpcUa_ReferenceNode** SOPC_AddressSpace_Item_Get_References(SOPC_AddressSpace_Item* item);
SOPC_Variant* SOPC_AddressSpace_Item_Get_Value(SOPC_AddressSpace_Item* item);

void SOPC_AddressSpace_Item_Clear(SOPC_AddressSpace_Item* item);

SOPC_AddressSpace* SOPC_AddressSpace_Create(bool free_items);
SOPC_ReturnStatus SOPC_AddressSpace_Append(SOPC_AddressSpace* space, SOPC_AddressSpace_Item* item);
void SOPC_AddressSpace_Delete(SOPC_AddressSpace* space);

#endif /* SOPC_ADDRESS_SPACE_H_ */