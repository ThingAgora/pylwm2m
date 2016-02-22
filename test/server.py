#!/usr/bin/env python

#
# Copyright (c) 2016, Luc Yriarte
# BSD License <http://www.opensource.org/licenses/bsd-license.php>
#
# Author:
# Luc Yriarte <luc.yriarte@thingagora.org>
#

import sys, time, socket, re
from lwm2m import *

MAXTIMESTEP = 30
BUFFER_SIZE = 1024
SERVER_HOST = "::1"
SERVER_PORT = 5683

COAP_201_CREATED = 0x41
COAP_202_DELETED = 0x42

LWM2M_CONTENT_TEXT = 0
LWM2M_CONTENT_JSON = 1543


#### #### #### #### #### #### #### #### #### #### #### #### #### #### #### ####

def bufferSendCallback(sessionH, buffer, length, userData):
	sent = 0
	print "Sending", length, "bytes to", sessionH
	while(sent < length):
		sent += userData.socket.sendto(buffer[sent:length], sessionH)
	return 0

def monitoringCallback(clientID, uri, status, format, data, dataLength, userData):
	if (status == COAP_201_CREATED):
		print "New client #" + str(clientID) + " registered."
		userData.clients.append(clientID)
		userData.dumpClient(clientID)
	if (status == COAP_202_DELETED):
		print "Client #" + str(clientID) + " unregistered."
		userData.clients.pop(userData.clients.index(clientID))
	print "\n"
	return None

def resultCallback(clientID, uri, status, format, data, dataLength, userData):
	print "\n---- resultCallback"
	print "\tclientID: ", clientID
	print "\turi: ", uri
	print "\tstatus: ", status
	print "\tformat: ", format
	print "\tdataLength: ", dataLength
	if (format in [LWM2M_CONTENT_TEXT, LWM2M_CONTENT_JSON]):
		print data, "\n"
	return None


#### #### #### #### #### #### #### #### #### #### #### #### #### #### #### ####

class Lwm2mServer():
	
	def help(self, command = None):
		if command:
			print self.commands[command]
		else:
			for key in self.commands.keys():
				print key, "\t", self.commands[key]
	
	def read(self, clientID, uri):
		result = lwm2m_dm_read(self.lwm2mH, int(clientID), uri, resultCallback, self)
		return result
	
	def list(self):
		for clientID in self.clients:
			self.dumpClient(clientID)
		return None
	
	def write(self, clientID, uri, data):
		result = lwm2m_dm_write(self.lwm2mH, int(clientID), uri, LWM2M_CONTENT_TEXT, data, len(data), resultCallback, self)
		return result
	
	def execute(self, clientID, uri, data):
		result = lwm2m_dm_execute(self.lwm2mH, int(clientID), uri, LWM2M_CONTENT_TEXT, data, len(data), resultCallback, self)
		return result
	
	def create(self, clientID, uri, data):
		result = lwm2m_dm_create(self.lwm2mH, int(clientID), uri, LWM2M_CONTENT_TEXT, data, len(data), resultCallback, self)
		return result
	
	def delete(self, clientID, uri):
		result = lwm2m_dm_delete(self.lwm2mH, int(clientID), uri, resultCallback, self)
		return result
	
	def __init__(self, host, port):
		self.commands = {
			'help' : "Help on available commands.",
			'list' : "List registered clients.",
			'read' : "Read from a client.",
			'write' : "Write to a client.",
			'execute' : "Execute a client resource.",
			'create' : "Create an Object instance.",
			'delete' : "Delete a client Object instance."
		}
		self.clients = []
		self.addresses = []
		self.timeStep = int(time.time())
		af, socktype, proto, canonname, address = socket.getaddrinfo(
			host, port, socket.AF_UNSPEC, socket.SOCK_DGRAM, 0, socket.AI_PASSIVE)[0]
		print "Started server address", address[0], "port", address[1]
		self.socket = socket.socket(af, socktype, proto)
		self.socket.bind(address)
		self.lwm2mH = lwm2m_init(None, bufferSendCallback, self)
		lwm2m_set_monitoring_callback(self.lwm2mH, monitoringCallback, self)

	def dumpClient(self, clientID):
		clientInfo = lwm2m_get_client_info(self.lwm2mH, clientID)
		print "Client #" + str(clientID) + ":"
		print "\tname: \"" + str(clientInfo[0]) + "\""
		print "\tobjects: " + ", ".join(clientInfo[1].split(" "))
		return clientInfo
		
	def handleCommand(self, commandLine):
		commandList = re.split('[ ]*',commandLine)
		try:
			self.commands.keys().index(commandList[0])
		except ValueError:
			print "Unknown command:", commandLine
			commandList = ['help']
		result = None
		try:
			result = getattr(self,commandList[0])(*commandList[1:])
		except Exception, e:
			print "Error:", str(e)
		return result

	def mainLoop(self):
		while(True):
			timeStepDelta = lwm2m_step(self.lwm2mH) - self.timeStep
			if (timeStepDelta < 0 or timeStepDelta > MAXTIMESTEP):
				timeStepDelta = MAXTIMESTEP
			self.timeStep += timeStepDelta
			self.socket.settimeout(timeStepDelta)
			data, address = (None, None)
			try:
				data, address = self.socket.recvfrom(BUFFER_SIZE)
			except KeyboardInterrupt:
				commandLine = raw_input("\n> ")
				if (commandLine):
					self.handleCommand(commandLine)
				else:
					break
			except socket.timeout:
				pass
			if (data and address):
				try:
					iAddr = self.addresses.index(address)
					address = self.addresses[iAddr]
				except ValueError:
					self.addresses.append(address)
				print len(data), "bytes received from", address
				lwm2m_handle_packet(self.lwm2mH, data, len(data), address)
		self.socket.close()

	def close(self):
		lwm2m_close(self.lwm2mH)


#### #### #### #### #### #### #### #### #### #### #### #### #### #### #### ####

if len(sys.argv) > 3:
	print "Usage:", sys.argv[0], "[ip] [port]"
	exit()

if len(sys.argv) > 1:
	SERVER_HOST = sys.argv[1]

if len(sys.argv) > 2:
	SERVER_PORT = sys.argv[2]


server = Lwm2mServer(SERVER_HOST, SERVER_PORT)
server.mainLoop()
server.close()
