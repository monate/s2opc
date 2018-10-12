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


import time

from _pys2opc import ffi, lib as libsub


class BaseConnectionHandler:
    """
    Base class giving the prototypes of the callbacks,
    and implements the subscription-library connection wrappers.

    The class supports Python's "with" statements.
    In this case, the connection is automatically closed upon exit of the context.
    """
    def __init__(self, id, configuration):
        self._id = id
        self.configuration = configuration
        self._dRequestContexts = {}  # Stores requests by their context {requestContext: Request()}
        self._dPendingResponses = {}  # Stores available responses {requestContext: Response()}. See get_response()
        self._sSkipResponse = set()  # Stores the requestContext of Responses that shall not be stored in _dequeResponses.
        self._connected = True
        self._dSubscription = {}  # Associates data_id to string NodeId

    # Internals
    def _on_datachanged(self, dataId, value):
        """
        Internal wrapper, calls on_datachanged() with a string NodeId.
        """
        assert dataId in self._dSubscription, 'Data change notification on unknown NodeId'
        self.on_datachanged(self._dSubscription[dataId], value)

    def _on_response(self, event, status, response, responseContext):
        """
        Receive a Response, associates it to a Request both-ways.
        Shall be called for every response received through callback_generic_event.
        """
        assert responseContext in self._dRequestContexts, 'Unknown requestContext {}.'.format(responseContext)
        request = self._dRequestContexts[responseContext]
        response.timestampReceived = time.time()
        request.response = response
        response.request = request
        if responseContext not in self._sSkipResponse:
            self.on_generic_response(response)
        else:
            del self._sSkipResponse[responseContext]
        request.eventResponseReceived.set()
    def _wait_for_response(self, request):
        request.eventResponseReceived.wait()
        return request.response
    def __enter__(self):
        return self
    def __exit__(self, *exc):
        self.disconnect()

    # Callbacks
    def on_datachanged(self, nodeId, dataValue):
        """
        dataValue is copied from the SOPC_DataValue: it contains the value,
        the source and server timestamps if available, and the status code.
        """
        raise NotImplementedError
    def on_disconnect(self):
        """
        Called when the disconnection of this connection is effective.
        """
        self._connected = False

    def on_generic_response(self, request, response):
        """
        This callback is called when the class receives a Response that is not waited upon.
        It is possible to not override it.

        The default implementation of this method stores the response in a double-end-queue
        which tracks available responses (see pop_response).
        It is possible to not call the on_generic_response of the parent class.
        """
        self._dequeResponses.append(response)

    # Disconnection
    def disconnect(self):
        """
        Disconnects the current connexion, and release its resources.
        Returns True if the disconnection was successful.
        """
        # The Toolkit will still call the on_disconnect() callback afterwards.
        status = libsub.SOPC_LibSub_Disconnect(self._id)
        return status == libsub.SOPC_STATUS_OK

    @property
    def connected(self):
        return self._connected

    # Subscription
    def add_nodes_to_subscription(self, nodeIds):
        """
        Subscribe to a list of string of NodeIds in the OPC-UA format:
        - "i=42" for an integer NodeId,
        - "s=Foobar" for a string NodeId,
        - "g=C496578A-0DFE-4b8f-870A-745238C6AEAE" for a GUID-NodeId,
        - "b=Barbar" for a ByteString.
        The string can be prepend by a "ns={};" which specifies the namespace index.
        """
        # TODO: check format?
        if nodeIds:
            n = len(nodeIds)
            lszNodeIds = [ffi.new('char[]', nid.encode()) for nid in nodeIds]
            lAttrIds = ffi.new('SOPC_LibSub_AttributeId[{}]'.format(n), [libsub.SOPC_LibSub_AttributeId_Value for _ in nodeIds])
            lDataIds = ffi.new('SOPC_LibSub_DataId[{}]'.format(n))
            status = libsub.SOPC_LibSub_AddToSubscription(self._id, lszNodeIds, lAttrIds, n, lDataIds)
            assert status == libsub.SOPC_STATUS_OK, 'Add to subscription failed with status {}'.format(status)
            for i, nid in zip(lDataIds, nodeIds):
                assert i not in self._dSubscription, 'data_id returned by Toolkit is already associated to a NodeId.'
                self._dSubscription[i] = nid

    # Generic request sender
    def send_generic_request(self, request, bWaitResponse):
        """
        Sends a request. When bWaitResponse:
        - waits for the response and returns it,
        - otherwise, returns the request, and the response will be available through get_response().
        """
        reqCtx = request.requestContext
        self._dRequestContexts[reqCtx] = request
        request.timestampSent = time.time()
        status = libsub.SOPC_LibSub_AsyncSendRequestOnSession(self._id, request.payload, request.requestContext)
        if bWaitResponse:
            self._sSkipResponse.add(reqCtx)
            return self._wait_for_response(request)
        else:
            return request

    def get_response(self, request):
        """
        Pops the response to the request from the store of available response.
        Returns the response if there is an available response.
        Otherwise returns None.
        """
        return self._dPendingResponses.pop(request.requestContext, None)

    # Specialized request sender
    def read_nodes(self, nodeidsAttributes, bWaitResponse=False):
        """
        When bWaitResponse, waits for the response and returns it. Otherwise, returns the request.
        """
        request = ReadRequest(params)
        obj = self.send_generic_request(request, bWaitResponse=False)
        #if bWaitResponse:
        #    return zip(*zip(*nodeidsAttributes), response.values)
    def write_nodes(self, nodeidsAttributesValues, bWaitResponse=False):
        # TODO:
        request = WriteRequest(params)
        return self.send_generic_request(request, bWaitResponse=False)
    def browse_nodes(self, nodeids, bWaitResponse):
        request = BrowseRequest(params)
        return self.send_generic_request(request, bWaitResponse=False)
    def history_read_nodes(self, nodeids, bWaitResponse):
        request = HistoryReadRequest(params)
        return self.send_generic_request(request, bWaitResponse=False)
