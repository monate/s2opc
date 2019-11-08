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

 File Name            : session_channel_it.c

 Date                 : 08/11/2019 16:15:04

 C Translator Version : tradc Java V1.0 (14/03/2012)

******************************************************************************/

/*------------------------
   Exported Declarations
  ------------------------*/
#include "session_channel_it.h"

/*----------------------------
   CONCRETE_VARIABLES Clause
  ----------------------------*/
t_entier4 session_channel_it__channel_i;

/*------------------------
   INITIALISATION Clause
  ------------------------*/
void session_channel_it__INITIALISATION(void) {
   session_channel_it__channel_i = 0;
}

/*--------------------
   OPERATIONS Clause
  --------------------*/
void session_channel_it__init_iter_channel(
   t_bool * const session_channel_it__p_continue) {
   constants__get_card_t_channel(&session_channel_it__channel_i);
   *session_channel_it__p_continue = (1 <= session_channel_it__channel_i);
}

void session_channel_it__continue_iter_channel(
   t_bool * const session_channel_it__p_continue,
   constants__t_channel_i * const session_channel_it__p_channel) {
   constants__get_cast_t_channel(session_channel_it__channel_i,
      session_channel_it__p_channel);
   session_channel_it__channel_i = session_channel_it__channel_i -
      1;
   *session_channel_it__p_continue = (1 <= session_channel_it__channel_i);
}

