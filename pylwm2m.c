/*
 * Copyright (c) 2016, Luc Yriarte
 * BSD License <http://www.opensource.org/licenses/bsd-license.php>
 * 
 * Author:
 * Luc Yriarte <luc.yriarte@thingagora.org>
 */

#include <stdlib.h>
#include <stdio.h>
#include <Python.h>

#include "pylwm2m.h"



/*
 * utilities
 */ 

static void prv_lwm2m_context_cleanup(pylwm2m_context_t * pylwm2mH) {
TRACE("%s %p\n",__FUNCTION__, pylwm2mH);
	int i;
	if (pylwm2mH->lwm2mH)
		lwm2m_close(pylwm2mH->lwm2mH);
	Py_XDECREF(pylwm2mH->connectCb);
	Py_XDECREF(pylwm2mH->bufferSendCb);
	Py_XDECREF(pylwm2mH->userData);
	Py_XDECREF(pylwm2mH->monitoringResult.resultCb);
	Py_XDECREF(pylwm2mH->monitoringResult.resultData);
	for (i=0; i<pylwm2mH->objectCount; i++) {
		Py_XDECREF(pylwm2mH->pylwm2mObjHTable[i]->readCb);
		Py_XDECREF(pylwm2mH->pylwm2mObjHTable[i]->writeCb);
		Py_XDECREF(pylwm2mH->pylwm2mObjHTable[i]->executeCb);
		Py_XDECREF(pylwm2mH->pylwm2mObjHTable[i]->createCb);
		Py_XDECREF(pylwm2mH->pylwm2mObjHTable[i]->deleteCb);
		Py_XDECREF(pylwm2mH->pylwm2mObjHTable[i]->userData);
		if (pylwm2mH->pylwm2mObjHTable[i]->lwm2mObjH->instanceIDs)
			free(pylwm2mH->pylwm2mObjHTable[i]->lwm2mObjH->instanceIDs);
		free(pylwm2mH->pylwm2mObjHTable[i]->lwm2mObjH);
		free(pylwm2mH->pylwm2mObjHTable[i]);
	}
	if (pylwm2mH->pylwm2mObjHTable)
		free(pylwm2mH->pylwm2mObjHTable);
	free(pylwm2mH);
}

int prv_lwm2m_uri_dump(lwm2m_uri_t * uriP, char * uriBuf) {
TRACE("%s %p %p\n",__FUNCTION__, uriP, uriBuf);
	uriBuf[0] = '\0';
	if (uriP) {
		sprintf(uriBuf,"/%u",(unsigned int)uriP->objectId);
		if (LWM2M_URI_IS_SET_INSTANCE(uriP)) {
			sprintf(uriBuf+strlen(uriBuf),"/%u",(unsigned int)uriP->instanceId);
			if (LWM2M_URI_IS_SET_RESOURCE(uriP)) {
				sprintf(uriBuf+strlen(uriBuf),"/%u",(unsigned int)uriP->resourceId);
			}
		}
	}
	return strlen(uriBuf);
}


/*
 * comms callbacks
 */ 

uint8_t buffer_send_cb(void * sessionH, uint8_t * buffer, size_t length, void * userData) {
TRACE("%s %p %p %d %p\n",__FUNCTION__, sessionH, buffer, (int)length, userData);
	pylwm2m_context_t * pylwm2mH = (pylwm2m_context_t *) userData;
	uint8_t result = COAP_500_INTERNAL_SERVER_ERROR;
	PyObject *pyresult = NULL;
	PyObject *pysessionH = (PyObject *) sessionH;
	if (pylwm2mH && PyCallable_Check(pylwm2mH->bufferSendCb)) {
TRACE("%s callback %p Os#iO %p %s[%d] %d %p\n",__FUNCTION__, pylwm2mH->bufferSendCb, 
			pysessionH, buffer, (int)length, (int)length, pylwm2mH->userData);
		pyresult = PyObject_CallFunction(pylwm2mH->bufferSendCb, "Os#iO", 
			pysessionH, buffer, (int)length, (int)length, pylwm2mH->userData);
TRACE("%s pyresult: %p\n",__FUNCTION__,pyresult);
		if (pyresult) {
			result = (uint8_t) PyInt_AsLong(pyresult);
		}
		Py_XDECREF(pyresult);
	}
	return result;
}


/*
 * lwm2m API wrapper
 */ 

PyObject * pylwm2m_init(PyObject *self, PyObject *args) {
TRACE("%s %p %p\n",__FUNCTION__, self, args);
	pylwm2m_context_t * pylwm2mH = NULL;
	pylwm2mH = (pylwm2m_context_t *) malloc(sizeof(struct _pylwm2m_context_));
	memset(pylwm2mH,0,sizeof(struct _pylwm2m_context_));
	pylwm2mH->lwm2mH = lwm2m_init(NULL, buffer_send_cb, (void *)pylwm2mH);
	if (!pylwm2mH->lwm2mH || 
	!PyArg_ParseTuple(args, "OOO", &pylwm2mH->connectCb, &pylwm2mH->bufferSendCb, &pylwm2mH->userData)) {
		free(pylwm2mH);
		return NULL;
	}
	Py_XINCREF(pylwm2mH->connectCb);
	Py_XINCREF(pylwm2mH->bufferSendCb);
	Py_XINCREF(pylwm2mH->userData);
	return PyCapsule_New((void *) pylwm2mH, NULL, NULL);
}

PyObject * pylwm2m_close(PyObject *self, PyObject *args) {
TRACE("%s %p %p\n",__FUNCTION__, self, args);
	pylwm2m_context_t * pylwm2mH = NULL;
	PyObject *pylwm2mHCap = NULL;
	if (!PyArg_ParseTuple(args, "O", &pylwm2mHCap)) {
		return NULL;
	}
	pylwm2mH = PyCapsule_GetPointer(pylwm2mHCap, NULL);
	prv_lwm2m_context_cleanup(pylwm2mH);
	return Py_None;
}

PyObject * pylwm2m_step(PyObject *self, PyObject *args) {
TRACE("%s %p %p\n",__FUNCTION__, self, args);
	pylwm2m_context_t * pylwm2mH = NULL;
	PyObject *pylwm2mHCap = NULL;
	time_t timeout = (time_t) -1;
	if (!PyArg_ParseTuple(args, "O", &pylwm2mHCap)) {
		return NULL;
	}
	pylwm2mH = PyCapsule_GetPointer(pylwm2mHCap, NULL);
TRACE("lwm2m_step %p %d\n",pylwm2mH->lwm2mH, (int)timeout);
	lwm2m_step(pylwm2mH->lwm2mH, &timeout);
TRACE("lwm2m_step result: %d\n", (int)timeout);
	return Py_BuildValue("i", timeout);
}

PyObject * pylwm2m_handle_packet(PyObject *self, PyObject *args) {
TRACE("%s %p %p\n",__FUNCTION__, self, args);
	pylwm2m_context_t * pylwm2mH = NULL;
	PyObject *pylwm2mHCap = NULL;
	uint8_t *buffer = NULL;
	int length = -1;
	PyObject *fromSessionH = NULL;
	if (!PyArg_ParseTuple(args, "Os#iO", &pylwm2mHCap, &buffer, &length, &length, &fromSessionH)) {
		return NULL;
	}
	pylwm2mH = PyCapsule_GetPointer(pylwm2mHCap, NULL);
TRACE("lwm2m_handle_packet %p %p %d %p\n",pylwm2mH->lwm2mH, buffer, length, fromSessionH);
	lwm2m_handle_packet(pylwm2mH->lwm2mH, buffer, length, fromSessionH);
	return Py_None;
}


/*
 * Python module management
 */ 

static PyMethodDef lwm2mMethods[] = {
    {"lwm2m_init", pylwm2m_init, METH_VARARGS, 
		"lwm2m_init(connectCallback, bufferSendCallback, userData) - > handle"},
    {"lwm2m_close", pylwm2m_close, METH_VARARGS, 
		"lwm2m_close(handle)"},
    {"lwm2m_step", pylwm2m_step, METH_VARARGS, 
		"lwm2m_step(handle) -> timeStep"},
    {"lwm2m_handle_packet", pylwm2m_handle_packet, METH_VARARGS, 
		"lwm2m_handle_packet(handle, data, length, address)"},
    {"lwm2m_set_monitoring_callback", pylwm2m_set_monitoring_callback, METH_VARARGS, 
		"lwm2m_set_monitoring_callback(handle, monitoringCallback, userData)"},
    {"lwm2m_get_client_info", pylwm2m_get_client_info, METH_VARARGS, 
		"lwm2m_get_client_info(handle, clientID) -> (name, uris)"},
    {"lwm2m_dm_read", pylwm2m_dm_read, METH_VARARGS, 
		"lwm2m_dm_read(handle, clientID, uriStr, resultCallback, userData) -> error"},
    {"lwm2m_dm_write", pylwm2m_dm_write, METH_VARARGS, 
		"lwm2m_dm_write(handle, clientID, uriStr, format, buffer, length, resultCallback, userData) -> error"},
    {"lwm2m_dm_execute", pylwm2m_dm_execute, METH_VARARGS, 
		"lwm2m_dm_execute(handle, clientID, uriStr, format, buffer, length, resultCallback, userData) -> error"},
    {"lwm2m_dm_create", pylwm2m_dm_create, METH_VARARGS, 
		"lwm2m_dm_create(handle, clientID, uriStr, format, buffer, length, resultCallback, userData) -> error"},
    {"lwm2m_dm_delete", pylwm2m_dm_delete, METH_VARARGS, 
		"lwm2m_dm_delete(handle, clientID, uriStr, resultCallback, userData) -> error"},
    {"lwm2m_observe", pylwm2m_observe, METH_VARARGS, 
		NULL},
    {"lwm2m_observe_cancel", pylwm2m_observe_cancel, METH_VARARGS, 
		NULL},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initlwm2m(void)
{
    (void) Py_InitModule("lwm2m", lwm2mMethods);
}
