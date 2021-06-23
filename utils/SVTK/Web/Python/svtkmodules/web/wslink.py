r"""wslink is a module that extends any
wslink related classes for the purposes of svtkWeb.

"""

from __future__ import absolute_import, division, print_function

# import inspect, types, string, random, logging, six, json, re, base64
import json, base64, time

from twisted.python         import log
from twisted.internet       import reactor

from autobahn.twisted.websocket import WebSocketServerProtocol

from wslink import websocket
from wslink import register as exportRpc

from svtk.web import protocols
from svtk.svtkWebCore import svtkWebApplication

# =============================================================================
application = None

# =============================================================================
#
# Base class for svtkWeb ServerProtocol
#
# =============================================================================

class ServerProtocol(websocket.ServerProtocol):
    """
    Defines the core server protocol for svtkWeb. Adds support to
    marshall/unmarshall RPC callbacks that involve ServerManager proxies as
    arguments or return values.

    Applications typically don't use this class directly, but instead
    sub-class it and call self.registerVtkWebProtocol() with useful svtkWebProtocols.
    """

    def __init__(self):
        log.msg('Creating SP')
        self.setSharedObject("app", self.initApplication())
        websocket.ServerProtocol.__init__(self)

    def initApplication(self):
        """
        Let subclass optionally initialize a custom application in lieu
        of the default svtkWebApplication.
        """
        global application
        if not application:
            application = svtkWebApplication()
        return application

    def setApplication(self, application):
        self.setSharedObject("app", application)

    def getApplication(self):
        return self.getSharedObject("app")

    def registerVtkWebProtocol(self, protocol):
        self.registerLinkProtocol(protocol)

    def getVtkWebProtocols(self):
        return self.getLinkProtocols()
