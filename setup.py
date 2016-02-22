#!/usr/bin/env python

#
# Copyright (c) 2016, Luc Yriarte
# BSD License <http://www.opensource.org/licenses/bsd-license.php>
#
# Author:
# Luc Yriarte <luc.yriarte@thingagora.org>
#

from distutils.core import setup, Extension

lwm2m_folder = 'wakaama/'

lwm2m_core_sources = [
	'liblwm2m.c',
	'uri.c',
	'utils.c',
	'objects.c',
	'tlv.c',
	'list.c',
	'packet.c',
	'transaction.c',
	'registration.c',
	'bootstrap.c',
	'management.c',
	'observe.c',
	'json.c'
	]

lwm2m_core_ext_sources = [
	'er-coap-13/er-coap-13.c'
	]

lwm2m_platform_sources = [
	'platforms/Linux/platform.c'
	]

pylwm2m_sources = [
	'pylwm2m.c',
	'lwm2m_server_api.c'
	]

pylwm2m_sources.extend([lwm2m_folder + 'core/' + src for src in lwm2m_core_sources])
pylwm2m_sources.extend([lwm2m_folder + 'core/' + src for src in lwm2m_core_ext_sources])
pylwm2m_sources.extend([lwm2m_folder +  src for src in lwm2m_platform_sources])

lwm2m_module = Extension('lwm2m',
	define_macros = [('MAJOR_VERSION', '0'), 
		('MINOR_VERSION', '1'),
		('LWM2M_SERVER_MODE', '1'), 
		('WITH_LOGS', '1'), 
		('LWM2M_LITTLE_ENDIAN', '1')],
	include_dirs = ['/usr/include/python2.7',
		lwm2m_folder + '/core'],
	sources = pylwm2m_sources
	)

setup (name = 'lwm2m',
	version = '0.1',
	description = 'LightWeight M2M',
	ext_modules = [lwm2m_module])
