/*
 * Licensed to Systerel under one or more contributor license
 * agreements. See the NOTICE file distributed with this work
 * for additional information regarding copyright ownership.
 * Systerel licenses this file to you under the Apache
 * License, Version 2.0 (the "License"); you may not use this
 * file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef SOPC_HELPER_URI_H_
#define SOPC_HELPER_URI_H_
#define TCP_UA_MAX_URL_LENGTH 4096

#include <stdbool.h>

typedef enum SOPC_UriType
{
    SOPC_URI_Undetermined,
    SOPC_URI_TcpUa,
    SOPC_URI_UdpUa,
    SOPC_URI_EthUa,
    SOPC_URI_MqttUa
} SOPC_UriType;

/* Return true if find exist in sep array, return false otherwise */
static bool URI_match(char find, const char* sep);

/* Extract respectively Port, Hostname and prefix from given URI */
static bool getUriPortId(const char** ppCursor, char** ppPort);
static bool getUriHostname(const char** ppCursor, char** ppHostname);
static bool getUriPrefix(const char** ppCursor, char** ppPrefix);

/*  */
static bool getUriTypeFromEnum(char** prefix, SOPC_UriType* type);

bool SOPC_Helper_URI_SplitUri(const char* uri, SOPC_UriType* type, char** hostname, char** port);

#endif /* SOPC_HELPER_URI_H_ */
