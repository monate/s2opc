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

/** \file
 *
 * \brief Contains the constants used by the Tookit. Those constants are not intended to be modified.
 *
 */

#ifndef SOPC_TOOLKIT_CONSTANTS_H_
#define SOPC_TOOLKIT_CONSTANTS_H_

#include "sopc_toolkit_config_constants.h"

/* SOPC return statuses */
typedef enum SOPC_ReturnStatus
{
    SOPC_STATUS_OK = 0,
    SOPC_STATUS_NOK,
    SOPC_STATUS_INVALID_PARAMETERS,
    SOPC_STATUS_INVALID_STATE,
    SOPC_STATUS_ENCODING_ERROR,
    SOPC_STATUS_WOULD_BLOCK,
    SOPC_STATUS_TIMEOUT,
    SOPC_STATUS_OUT_OF_MEMORY,
    SOPC_STATUS_CLOSED,
    SOPC_STATUS_NOT_SUPPORTED
} SOPC_ReturnStatus;

#endif /* SOPC_TOOLKIT_CONSTANTS_H_ */
