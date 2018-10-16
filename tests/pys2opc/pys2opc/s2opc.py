#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Licensed to Systerel under one or more contributor license
# agreements. See the NOTICE file distributed with this work
# for additional information regarding copyright ownership.
# Systerel licenses this file to you under the Apache
# License, Version 2.0 (the "License"); you may not use this
# file except in compliance with the License. You may obtain
# a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.


from contextlib import contextmanager
import time

from _pys2opc import ffi, lib as libsub
from .connection import BaseConnectionHandler
from .types import DataValue
from .responses import Response


NULL = ffi.NULL
SOPC_STATUS_OK = libsub.SOPC_STATUS_OK

# TODO: make this configurable
LOG_LEVEL = libsub.SOPC_TOOLKIT_LOG_LEVEL_DEBUG

@ffi.def_extern()
def _callback_log(level, text):
    """
    Receives log information from the LibSub (not from the S2OPC toolkit).
    """
    dLevel = {libsub.SOPC_TOOLKIT_LOG_LEVEL_ERROR: '# Error: ',
              libsub.SOPC_TOOLKIT_LOG_LEVEL_WARNING: '# Warning: ',
              libsub.SOPC_TOOLKIT_LOG_LEVEL_INFO: '# Info: ',
              libsub.SOPC_TOOLKIT_LOG_LEVEL_DEBUG: '# Debug: '}
    if level <= LOG_LEVEL:
        print(dLevel[level] + ffi.string(text).decode())

@ffi.def_extern()
def _callback_disconnected(clientId):
    return PyS2OPC._callback_disconnected(clientId)

@ffi.def_extern()
def _callback_datachanged(connectionId, dataId, c_value):
    return PyS2OPC._callback_datachanged(connectionId, dataId, c_value)

@ffi.def_extern()
def _callback_generic_event(connectionId, event, status, responsePayload, responseContext):
    return PyS2OPC._callback_generic_event(connectionId, event, status, responsePayload, responseContext)



class PyS2OPC:
    """
    Wraps the C library. Stores the global status for this purpose.
    """
    _initialized = False
    _configured = False
    _dConfigurations = {}  # Stores known configurations {Id: configurationParameters}
    _dConnections = {}  # Stores known active connections {Id: ConnectionHandler()}

    @staticmethod
    def get_version():
        """Returns version string"""
        return ffi.string(libsub.SOPC_LibSub_GetVersion()).decode()

    @staticmethod
    @contextmanager
    def initialize():
        """
        Toolkit and LibSub initializations.
        Must be called exactly once per process.

        This function supports the context management:
        >>> with PyS2OPC.initialize():
        ...     # Do things here
        ...     pass

        When reaching out of the `with` statement, the Toolkit is automatically cleared.
        See clear().
        """
        assert not PyS2OPC._initialized, 'Library is alread initialized'

        status = libsub.SOPC_LibSub_Initialize([(libsub._callback_log, libsub._callback_disconnected)])
        assert status == SOPC_STATUS_OK, 'Library initialization failed with status code {}.'.format(status)
        PyS2OPC._initialized = True

        try:
            yield
        finally:
            PyS2OPC.clear()

    @staticmethod
    def clear():
        """
        Disconnect current servers and clears the Toolkit.
        Existing Configurations and Connections are then invalid and may be freed.
        """
        # TODO: Disconnect existing clients
        libsub.SOPC_LibSub_Clear()
        PyS2OPC._initialized = False

    @staticmethod
    def add_configuration_unsecured(*,
                                    server_url = 'opc.tcp://localhost:4841',
                                    publish_period = 500,
                                    n_max_keepalive = 3,
                                    n_max_lifetime = 10,
                                    timeout_ms = 10000,
                                    sc_lifetime = 3600000,
                                    token_target = 3):
        """
        Returns a configuration that can be later used in connect() or get_endpoints().

        Args:
            server_url: The endpoint and server url to connect to.
            publish_period: The period of the subscription, in ms.
            n_max_keepalive: The number of times the subscription has no notification to send before
                             sending an empty PublishResponse (the KeepAlive message). It is necessary
                             to keep `n_max_keepalive*timeout_ms*token_target < 5000ms`.
            n_max_lifetime: The maximum number of times a subscription has notifications to send
                            but no available token. In this case, the subscription is destroyed.
            timeout_ms: The connect() timeout, in ms.
            sc_lifetime: The target lifetime of the secure channel, before renewal, in ms.
            token_target: The number of subscription tokens (PublishRequest) that should be
                          made available to the server at anytime.
        """
        assert PyS2OPC._initialized and not PyS2OPC._configured,\
            'Toolkit is not initialized or already configured, cannot add new configurations.'

        pCfgId = ffi.new('SOPC_LibSub_ConfigurationId *')
        dConnectionParameters = {'server_url': ffi.new('char[]', server_url.encode()),
                                 'security_policy': libsub.SOPC_SecurityPolicy_None_URI,
                                 'security_mode': libsub.OpcUa_MessageSecurityMode_None,
                                 'disable_certificate_verification': True,
                                 'path_cert_auth': NULL,
                                 'path_cert_srv': NULL,
                                 'path_cert_cli': NULL,
                                 'path_key_cli': NULL,
                                 'path_crl': NULL,
                                 'policyId': ffi.new('char[]', b"anonymous"),
                                 'username': NULL,
                                 'password': NULL,
                                 'publish_period_ms': publish_period,
                                 'n_max_keepalive': n_max_keepalive,
                                 'n_max_lifetime': n_max_lifetime,
                                 'data_change_callback': libsub._callback_datachanged,
                                 'timeout_ms': timeout_ms,
                                 'sc_lifetime': sc_lifetime,
                                 'token_target': token_target,
                                 'generic_response_callback': libsub._callback_generic_event}
        #import pdb; pdb.set_trace()
        status = libsub.SOPC_LibSub_ConfigureConnection([dConnectionParameters], pCfgId)
        assert status == SOPC_STATUS_OK, 'Configuration failed with status {}.'.format(status)

        cfgId = pCfgId[0]
        config = ClientConfiguration(cfgId, dConnectionParameters)
        PyS2OPC._dConfigurations[cfgId] = config
        return config

    #def add_configuration_secured(parameters, security_parameters):


    @staticmethod
    def configured():
        """
        Must be called after all calls to add_configuration_unsecured() and add_configuration_secured(),
        and before connect() or get_endpoints().

        This tells S2OPC that the configuration phase is over.
        """
        assert PyS2OPC._initialized and not PyS2OPC._configured,\
            'Toolkit is not initialized or already configured, cannot add new configurations.'
        assert libsub.SOPC_LibSub_Configured() == libsub.SOPC_STATUS_OK
        PyS2OPC._configured = True


    @staticmethod
    def get_endpoints(configuration):
        """
        Optional call to fetch the endpoints of the server through the configuration.

        Not implemented yet.
        """
        pass


    @staticmethod
    def connect(configuration, ConnectionHandlerClass):
        """
        Connects to the server with the given `configuration`.
        The configuration is returned by a call to add_configuration_unsecured().
        The ConnectionHandlerClass is a class that must be inherited from BaseConnectionHandler,
        and that at least overrides the callbacks.
        It will be instantiated and the instance is returned.

        It can be optionally used in a `with` statement, which automatically disconnects the connection.
        """
        assert PyS2OPC._initialized and PyS2OPC._configured, 'Toolkit not configured, cannot connect().'
        assert isinstance(configuration, ClientConfiguration)
        cfgId = configuration._id
        assert cfgId in PyS2OPC._dConfigurations, 'Unknown configuration, see add_configuration_unsecured().'
        assert issubclass(ConnectionHandlerClass, BaseConnectionHandler)

        pConId = ffi.new('SOPC_LibSub_DataId *')
        status = libsub.SOPC_LibSub_Connect(cfgId, pConId)
        if status != libsub.SOPC_STATUS_OK:
            raise ConnectionError('Could not connect to the server with the given configuration.')

        connectionId = pConId[0]
        assert connectionId not in PyS2OPC._dConnections,\
            'Subscription library returned a connection id that is already taken by an active connection.'

        connection = ConnectionHandlerClass(connectionId, configuration)
        PyS2OPC._dConnections[connectionId] = connection
        return connection

    @staticmethod
    def _callback_disconnected(clientId):
        if clientId not in PyS2OPC._dConnections:
            print('Disconnected unknown client', clientId)
            return
        connection = PyS2OPC._dConnections.pop(clientId)
        connection.on_disconnect()

    @staticmethod
    def _callback_datachanged(connectionId, dataId, c_value):
        value = DataValue.from_sopc_libsub_value(c_value)
        #print('Data changed (connection {}, data_id {}), new value {}'.format(connection_id, data_id, value.value))
        assert connectionId in PyS2OPC._dConnections, 'Data change notification on unknown connection'
        connection = PyS2OPC._dConnections[connectionId]
        connection._on_datachanged(dataId, value)

    @staticmethod
    def _callback_generic_event(connectionId, event, status, responsePayload, responseContext):
        assert connectionId in PyS2OPC._dConnections, 'Event notification on unknown connection'
        connection = PyS2OPC._dConnections[connectionId]
        # It is not possible to store the payload, as it is freed by the caller of the callback.
        connection._on_response(event, status, responsePayload, responseContext)



class ClientConfiguration:
    """
    Stores configuration given to the subscription library.
    Such configurations should not be modified, as modifying these will have no impact on the Toolkit.
    """
    def __init__(self, id, parameters):
        self._id = id
        self._parameters = parameters

    @property
    def parameters(self):
        """
        Returns a copy of the parameters given to the configuration.
        Modifying these will have no effect on the configuration whatsoever.
        """
        return self._parameters.copy()

