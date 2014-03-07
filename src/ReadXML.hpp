/*
 * ReadXML.hpp
 *
 *  Created on: Mar 4, 2014
 *  Author: ioan
 */

#ifndef READXML_HPP_
#define READXML_HPP_

#include <iostream>
#include <list>
#include <vector>

#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "XMLDataTypes.hpp"

class ReadXML {
public:
  ReadXML(char *filename);
  
  virtual ~ReadXML();

  void parseFile();
  
  void print();

  void getOrdersList(std::vector<XMLOrder*> &vec){
    vec.clear();
    std::copy( _orderList.begin(), _orderList.end(), std::back_inserter( vec ) );
  }
    
  void getVehiclesList(std::vector<XMLVehicle*> &vec){
    vec.clear();
    std::copy( _vehicleList.begin(), _vehicleList.end(), std::back_inserter( vec ) );
  }

  void getStationsList(std::vector<XMLStation*> &vec){
    vec.clear();
    std::copy( _stationList.begin(), _stationList.end(), std::back_inserter( vec ) );
  }


private:
  void processFile(xmlNode* node);
  void readOrders(xmlNode* orders);
  void readVehicles(xmlNode* vehicles);
  void readStations(xmlNode* stations);

  XMLConstructionYard *readConstructionYard(xmlNode* cyard);

  //file to be parsed
  char* _filename;
  /*
   * List of orders
   */
  std::list<XMLOrder*> _orderList;
  /*
   * List of vehicles
   */
  std::list<XMLVehicle*> _vehicleList;
  /*
   * List of stations
   */
  std::list<XMLStation*> _stationList;
};

#endif /* READXML_HPP_ */
