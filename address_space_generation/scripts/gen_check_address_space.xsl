<?xml version="1.0" encoding="UTF-8"?>
<!--
Copyright (C) 2018 Systerel and others.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
-->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
version="2.0" xmlns:ua="http://opcfoundation.org/UA/2011/03/UANodeSet.xsd"  xmlns:uax="http://opcfoundation.org/UA/2008/02/Types.xsd"  xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:sys="http://www.systerel.fr" >
    <xsl:output method="text"  encoding="UTF-8" />

<%
classes = ['View', 'Object', 'Variable', 'VariableType', 'ObjectType', 'ReferenceType', 'DataType', 'Method']
%>

<!-- create the constants for each class -->
% for i in range(1, 9):
<xsl:variable name="NB_${i}" select="count(//ua:UA${classes[i-1]})"/>
% endfor

<xsl:variable name="NB_REF" select="count(References/*)"/>

<xsl:variable name="alias" select="//ua:Alias"/>

<!-- copy node to be processed in a specific variable to manage the order of the objects -->
<xsl:variable name="ua_nodes">
    <xsl:for-each select="${ '|'.join(['*/ua:UA' + s for s  in classes])}">
        <xsl:sort select="sys:ord_class(.)"/>
        <xsl:apply-templates select="." mode="copy"/>
    </xsl:for-each>
</xsl:variable>

<xsl:variable name="nodeid_list" select="distinct-values($ua_nodes//@NodeId|$ua_nodes//ua:Reference/text()|$ua_nodes//ua:Reference/@ReferenceType)"/> <!--  -->
<xsl:variable name="nodeid_var_name">
    <xsl:for-each select="$nodeid_list">
        <n id="{.}" vn="nodeid_{position()}"/>
    </xsl:for-each>
</xsl:variable>

<!-- create a liste of name for each -->
<xsl:variable name="var_vartype" select="$ua_nodes/ua:UAVariable|$ua_nodes/ua:UAVariableType"/>

<xsl:template match="/">/*
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
 *  along with this program.  If not, see &lt;http://www.gnu.org/licenses/>.
 */

#pragma GCC diagnostic ignored "-Wunused-variable"

#include &lt;stdio.h>
#include &lt;string.h>

#include "sopc_toolkit_constants.h"
#include "sopc_builtintypes.h"
#include "sopc_types.h"
#include "sopc_user_app_itf.h"

extern SOPC_AddressSpace addressSpace;
#define DisplayName addressSpace.displayNameArray
#define DisplayName_begin addressSpace.displayNameIdxArray_begin
#define DisplayName_end addressSpace.displayNameIdxArray_end
#define Description addressSpace.descriptionArray
#define Description_begin addressSpace.descriptionIdxArray_begin
#define Description_end addressSpace.descriptionIdxArray_end
#define DEFAULT_VARIANT  {true, SOPC_Null_Id, SOPC_VariantArrayType_SingleValue,{0}}

bool test_browsename(void){
    bool bres = true;
    printf ("test BrowseName\n");
    const char *var;
    <xsl:apply-templates select = "$ua_nodes/*/@BrowseName" mode="qName"/>
    return bres;
}

bool test_value(void){
    printf ("test Value\n");
    bool bres = true;
    int pos;
    const char* nodeid;
    SOPC_Byte builtInTypeId;
    char *value_node;
    bool bool_value;
    signed char int8_value;
    unsigned char uint8_value;
    signed short int16_value;
    unsigned short uint16_value;
    signed long int32_value;
    unsigned long uint32_value;
    int long long int64_value;
    unsigned int long long uint64_value;
    double double_value;
    unsigned long date_time_value_32_lsb;
    unsigned long date_time_value_32_msb;
    float float_value;
    int status_value;
    <xsl:apply-templates select = "$ua_nodes/ua:UAVariable|$ua_nodes/ua:UAVariableType" mode="tValue"/>
    return bres;
}

bool test_reference(void){
    printf("test reference \n");
    bool bres = true;
    int pos;
    int res;
    int exp;
    const char* nodeid;

    <xsl:apply-templates select="$ua_nodes/*" mode="ref"/>
    return bres;
}

int compareLocalizedText(SOPC_LocalizedText LText, const char *text, const char *locale){
    int i = 0;
    if (strcmp((char*)LText.Locale.Data, locale) != 0){
        printf("locale KO");
        i++;
    }
    else{
        printf("locale ok");
    }
    if (strcmp((char*)LText.Text.Data, text) != 0){
        printf(", text KO");
        i++;
    }
    else{
        printf(", text ok");
    }
    return i;
}

% for s in ['Description', 'DisplayName']:

bool test_${s}(void){
    printf("test ${s}\n");
    bool bres = true;
    int pos;
    int res;
    int exp;
    const char* nodeid;
    <xsl:apply-templates select="$ua_nodes/*" mode="${s}"/>
    return bres;
}

% endfor

int main (void){
    bool bres = true;
    bres = test_browsename();
    bres = bres &amp;&amp; test_value();
    bres = bres &amp;&amp; test_reference();
    bres = bres &amp;&amp; test_Description();
    bres = bres &amp;&amp; test_DisplayName();
    if(bres == false){
        return 1;
    }else{
        return 0;
    }
}

</xsl:template>


% for s in ['Description', 'DisplayName']:
<xsl:template match="*" mode="${s}">
    <xsl:variable name="nodeId" select="@NodeId"/>
    nodeid = "<xsl:value-of select="@NodeId"/>";
    pos = <xsl:value-of select="position()"/>;
    exp = <xsl:value-of select="count(./ua:${s})"/>;
    res = ${s}_end[pos] - ${s}_begin[pos] + 1;
    if (res != exp){
        bres = false;
        printf("Invalid number of ${s} expected %d result %d : pos %d nodeid %s\n", exp, res, pos, nodeid);
    }
    else{
        <xsl:for-each select="./ua:${s}">
            printf("test ${s} %d node %d nodeid %s : ", <xsl:value-of select="position()"/>, pos, nodeid);
            compareLocalizedText(${s}[${s}_begin[pos] + <xsl:value-of select="position()"/> -1], "<xsl:value-of select="./text()"/>", "<xsl:value-of select='@Locale'/>");
            printf("\n");
        </xsl:for-each>
    }

</xsl:template>
% endfor

<!-- compare node id -->

<xsl:template name="cmp_nodeId">
    <xsl:param name="expected"/>
    <xsl:param name="result"/>
{
SOPC_NodeId* res = <xsl:value-of select="$result"/>;

    <xsl:analyze-string select="$expected" regex="(ns=\d*;)?(i=\d*|s=.*)">
        <xsl:matching-substring>
int nsIndex = <xsl:value-of select="if (regex-group(1)) then substring-after(substring-before(regex-group(1),';'),'=') else 0"/>;
        <xsl:variable name="ident" select="regex-group(2)"/>
        <xsl:choose>
            <xsl:when test="starts-with($ident, 'i')">
    if (res-> IdentifierType != SOPC_IdentifierType_Numeric ||
        res-> Namespace != nsIndex ||
        res-> Data.Numeric != <xsl:value-of select="substring-after($ident,'=')"/>){printf ("Invalid nodeId expected (%d, i, %d) result (%d,%d,%d) for %s \n", nsIndex, <xsl:value-of select="substring-after($ident,'=')"/>, res-> Namespace, res-> IdentifierType, res-> Data.Numeric, nodeid);}
            </xsl:when>
            <xsl:when test="starts-with($ident, 's')">
    if (res-> IdentifierType != SOPC_IdentifierType_String ||
        res-> Namespace != nsIndex ||
        strcmp((char*)res-> Data.String.Data, "<xsl:value-of select="substring-after($ident,'=')"/>")!=0) {printf ("Invalid nodeId expected <xsl:value-of select="substring-after($ident,'=')"/> for %s \n" , nodeid);}
            </xsl:when>
            <xsl:when test="starts-with($ident, 'g')">
    if (res-> IdentifierType != SOPC_IdentifierType_Guid ||
        res-> Namespace != nsIndex ) {printf ("Invalid nodeId %s", nodeid);}

            </xsl:when>
            <xsl:when test="starts-with($ident, 'b')">
    if (res-> IdentifierType != SOPC_IdentifierType_ByteString ||
        res-> Namespace != nsIndex ) {printf ("Invalid nodeId %s", nodeid);}
            </xsl:when>
            <xsl:otherwise>
                <xsl:message terminate="yes">Unknown identifier type : '<xsl:value-of select="$ident"/>'.</xsl:message>
            </xsl:otherwise>
        </xsl:choose>
        </xsl:matching-substring>
        <xsl:non-matching-substring>
            <xsl:message  terminate="yes">'<xsl:value-of select="$expected"/>' invalid node id</xsl:message>
        </xsl:non-matching-substring>
    </xsl:analyze-string><xsl:text/>
}
</xsl:template>

<xsl:template match="*" mode="desc">
    <xsl:variable name="nodeId" select="@NodeId"/>
    nodeid = "<xsl:value-of select="@NodeId"/>";
    pos = <xsl:value-of select="position()"/>;
    exp = <xsl:value-of select="count(./ua:Description)"/>;
    res = addressSpace.descriptionIdxArray_end[pos] - addressSpace.descriptionIdxArray_begin[pos] + 1;
    if (res != exp){
        bres = false;
        printf("Invalid number of description expected %d result %d : nodeid %s\n", exp, res, nodeid);
    }
    else{
        <xsl:for-each select="./ua:Description">
            printf("test description %d node %d nodeid %s : ", <xsl:value-of select="position()"/>, pos, nodeid);
            compareLocalizedText(addressSpace.descriptionArray[addressSpace.descriptionIdxArray_begin[pos] + <xsl:value-of select="position()"/> -1], "<xsl:value-of select="./text()"/>", "<xsl:value-of select='@Locale'/>");
            printf("\n");
        </xsl:for-each>
    }

</xsl:template>

<xsl:template match="*" mode="ref">
    <xsl:variable name="nodeId" select="@NodeId"/>
    nodeid = "<xsl:value-of select="@NodeId"/>";
    pos = <xsl:value-of select="position()"/>;
    exp = <xsl:value-of select="count(./ua:References/*)"/>;
    res = addressSpace.referenceIdxArray_end[pos] - addressSpace.referenceIdxArray_begin[pos] + 1 ;
    if (res != exp) {
        bres = false;
        printf("Invalid number of reference expected %d result %d : nodeid %s\n", exp, res, nodeid);
    }
    else{
        <xsl:for-each select="./ua:References/*">
            printf("test reference %d node %d nodeid %s\n",   <xsl:value-of select="position()"/>, pos, nodeid);
            <xsl:call-template name="cmp_nodeId">
                <xsl:with-param name="expected" select="@ReferenceType"/>
                <xsl:with-param name="result">addressSpace.referenceTypeArray[addressSpace.referenceIdxArray_begin[pos] + <xsl:value-of select="position()"/> -1 ]</xsl:with-param>
            </xsl:call-template>
            <xsl:call-template name="cmp_nodeId">
                <xsl:with-param name="expected" select="./text()"/>
                <xsl:with-param name="result">&amp;(addressSpace.referenceTargetArray[addressSpace.referenceIdxArray_begin[pos] + <xsl:value-of select="position()"/> -1 ])->NodeId</xsl:with-param>
            </xsl:call-template>
        </xsl:for-each>
    }

</xsl:template>

<xsl:template match="@*" mode="qName">
    <xsl:variable name="bn" select="."/>
    <xsl:variable name="pos" select="position()"/>
    <xsl:variable name="NodeId" select="../@NodeId"/>
    <xsl:analyze-string select="$bn" regex="(\d+:)?(.+)">
        <xsl:matching-substring>
        <xsl:variable name="nsIndex" select="if (regex-group(1)) then substring-before(regex-group(1),':') else 0"/>
${print_value('if (addressSpace.browseNameArray[%s].NamespaceIndex != %s) {printf("invalid BrowseName ") ;}  ',"$pos", "$nsIndex")}<xsl:text>
</xsl:text>
${print_value('var = "%s";', "regex-group(2)")}
${print_value('if (strcmp((char*)addressSpace.browseNameArray[%s].Name.Data, var)) {bres = false; printf("invalid BrowseName ") ;}  ',"$pos")}<xsl:text>
</xsl:text>
        </xsl:matching-substring>
        <xsl:non-matching-substring>
            <xsl:message><xsl:value-of select="$bn"/> invalid qualified string format</xsl:message>
        </xsl:non-matching-substring>
    </xsl:analyze-string>
</xsl:template>

<!-- template de test des value -->

<xsl:template match="ua:UAVariable[ua:Value]|ua:UAVariableType[ua:Value]" mode="tValue">
  pos = <xsl:value-of select="position()"/>;
  <xsl:apply-templates select="ua:Value/*" mode="tValue"/>
</xsl:template>

<xsl:template match="ua:UAVariable[not(ua:Value)]|ua:UAVariableType[not(ua:Value)]" mode="tValue">
  <xsl:variable name="NodeId" select="@NodeId"/>
  pos = <xsl:value-of select="position()"/>;
  nodeid = "<xsl:value-of select="@NodeId"/>";
  printf("test Value for nodeid %s\n", nodeid);
  if (!(addressSpace.valueArray[pos].BuiltInTypeId == SOPC_Null_Id)) {bres = false; printf("invalid BuiltInTypeId \n") ;}<xsl:text>
</xsl:text>
  if (!(addressSpace.valueArray[pos].ArrayType == SOPC_VariantArrayType_SingleValue)) {bres = false; printf("invalid Arraytype \n") ;}<xsl:text>
</xsl:text>
  if (!(addressSpace.valueArray[pos].Value.Boolean == 0)) {bres = false; printf("invalid Value \n") ;}<xsl:text>
</xsl:text>
</xsl:template>

% for s in ['Boolean', 'SByte', 'Byte', 'Int16', 'Int32', 'Int64', 'Double', 'NodeId', 'Status']:
<xsl:template match="uax:${s}" mode="tValue">
  <xsl:variable name="NodeId" select="../../@NodeId"/>
  nodeid = "<xsl:value-of select="../../@NodeId"/>";
  printf("test Value for nodeid %s\n", nodeid);
  value_node = "<xsl:value-of select="."/>";
  builtInTypeId = SOPC_${s}_Id;
  if (!(addressSpace.valueArray[pos].BuiltInTypeId == builtInTypeId)) {bres = false; printf("invalid BuiltInTypeId \n") ;}<xsl:text>
</xsl:text>
  if (!(addressSpace.valueArray[pos].ArrayType == SOPC_VariantArrayType_SingleValue)) {bres = false; printf("invalid Arraytype \n") ;}<xsl:text>
</xsl:text>
  if (builtInTypeId == SOPC_Boolean_Id) {
    if (!strcmp(value_node,"true")) {
      bool_value = true;
    }
    else {
      bool_value = false;
    }
    if (!(addressSpace.valueArray[pos].Value.Boolean == bool_value)) {bres = false; printf("invalid Value \n") ;}

  } else if (builtInTypeId == SOPC_SByte_Id) {
    sscanf(value_node, "%hhi", &amp;int8_value);
    if (!(addressSpace.valueArray[pos].Value.Sbyte == int8_value)) {printf("invalid Value \n") ;}<xsl:text>
    </xsl:text>
  } else if (builtInTypeId == SOPC_Byte_Id) {
    sscanf(value_node, "%hhu", &amp;uint8_value);
    if (!(addressSpace.valueArray[pos].Value.Byte == uint8_value)) {printf("invalid Value \n") ;}<xsl:text>
    </xsl:text>
  } else if (builtInTypeId == SOPC_Int16_Id) {
    sscanf(value_node, "%hi", &amp;int16_value);
    if (!(addressSpace.valueArray[pos].Value.Int16 == int16_value)) {printf("invalid Value \n") ;}<xsl:text>
    </xsl:text>
  }else if (builtInTypeId == SOPC_Int32_Id) {
    sscanf(value_node, "%li", &amp;int32_value);
    if (!(addressSpace.valueArray[pos].Value.Int32 == int32_value)) {printf("invalid Value \n") ;}<xsl:text>
    </xsl:text>
  }else if (builtInTypeId == SOPC_Int64_Id) {
    sscanf(value_node, "%lld", &amp;int64_value);
    if (!(addressSpace.valueArray[pos].Value.Int64 == int64_value)) {printf("invalid Value \n") ;}<xsl:text>
    </xsl:text>
  }else if (builtInTypeId == SOPC_Double_Id) {
    sscanf(value_node, "%lf", &amp;double_value);
    if (!(addressSpace.valueArray[pos].Value.Doublev == double_value)) {bres = false; printf("invalid Value \n") ;}<xsl:text>
    </xsl:text>
  }else if (builtInTypeId == SOPC_NodeId_Id) {
    if (!strcmp((char*)addressSpace.valueArray[pos].Value.NodeId->Data.String.Data, value_node)) {bres = false; printf("invalid Value \n") ;}<xsl:text>
    </xsl:text>
  }

</xsl:template>
% endfor

<xsl:template match="uax:UInt16" mode="tValue">
  <xsl:variable name="NodeId" select="../../@NodeId"/>
  nodeid = "<xsl:value-of select="../../@NodeId"/>";
  printf("test Value for nodeid %s\n", nodeid);
  value_node = "<xsl:value-of select="."/>";
  sscanf(value_node, "%hu", &amp;uint16_value);

  if (!(addressSpace.valueArray[pos].BuiltInTypeId == SOPC_UInt16_Id)) {bres = false; printf("invalid BuiltInTypeId \n") ;}<xsl:text>
</xsl:text>
  if (!(addressSpace.valueArray[pos].ArrayType == SOPC_VariantArrayType_SingleValue)) {bres = false; printf("invalid Arraytype \n") ;}<xsl:text>
</xsl:text>
  if (!(addressSpace.valueArray[pos].Value.Uint16 == uint16_value)) {printf("invalid Value \n") ;}<xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="uax:UInt32" mode="tValue">
  <xsl:variable name="NodeId" select="../../@NodeId"/>
  nodeid = "<xsl:value-of select="../../@NodeId"/>";
  printf("test Value for nodeid %s\n", nodeid);
  value_node = "<xsl:value-of select="."/>";
  sscanf(value_node, "%lu", &amp;uint32_value);

  if (!(addressSpace.valueArray[pos].BuiltInTypeId == SOPC_UInt32_Id)) {bres = false; printf("invalid BuiltInTypeId \n") ;}<xsl:text>
</xsl:text>
  if (!(addressSpace.valueArray[pos].ArrayType == SOPC_VariantArrayType_SingleValue)) {bres = false; printf("invalid Arraytype \n") ;}<xsl:text>
</xsl:text>
  if (!(addressSpace.valueArray[pos].Value.Uint32 == uint32_value)) {printf("invalid Value \n") ;}<xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="uax:UInt64" mode="tValue">
  <xsl:variable name="NodeId" select="../../@NodeId"/>
  nodeid = "<xsl:value-of select="../../@NodeId"/>";
  printf("test Value for nodeid %s\n", nodeid);
  value_node = "<xsl:value-of select="."/>";
  sscanf(value_node, "%llu", &amp;uint64_value);

  if (!(addressSpace.valueArray[pos].BuiltInTypeId == SOPC_UInt64_Id)) {bres = false; printf("invalid BuiltInTypeId \n") ;}<xsl:text>
</xsl:text>
  if (!(addressSpace.valueArray[pos].ArrayType == SOPC_VariantArrayType_SingleValue)) {bres = false; printf("invalid Arraytype \n") ;}<xsl:text>
</xsl:text>
  if (!(addressSpace.valueArray[pos].Value.Uint64 == uint64_value)) {printf("invalid Value \n") ;}<xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="uax:DateTime" mode="tValue">
  <xsl:variable name="NodeId" select="../../@NodeId"/>
  nodeid = "<xsl:value-of select="../../@NodeId"/>";
  printf("test Value for nodeid %s\n", nodeid);
  value_node = "<xsl:value-of select="."/>";
  unsigned long long date_time_value;
  sscanf(value_node, "%llu", &amp;date_time_value);
  date_time_value_32_lsb = date_time_value &amp; 0xFFFFFFFF;
  date_time_value_32_msb = date_time_value &gt;&gt; 32;

  if (!(addressSpace.valueArray[pos].BuiltInTypeId == SOPC_DateTime_Id)) {printf("invalid BuiltInTypeId \n") ;}<xsl:text>
</xsl:text>
  if (!(addressSpace.valueArray[pos].ArrayType == SOPC_VariantArrayType_SingleValue)) {printf("invalid Arraytype \n") ;}<xsl:text>
</xsl:text>
  if (!(addressSpace.valueArray[pos].Value.Date.Low32 == date_time_value_32_lsb)) {printf("invalid Value for lsb\n") ;}<xsl:text>
</xsl:text>
  if (!(addressSpace.valueArray[pos].Value.Date.High32 == date_time_value_32_msb)) {printf("invalid Value for msb\n") ;}<xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="uax:XmlElement" mode="tValue">
  <xsl:variable name="NodeId" select="../../@NodeId"/>
  nodeid = "<xsl:value-of select="../../@NodeId"/>";
  printf("test Value for nodeid %s\n", nodeid);
  value_node = <xsl:value-of select="."/>;

if (!(addressSpace.valueArray[pos].BuiltInTypeId == SOPC_XmlElement_Id)) {bres = false; printf("invalid BuiltInTypeId \n") ;}<xsl:text>
</xsl:text>
  if (!(addressSpace.valueArray[pos].ArrayType == SOPC_VariantArrayType_SingleValue)) {bres = false; printf("invalid Arraytype \n") ;}<xsl:text>
</xsl:text>
    if (strcmp((char*)addressSpace.valueArray[pos].Value.XmlElt.Data, value_node) != 0) {printf("invalid Value \n") ;}<xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="uax:Float" mode="tValue">
  <xsl:variable name="NodeId" select="../../@NodeId"/>
  nodeid = "<xsl:value-of select="../../@NodeId"/>";
  printf("test Value for nodeid %s\n", nodeid);
  value_node = "<xsl:value-of select="."/>";
  sscanf(value_node, "%f", &amp;float_value);
  if (!(addressSpace.valueArray[pos].BuiltInTypeId == SOPC_Float_Id)) {bres = false; printf("invalid BuiltInTypeId \n") ;}<xsl:text>
</xsl:text>
  if (!(addressSpace.valueArray[pos].ArrayType == SOPC_VariantArrayType_SingleValue)) {bres = false; printf("invalid Arraytype  \n") ;}<xsl:text>
</xsl:text>
  if (!(addressSpace.valueArray[pos].Value.Floatv == float_value)) {bres = false; printf("invalid Value  \n") ;}<xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="uax:String" mode="tValue">
  <xsl:variable name="NodeId" select="../../@NodeId"/>
  nodeid = "<xsl:value-of select="../../@NodeId"/>";
  printf("test Value for nodeid %s\n", nodeid);
  value_node = <xsl:value-of select="."/>;
  if (!(addressSpace.valueArray[pos].BuiltInTypeId == SOPC_String_Id)) {bres = false; printf("invalid BuiltInTypeId \n") ;}<xsl:text>
</xsl:text>
  if (!(addressSpace.valueArray[pos].ArrayType == SOPC_VariantArrayType_SingleValue)) {bres = false; printf("invalid Arraytype \n") ;}<xsl:text>
</xsl:text>
    if (strcmp((char*)addressSpace.valueArray[pos].Value.String.Data, value_node) != 0) {bres = false; printf("invalid Value \n") ;}<xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="uax:ByteString" mode="tValue">
  <xsl:variable name="NodeId" select="../../@NodeId"/>
  nodeid = "<xsl:value-of select="../../@NodeId"/>";
  printf("test Value for nodeid %s\n", nodeid);
  value_node = <xsl:value-of select="."/>;
  if (!(addressSpace.valueArray[pos].BuiltInTypeId == SOPC_ByteString_Id)) {bres = false; printf("invalid BuiltInTypeId \n") ;}<xsl:text>
</xsl:text>
  if (!(addressSpace.valueArray[pos].ArrayType == SOPC_VariantArrayType_SingleValue)) {bres = false; printf("invalid Arraytype \n") ;}<xsl:text>
</xsl:text>
    if (strcmp((char*)addressSpace.valueArray[pos].Value.Bstring.Data, value_node) != 0) {bres = false; printf("invalid Value \n") ;}<xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="uax:Status" mode="tValue">
  <xsl:variable name="NodeId" select="../../@NodeId"/>
  nodeid = "<xsl:value-of select="../../@NodeId"/>";
  printf("test Value for nodeid %s\n", nodeid);
  value_node = "<xsl:value-of select="."/>";
  sscanf(value_node, "%d", &amp;status_value);

  if (!(addressSpace.valueArray[pos].BuiltInTypeId == SOPC_StatusCode_Id)) {bres = false; printf("invalid BuiltInTypeId \n") ;}<xsl:text>
</xsl:text>
  if (!(addressSpace.valueArray[pos].ArrayType == SOPC_VariantArrayType_SingleValue)) {bres = false; printf("invalid Arraytype \n") ;}<xsl:text>
</xsl:text>
  if (!(addressSpace.valueArray[pos].Value.Status == status_value)) {printf("invalid Value \n") ;}<xsl:text>
</xsl:text>
</xsl:template>

<xsl:template name="write-string">
    <xsl:param name="value"/>${print_value('{%s,0,"%s"}',"string-length($value)","$value")}
</xsl:template>

<!-- templates de recopie des noeuds -->
<xsl:template match="@ReferenceType" mode="copy">
    <xsl:variable name="type" select="."/>
    <xsl:attribute name="ReferenceType"><xsl:value-of select="$alias[@Alias = $type]"/></xsl:attribute>
</xsl:template>

<xsl:template match="@* | node()" mode="copy">
	<xsl:copy>
		<xsl:apply-templates select="@* | node()" mode="copy"/>
	</xsl:copy>
</xsl:template>

<!-- generate two xsl functions.
First map a uanode xml element to a number allowing the sort of elements
Second map a uanode xml element to the enum node class. -->
% for (n, f, type) in [('ord_class', lambda x: x, 'integer'), ('get_enum_value', lambda x : 'OpcUa_NodeClass_'+ classes[x-1], 'string')]:
  <xsl:function name="sys:${n}" as="xsd:${type}">
     <xsl:param name="e"/>
    <xsl:variable name='ln' select="local-name($e)"/>
     <xsl:choose>
%   for i in range(1, 9):
	<xsl:when test="$ln='UA${classes[i-1]}'">${f(i)}</xsl:when>
%   endfor
	<xsl:otherwise>
	   <xsl:message terminate="yes">Unknown class : '<xsl:value-of select="$ln"/>'.</xsl:message>
	</xsl:otherwise>
     </xsl:choose>
  </xsl:function>
% endfor

<%def name="write_string(value)"><xsl:call-template name="write-string"><xsl:with-param name="value" select="${value}"/></xsl:call-template></%def>

<%def name="print_value(patt, *args)">
    <%doc>
    Function that apply the given template string to the
    result of the XPath queries and returns an output formatted string.
    @patt str: a format string
    @args str: a set of XPath expressions
    </%doc>
${("<xsl:text>"+patt+"</xsl:text>") % tuple(map(lambda x : '</xsl:text><xsl:value-of select="%s"/><xsl:text>' % x, args))}</%def>
</xsl:stylesheet>
