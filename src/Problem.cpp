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
#include <cmath>


int RMCInput::getStation(const std::string &code) const
{
  if (code.empty()) return -1;
  std::map<std::string, int>::const_iterator it = _stationCodes.find(code);
  
  if (it == _stationCodes.end()) {
    return -1;
  }
  return it->second;
}

void RMCInput::setTimesForOrder(Order* order, XMLOrder& xmlorder)
{
  std::list<_StationDuration_>::iterator ite;
  for (ite = xmlorder._constructionYard->_stationDuration.begin();
       ite != xmlorder._constructionYard->_stationDuration.end();
       ite++) 
  {
    int idx = getStation(ite->_stationCode);
    if (idx != -1) {
      if (ite->_direction == true) {
        //order->setToYard(idx, (ite->_drivingMinutes==0? 32000 : ite->_drivingMinutes));
        order->setToYard(idx, ite->_drivingMinutes);
        
        _maxTravelTime = std::max(_maxTravelTime, ite->_drivingMinutes);
      } else {
        //order->setFromYard(idx, (ite->_drivingMinutes==0? 32000 : ite->_drivingMinutes));
        order->setFromYard(idx, ite->_drivingMinutes);
        
        _maxTravelTime = std::max(_maxTravelTime, ite->_drivingMinutes);
      }
    }
  }
}

void RMCInput::loadProblem(char* filename) {
  std::vector<XMLOrder*> orderList;
  std::vector<XMLVehicle*> vehicleList;
  std::vector<XMLStation*> stationList;

  ReadXML xmlReader(filename);
  xmlReader.parseFile();

  xmlReader.getOrdersList(orderList);
  xmlReader.getVehiclesList(vehicleList);
  xmlReader.getStationsList(stationList);

  //xmlReader->print();
  
  //compute the base time stamp
  _baseTimeStamp = orderList[0]->_unixTimeStamp;

  for (int i=1 ; i < orderList.size(); i++) {
    if (_baseTimeStamp > orderList[i]->_unixTimeStamp) {
      _baseTimeStamp = orderList[i]->_unixTimeStamp;
    }
  }

  for (int i=0 ; i < vehicleList.size(); i++) {
    if (_baseTimeStamp > vehicleList[i]->_nextAvailabelTimeStampUnix) {
      _baseTimeStamp = vehicleList[i]->_nextAvailabelTimeStampUnix;
    }
  }

  _stations.clear();
  _vehicles.clear();
  _orders.clear();
  
  _maxTravelTime = 0;
  _maxDeliveries = 0;
  _maxTimeStamp = 0;
  
  int maxLoadTime = 0;
  
  // load stations
  for(int i = 0 ; i < stationList.size(); i++) {
    XMLStation &station = *stationList[i];
    
    if (station._stationCode.empty()) continue;
    
    _stationCodes.insert( std::pair<std::string,int>(station._stationCode, _stations.size()) );
    
    _stations.push_back( new Station(station._stationCode, station._loadingMinutes) );
    
    maxLoadTime = std::max(maxLoadTime, station._loadingMinutes);
  }
  
  //treat each of the cars
  for (int i = 0; i < vehicleList.size(); i++) {
    XMLVehicle &currentVehicle = *vehicleList[i];
    
    int timeStamp = (int)difftime(currentVehicle._nextAvailabelTimeStampUnix, _baseTimeStamp);
    
    if (currentVehicle._normalVolume == 0 && currentVehicle._maximumVolume == 0) continue;
    
    Vehicle *v = new Vehicle(currentVehicle._vehicleCode, currentVehicle._pumpLength, currentVehicle._dischargeRate * (1000.0/60.0),
                             currentVehicle._normalVolume * 1000, currentVehicle._maximumVolume * 1000, timeStamp);
    _vehicles.push_back(v);
  }
  
  //treat each of the orders
  for(int i = 0; i < orderList.size(); i++) {
    XMLOrder &currentOrder = *orderList[i];
    
    int minTimeStamp = (int)difftime(currentOrder._unixTimeStamp, _baseTimeStamp);
    
    Order *o = new Order(currentOrder._orderCode, currentOrder._volume * 1000,(currentOrder._dischargeRate * (1000.0/60.0)), 
                         currentOrder._pumpLength, getStation(currentOrder._preferredStationCode), 
                         currentOrder._maxVolumeAllowed, minTimeStamp, 
                         currentOrder._constructionYard->_waitingMinutes, stationList.size());

    setTimesForOrder(o, currentOrder);

    _orders.push_back(o);
  }

  if (_vehicles.empty()) return;
  
  // TODO make this more tight! (i.e., use a greedy alg. to assign yards to trucks
  
  // find the smallest vehicle capacity
  int minCapacity = _vehicles[0]->volume(false);
  for (int i = 1; i < _vehicles.size(); i++) {
    minCapacity = std::min(minCapacity, _vehicles[i]->volume(false));
  }
  
  // count how many deliveries are needed at most per order
  for (int i = 0; i < _orders.size(); i++) {
    Order &order = *_orders[i];
    
    int volume = order.totalVolume();
    int deliveries = volume / minCapacity;
    
    int timeDelivery = maxLoadTime + 2 * _maxTravelTime + order.dTimeSetup() 
                     + minCapacity / order.dischargeRate();
    
    _maxDeliveries = std::max(_maxDeliveries, deliveries);
    
    _maxTimeStamp = std::max(_maxTimeStamp, order.timeStart());
    _maxTimeStamp += deliveries * timeDelivery;
  }
  
  buildValueArrays();
}

void RMCInput::buildValueArrays()
{
  _orderStartTimes = new int[_orders.size()];
  _orderReqDischargeRates = new int[_orders.size()];
  _orderReqPipeLength = new int[_orders.size()];
  _orderTotalVolumes = new int[_orders.size()];
  _orderSetupTimes = new int[_orders.size()];
  _orderPreferredStations = new int[_orders.size()];
  
  for (int i = 0; i < _orders.size(); i++) {
    Order *o = _orders[i];
    _orderStartTimes[i] = o->timeStart();
    _orderReqDischargeRates[i] = o->dischargeRate();
    _orderReqPipeLength[i] = o->requiredPumpLength();
    _orderTotalVolumes[i] = o->totalVolume();
    _orderSetupTimes[i] = o->dTimeSetup();
    _orderPreferredStations[i] = o->preferredStation();
  }
  
  _stationLoadTimes = new int[_stations.size()];
  
  for (int i = 0; i < _stations.size(); i++) {
    Station *s = _stations[i];
    _stationLoadTimes[i] = s->loadingMinutes(); 
  }
  
  _orderVehicleVolumes = new int[_orders.size() * _vehicles.size()];
  
  for (int j = 0; j < _orders.size(); j++) {
    Order *o = _orders[j];
    for (int i = 0; i < _vehicles.size(); i++) {
      Vehicle *v = _vehicles[i];
      _orderVehicleVolumes[ i * _orders.size() + j ] = v->volume(o->maxVolumeAllowed());
    }
  }
  
  _travelTimesTo   = new int[_orders.size() * _stations.size()];
  _travelTimesFrom = new int[_orders.size() * _stations.size()];
  
  for (int i = 0; i < _orders.size(); i++) {
    Order *o = _orders[i];
    for (int j = 0; j < _stations.size(); j++) {
      _travelTimesTo  [ i * _stations.size() + j ] = o->toStation(j);
      _travelTimesFrom[ i * _stations.size() + j ] = o->fromStation(j);
    }
  }
  
  
}

