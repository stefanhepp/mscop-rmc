/*
 * ReadXML.cpp
 *
 *  Created on: Mar 4, 2014
 *  Author: ioan
 */

#include "ReadXML.hpp"
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <time.h>
#include <string.h>

using namespace std;

ReadXML::ReadXML(const char * filename) 
: _filename(filename)
{
}

ReadXML::~ReadXML() {
  list<XMLOrder*>::iterator ito;
  for (ito = _orderList.begin(); ito != _orderList.end(); ito++) {
    delete *ito;
  }
  _orderList.clear();
  list<XMLVehicle*>::iterator itv;
  for (itv = _vehicleList.begin(); itv != _vehicleList.end(); itv++) {
    delete *itv;
  }
  _vehicleList.clear();
  list<XMLStation*>::iterator its;
  for (its = _stationList.begin(); its != _stationList.end(); its++) {
    delete *its;
  }
  _stationList.clear();

}

void ReadXML::parseFile() {
  xmlDoc *doc = NULL;
  xmlNode *root_element = NULL;

  /*parse the file and get the DOM */
  doc = xmlReadFile(_filename, NULL, 0);

  if (doc == NULL) {
    printf("error: could not parse file %s\n", _filename);
    exit(0);
  }

  /*Get the root element node */
  root_element = xmlDocGetRootElement(doc);

  processFile(root_element->children);
  /*free the document */
  xmlFreeDoc(doc);

  /*
   *Free the global variables that may
   *have been allocated by the parser.
   */
  xmlCleanupParser();
}


void ReadXML::print(){
  //some information about orders
  list<XMLOrder*>::iterator ite;
    for(ite = _orderList.begin(); ite!=_orderList.end(); ite++ ){
      cout<<(*ite)->_orderCode<<endl;
      cout<<(*ite)->_dischargeRate<<endl;
      cout<<(*ite)->_startTime<<endl;
      cout<<"const yard"<<(*ite)->_constructionYard->_code<<endl;
      list<XMLStationDuration>::iterator site;
      for(site = (*ite)->_constructionYard->_stationDuration.begin(); site!= (*ite)->_constructionYard->_stationDuration.end(); site++){
        cout<< site->_drivingMinutes<< " code "<< site->_stationCode<<endl;
      }
    }
    //some information about vehicle
    list<XMLVehicle*>::iterator ve;
    for(ve = _vehicleList.begin(); ve!= _vehicleList.end(); ve++){
      cout<<"vehicle "<< (*ve)->_vehicleType<<"  "<<(*ve)->_vehicleCode<<"  "<<(*ve)->_dischargeRate<<endl;
    }
    //some information about stations
    list<XMLStation*>::iterator is;
    for(is = _stationList.begin(); is!= _stationList.end(); is++){
      cout<<"station "<<(*is)->_stationCode<<endl;
      cout<<"load minutes "<<(*is)->_loadingMinutes<<endl;
    }

}
/**
 * iterate over top level structure /orders/vehicles/sites :
 * @a_node: the initial xml node to consider.
 *
 * Iterates over the top level nodes and calls the appropriate reader for each of them
 */
void ReadXML::processFile(xmlNode * a_node)
{
    xmlNode *cur_node = NULL;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE && !xmlStrcmp(cur_node->name, (const xmlChar*)"Orders") ) {

          xmlNode* order = NULL;
            for(order = cur_node->children; order ; order = order->next){
              if(!xmlStrcmp(order->name , (const xmlChar*)"Order")){
              readOrders(order);
              }
            }
            continue;
        }
        if (cur_node->type == XML_ELEMENT_NODE && !xmlStrcmp(cur_node->name, (const xmlChar*)"Vehicles") ) {

      xmlNode* order = NULL;
      for (order = cur_node->children; order; order = order->next) {
        if (!xmlStrcmp(order->name, (const xmlChar*) "Vehicle")) {
          readVehicles(order);
        }
      }
      continue;
    }
        if (cur_node->type == XML_ELEMENT_NODE && !xmlStrcmp(cur_node->name, (const xmlChar*)"Stations") ) {

      xmlNode* order = NULL;
      for (order = cur_node->children; order; order = order->next) {
        if (!xmlStrcmp(order->name, (const xmlChar*) "Station")) {
          readStations(order);
        }
      }
      continue;
    }
    }

}

/*
 * traverse all the orders and collect information for each of them. And put it in the list of informations
 */
void ReadXML::readOrders(xmlNode* order) {
  xmlNode * iter = NULL;
  XMLOrder *o = new XMLOrder();

  for (iter = order->children; iter; iter = iter->next) {
    if (!xmlStrcmp(iter->name, (const xmlChar*) "OrderCode")) {
      o->_orderCode = (char*)xmlNodeGetContent(iter);
      continue;
    }
    if (!xmlStrcmp(iter->name, (const xmlChar*) "PumpLineLengthRequired")) {
      o->_pumpLength = atoi((char*)xmlNodeGetContent(iter));
      continue;
    }
    if (!xmlStrcmp(iter->name, (const xmlChar*) "From")) {
      o->_startTime = (char*) xmlNodeGetContent(iter);
      struct tm t = {0,0,0,0,0,0,0,0,0};
      char *rs = strptime(o->_startTime.c_str(), "%Y-%m-%dT%H:%M:%S", &t);
      if (!rs || (*rs != '\0' && *rs != '.'))
        cout << "error parsing " << o->_startTime << endl;
      else {
        time_t timeStamp = mktime(&t);
        o->_unixTimeStamp = timeStamp;
      }

      continue;
    }
    if (!xmlStrcmp(iter->name, (const xmlChar*) "TotalVolumeM3")) {
      o->_volume = atof((char*)xmlNodeGetContent(iter));
      continue;
    }
    if (!xmlStrcmp(iter->name, (const xmlChar*) "RequiredDischargeM3PerHour")) {
      o->_dischargeRate = atof((char*)xmlNodeGetContent(iter));
      continue;
    }
    if (!xmlStrcmp(iter->name, (const xmlChar*) "PreferredStationCode")) {
      o->_preferredStationCode = ((char*)xmlNodeGetContent(iter));
      continue;
    }
    if (!xmlStrcmp(iter->name, (const xmlChar*) "MaximumVolumeAllowed")) {
      if(!xmlStrcmp(xmlNodeGetContent(iter),(const xmlChar*)"true")){
        o->_maxVolumeAllowed = true;
      } else o->_maxVolumeAllowed = false;
      continue;
    }
    if (!xmlStrcmp(iter->name, (const xmlChar*) "IsPickUp")) {
      if(!xmlStrcmp(xmlNodeGetContent(iter),(const xmlChar*)"true")){
              o->_isPickup = true;
      } else o->_isPickup = false;
      continue;
    }
    if (!xmlStrcmp(iter->name, (const xmlChar*) "Priority")) {
      o->_priority = atoi((char*)xmlNodeGetContent(iter));
      continue;
    }

    //read construction yard information
    if (!xmlStrcmp(iter->name, (const xmlChar*) "ConstructionYard")) {
      XMLConstructionYard cyard;
      o->_constructionYard = readConstructionYard(iter);

    }
  }
  _orderList.push_back(o);
}

/*
 * traverse all the vehicle nodes and collect information about each of them and collect them in the
 * list of vehicles
 */
void ReadXML::readVehicles(xmlNode* vehicles) {
  xmlNode * iter = NULL;
  XMLVehicle *o = new XMLVehicle();

  for (iter = vehicles->children; iter; iter = iter->next) {

    if (!xmlStrcmp(iter->name, (const xmlChar*) "VehicleCode")) {
      o->_vehicleCode = ((char*)xmlNodeGetContent(iter));
      continue;
    }
    if (!xmlStrcmp(iter->name, (const xmlChar*) "VehicleType")) {
      o->_vehicleType = ((char*)xmlNodeGetContent(iter));
      continue;
    }
    if (!xmlStrcmp(iter->name, (const xmlChar*) "NormalVolume")) {
      o->_normalVolume = atoi((char*) xmlNodeGetContent(iter));
      continue;
    }
    if (!xmlStrcmp(iter->name, (const xmlChar*) "PumpLineLength")) {
      o->_pumpLength = atoi((char*)xmlNodeGetContent(iter));
      continue;
    }
    if (!xmlStrcmp(iter->name, (const xmlChar*) "MaximumVolume")) {
      o->_maximumVolume = atoi((char*) xmlNodeGetContent(iter));
      continue;
    }
    if (!xmlStrcmp(iter->name, (const xmlChar*) "DischargeM3PerHour")) {
      o->_dischargeRate = atof((char*) xmlNodeGetContent(iter));
      continue;
    }
    if (!xmlStrcmp(iter->name,(const xmlChar*) "NextAvailableStartDateTime")) {
      o->_nextAvailableTime = ((char*) xmlNodeGetContent(iter));

      struct tm t = {0,0,0,0,0,0,0,0,0};
      char *rs = strptime(o->_nextAvailableTime.c_str(), "%Y-%m-%dT%H:%M:%S", &t);
      if (!rs || (*rs != '\0' && *rs != '.'))
        cout << "error parsing " << o->_nextAvailableTime << endl;
      else {
        time_t timeStamp = mktime(&t);
        o->_nextAvailabelTimeStampUnix = timeStamp;
      }

      continue;
    }
  }
  _vehicleList.push_back(o);
}

/*
 * traverse all the station nodes and collect information about each of them and collect them in the
 * list of station
 */
void ReadXML::readStations(xmlNode* stations) {
  xmlNode * iter = NULL;

  XMLStation *o = new XMLStation();

  for (iter = stations->children; iter; iter = iter->next) {

    if (!xmlStrcmp(iter->name, (const xmlChar*) "StationCode")) {
      o->_stationCode = ((char*)xmlNodeGetContent(iter));
      continue;
    }
    if (!xmlStrcmp(iter->name, (const xmlChar*) "LoadingMinutes")) {
      o->_loadingMinutes = atoi((char*)xmlNodeGetContent(iter));
      continue;
    }
  }
  _stationList.push_back(o);
}

/*
 * traverse a node containing construction yards information and return a construction yard
 */
XMLConstructionYard *ReadXML::readConstructionYard(xmlNode* cyard) {
  XMLConstructionYard *cya = new XMLConstructionYard();
  xmlNode* ci = NULL;

  for (ci = cyard->children; ci; ci = ci->next) {
    if (!xmlStrcmp(ci->name, (const xmlChar*) "ConstructionYardCode")) {
      cya->_code = (char*) xmlNodeGetContent(ci);
      continue;
    }

    if (!xmlStrcmp(ci->name, (const xmlChar*) "WaitingMinutes")) {
      cya->_waitingMinutes = atoi((char*) xmlNodeGetContent(ci));
      continue;
    }

    //read Station duration here
    if (!xmlStrcmp(ci->name, (const xmlChar*) "StationDurations")) {

      xmlNode* constSite = NULL;
      /*
       * traverse all the station duration nodes and collect information from each of them
       * afterwards add the information to the corresponding list of stationDurations
       */
      for(constSite = ci->children; constSite; constSite=constSite->next){

        if (!xmlStrcmp(constSite->name, (const xmlChar*) "StationDuration")) {

          xmlNode* station = NULL;
          XMLStationDuration st;
          for(station = constSite->children; station; station= station->next){

            if (!xmlStrcmp(station->name, (const xmlChar*) "StationCode")){
             st._stationCode = (char*) xmlNodeGetContent(station);
            continue;
            }
            if (!xmlStrcmp(station->name, (const xmlChar*) "DrivingMinutes")) {
              st._drivingMinutes = atoi((char*) xmlNodeGetContent(station));
              continue;
            }
            if (!xmlStrcmp(station->name, (const xmlChar*) "Direction")) {

              if (!xmlStrcmp(xmlNodeGetContent(station),(const xmlChar*)"To")) {
                st._direction = true;
              } else
                 st._direction = false;
              continue;
            }

          }
          //add another station duration info to the list of the construction yard
          cya->_stationDuration.push_back(st);
        }

      }
      continue;
    }
  }
  //return the new construction site
  return cya;
}
