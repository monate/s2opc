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

/******************************************************************************

 File Name            : service_mgr_1.h

 Date                 : 29/01/2019 09:56:41

 C Translator Version : tradc Java V1.0 (14/03/2012)

******************************************************************************/

#ifndef _service_mgr_1_h
#define _service_mgr_1_h

/*--------------------------
   Added by the Translator
  --------------------------*/
#include "b2c.h"

/*----------------------------
   CONCRETE_VARIABLES Clause
  ----------------------------*/
extern t_bool service_mgr_1__local_service_treatment_i;

/*------------------------
   INITIALISATION Clause
  ------------------------*/
extern void service_mgr_1__INITIALISATION(void);

/*--------------------
   OPERATIONS Clause
  --------------------*/
extern void service_mgr_1__is_local_service_treatment(
   t_bool * const service_mgr_1__bres);
extern void service_mgr_1__set_local_service_treatment(void);
extern void service_mgr_1__unset_local_service_treatment(void);

#endif
