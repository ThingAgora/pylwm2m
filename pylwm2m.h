/*
 * Copyright (c) 2016, Luc Yriarte
 * BSD License <http://www.opensource.org/licenses/bsd-license.php>
 * 
 * Author:
 * Luc Yriarte <luc.yriarte@thingagora.org>
 */

#include <liblwm2m.h>


/*
 * defines and macros
 */ 


#ifdef DEBUG_TRACE
#define TRACE(format, ...) printf(format, __VA_ARGS__)
#else
#define TRACE(format, ...) 
#endif

#define URI_BUF_SIZE 20
#define URI_SPLITTER ' '


/*
 * types and structures
 */ 

typedef struct _pylwm2m_result_ {
	PyObject		* resultCb;
	PyObject		* resultData;
} pylwm2m_result_t;

typedef struct _pylwm2m_object_ pylwm2m_object_t;

typedef struct _pylwm2m_context_ {
	lwm2m_context_t	* lwm2mH;
	PyObject		* connectCb;
	PyObject		* bufferSendCb;
	PyObject		* userData;
	pylwm2m_result_t	monitoringResult;
	int				objectCount;
	pylwm2m_object_t ** pylwm2mObjHTable;
} pylwm2m_context_t;

struct _pylwm2m_object_ {
	pylwm2m_context_t * pylwm2mH;
	lwm2m_object_t	* lwm2mObjH;
	PyObject		* readCb;
	PyObject		* writeCb;
	PyObject		* executeCb;
	PyObject		* createCb;
	PyObject		* deleteCb;
	PyObject		* userData;
};



/*
 * utilities
 */ 

int prv_lwm2m_uri_dump(lwm2m_uri_t * uriP, char * uriBuf);


/*
 * comms callbacks
 */ 

uint8_t buffer_send_cb(void * sessionH, uint8_t * buffer, size_t length, void * userData);


/*
 * server callbacks
 */ 

void result_cb(uint16_t clientID, lwm2m_uri_t * uriP, int status, lwm2m_media_type_t format, uint8_t * data, int dataLength, void * userData);

void result_cb_wrapper(uint16_t clientID, lwm2m_uri_t * uriP, int status, lwm2m_media_type_t format, uint8_t * data, int dataLength, void * userData);


/*
 * lwm2m API wrapper
 */ 

PyObject * pylwm2m_init(PyObject *self, PyObject *args);

PyObject * pylwm2m_close(PyObject *self, PyObject *args);

PyObject * pylwm2m_step(PyObject *self, PyObject *args);

PyObject * pylwm2m_handle_packet(PyObject *self, PyObject *args);


/*
 * lwm2m server API wrapper
 */ 

PyObject * pylwm2m_set_monitoring_callback(PyObject *self, PyObject *args);

PyObject * pylwm2m_get_client_info(PyObject *self, PyObject *args);

PyObject * pylwm2m_dm_read(PyObject *self, PyObject *args);

PyObject * pylwm2m_dm_write(PyObject *self, PyObject *args);

PyObject * pylwm2m_dm_execute(PyObject *self, PyObject *args);

PyObject * pylwm2m_dm_create(PyObject *self, PyObject *args);

PyObject * pylwm2m_dm_delete(PyObject *self, PyObject *args);

PyObject * pylwm2m_observe(PyObject *self, PyObject *args);

PyObject * pylwm2m_observe_cancel(PyObject *self, PyObject *args);

