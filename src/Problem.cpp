/*
 * OrderClass.cpp
 *
 *  Created on: Mar 6, 2014
 *  Author: stefan
 */

#include "Problem.hpp"

#include "XMLDataTypes.hpp"
#include "ReadXML.hpp"

#include <ctime>

int RMCInput::getStation(const char *code) const
{
  if (!code) return -1;
  std::map<std::string, int>::const_iterator it = _stationCodes.find(std::string(code));
  
  if (it == _stationCodes.end()) {
    return -1;
  }
  return it->second;
}

void RMCInput::setTimesForOrder(Order* order, XMLOrder& xmlorder)
{
  std::list<_StationDuration_>::iterator ite;
  for (ite = xmlorder._constructionYard._stationDuration.begin();
       ite != xmlorder._constructionYard._stationDuration.end();
       ite++) 
  {
    int idx = getStation(ite->_stationCode);
    if (idx != -1) {
      if (ite->_direction == true) {
        //order->setToYard(idx, (ite->_drivingMinutes==0? 32000 : ite->_drivingMinutes));
        order->setToYard(idx, ite->_drivingMinutes);
      } else {
        //order->setFromYard(idx, (ite->_drivingMinutes==0? 32000 : ite->_drivingMinutes));
        order->setFromYard(idx, ite->_drivingMinutes);
      }
    }
  }
}

void RMCInput::loadProblem(char* filename) {
  std::vector<XMLOrder> orderList;
  std::vector<XMLVehicle> vehicleList;
  std::vector<XMLStation> stationList;

  ReadXML *xmlReader = new ReadXML(filename);
  xmlReader->parseFile();

  xmlReader->getOrdersList(orderList);
  xmlReader->getVehiclesList(vehicleList);
  xmlReader->getStationsList(stationList);

  xmlReader->print();
  
  //compute the base time stamp
  _baseTimeStamp = orderList[0]._unixTimeStamp;

  for (int i=1 ; i < orderList.size(); i++) {
    if (_baseTimeStamp > orderList[i]._unixTimeStamp) {
      _baseTimeStamp = orderList[i]._unixTimeStamp;
    }
  }

  for (int i=0 ; i < vehicleList.size(); i++) {
    if (_baseTimeStamp > vehicleList[i]._nextAvailabelTimeStampUnix) {
      _baseTimeStamp = vehicleList[i]._nextAvailabelTimeStampUnix;
    }
  }

  _stations.clear();
  _vehicles.clear();
  _orders.clear();
  
  // load stations
  for(int i = 0 ; i < stationList.size(); i++) {
    XMLStation &station = stationList[i];
    
    if (!station._stationCode) continue;
    
    _stationCodes.insert( std::pair<std::string,int>(std::string(station._stationCode), _stations.size()) );
    
    _stations.push_back( new Station(station._stationCode, station._loadingMinutes) );
  }
  
  //treat each of the cars
  for (int i = 0; i < vehicleList.size(); i++) {
    XMLVehicle &currentVehicle = vehicleList[i];
    
    int timeStamp = (int)difftime(currentVehicle._nextAvailabelTimeStampUnix, _baseTimeStamp);
    
    if (currentVehicle._normalVolume == 0 && currentVehicle._maximumVolume == 0) continue;
    
    Vehicle *v = new Vehicle(currentVehicle._vehicleCode, currentVehicle._pumpLength, currentVehicle._dischargeRate/60.0,
                             currentVehicle._normalVolume, currentVehicle._maximumVolume, timeStamp);
    _vehicles.push_back(v);
  }
  
  //treat each of the orders
  for(int i = 0; i < orderList.size(); i++) {
    XMLOrder &currentOrder = orderList[i];
    
    int minTimeStamp = (int)difftime(currentOrder._unixTimeStamp, _baseTimeStamp);
    
    Order *o = new Order(currentOrder._orderCode, currentOrder._volume,(currentOrder._dischargeRate/60.0), 
                         currentOrder._pumpLength, getStation(currentOrder._preferredStationCode), 
                         currentOrder._maxVolumeAllowed, minTimeStamp, 
                         currentOrder._constructionYard._waitingMinutes, stationList.size());

    setTimesForOrder(o, currentOrder);

    _orders.push_back(o);
  }

}


