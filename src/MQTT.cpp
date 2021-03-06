/*//-------------------------------------------------------------------------------
 *  MQTT.cpp
 *
 *  Implementation file for MQTT Wrapper
 *
 *  Wrapper for Arduino written by Ingo Randolf during
 *	eTextiles Summercamp 2015.
 *
 *	This library is intended to be used with esp8266 modules.
 *
 *
 *  This class wraps a slightly modified version of mqtt
 *	for esp8266 written by Tuan PM.
 *	Original code: https://github.com/tuanpmt/esp_mqtt
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 //-------------------------------------------------------------------------------*/
#include "MQTT.h"

#include "user_interface.h"
#include "osapi.h"
#include "os_type.h"

//needed?
//#include <stdlib.h>
//extern "C" {
//#include "ets_sys.h"
//#include "os_type.h"
//#include "osapi.h"
//#include "mem.h"
//#include "user_interface.h"
//}


//------------------------------------------------------------------------------------
// mqtt internal callbacks
//------------------------------------------------------------------------------------
static void mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	
	MQTT* _this = (MQTT*)client->user_data;
	
	if (_this && _this->onMqttConnectedCb) {
		_this->onMqttConnectedCb();
	}
}

static void mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	
	MQTT* _this = (MQTT*)client->user_data;
	
	if (_this && _this->onMqttDisconnectedCb) {
		_this->onMqttDisconnectedCb();
	}
}

static void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	
	MQTT* _this = (MQTT*)client->user_data;
	
	if (_this && _this->onMqttPublishedCb) {
		_this->onMqttPublishedCb();
	}
}

static void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	MQTT_Client* client = (MQTT_Client*)args;
	
	MQTT* _this = (MQTT*)client->user_data;
	
	if (_this) {
		
		_this->_onMqttDataCb(topic, topic_len, data, data_len);
	}
}



//------------------------------------------------------------------------------------
// MQTT class implementation
//------------------------------------------------------------------------------------
MQTT::MQTT(const char* host, uint32 port,
		const char* client_id, const char* client_user, const char* client_pass, uint32_t keepAliveTime, uint8_t cleanSession,
		const char* will_topic, const char* will_msg, uint8_t will_qos, uint8_t will_retain) :
	onMqttConnectedCb(0)
	,onMqttDisconnectedCb(0)
	,onMqttPublishedCb(0)
	,onMqttDataCb(0)
	,onMqttDataRawCb(0)
{
	// init connections
	MQTT_InitConnection(&mqttClient, host, port);

	// init client
	MQTT_InitClient(&mqttClient, client_id, client_user, client_pass, keepAliveTime, cleanSession);

	// init LWT
	MQTT_InitLWT(&mqttClient, will_topic, will_msg, will_qos, will_retain);

	// set user data
	mqttClient.user_data = (void*)this;

	// setup callbacks
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);
}


MQTT::~MQTT()
{
	MQTT_Free(&mqttClient);
}


/*
 */
void MQTT::connect()
{
	MQTT_Connect(&mqttClient);
}

void MQTT::disconnect()
{
	MQTT_Disconnect(&mqttClient);
}

bool MQTT::isConnected()
{
	return (mqttClient.connState >= TCP_CONNECTED);
}

/*
 */
bool MQTT::publish(const char* topic, const char* buf, uint32_t buf_len, int qos, int retain)
{
	return MQTT_Publish(&mqttClient, topic, buf, buf_len, qos, retain);
}

bool MQTT::publish(String& topic, String& data, int qos, int retain)
{
	return publish(topic.c_str(), data.c_str(), data.length(), qos, retain);
}

bool MQTT::publish(String& topic, const char* buf, uint32_t buf_len, int qos, int retain)
{
	return publish(topic.c_str(), buf, buf_len, qos, retain);
}

bool MQTT::publish(const char* topic, String& data, int qos, int retain)
{
	return publish(topic, data.c_str(), data.length(), qos, retain);
}


/*
 */
bool MQTT::subscribe(const char* topic, uint8_t qos)
{
	return MQTT_Subscribe(&mqttClient, (char*)topic, qos);
}

bool MQTT::subscribe(const String& topic, uint8_t qos)
{
	return MQTT_Subscribe(&mqttClient, (char*)topic.c_str(), qos);
}



//-------------------------------------------------------------------------------
// set user callback functions
//-------------------------------------------------------------------------------
void MQTT::onConnected( void (*function)(void) )
{
	onMqttConnectedCb = function;
}

void MQTT::onDisconnected( void (*function)(void) )
{
	onMqttDisconnectedCb = function;
}

void MQTT::onPublished( void (*function)(void) )
{
	onMqttPublishedCb = function;
}

void MQTT::onData( void (*function)(String&, String&) )
{
	onMqttDataCb = function;
}

void MQTT::onData( void (*function)(const char*, uint32_t, const char*, uint32_t) )
{
	onMqttDataRawCb = function;
}


// internal callback, calling user CB
void MQTT::_onMqttDataCb(const char* topic, uint32_t topic_len, const char* buf, uint32_t buf_len)
{
	if (onMqttDataRawCb) {
		onMqttDataRawCb(topic, topic_len, buf, buf_len);
	}
	
	if (onMqttDataCb) {
		
		char* topicCpy = (char*)malloc(topic_len+1);
		memcpy(topicCpy, topic, topic_len);
		topicCpy[topic_len] = 0;
		// string it
		String topicStr(topicCpy);
		
		char* bufCpy = (char*)malloc(buf_len+1);
		memcpy(bufCpy, buf, buf_len);
		bufCpy[buf_len] = 0;
		// string it
		String bufStr(bufCpy);
		
		onMqttDataCb(topicStr, bufStr);
		
		free(topicCpy);
		free(bufCpy);
	}
}









