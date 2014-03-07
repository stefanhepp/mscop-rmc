/*
 * Problem.hpp
 *
 *  Created on: Mar 6, 2014
 *  Author: ioan, stefan
 */

#ifndef PROBLEM_HPP_
#define PROBLEM_HPP_

#include "XMLDataTypes.hpp"

#include <string>
#include <vector>
#include <map>


static const int MAX_TRAVEL_TIME = 5000000;

class Order {
public:
  /// volumes in dm3, rates in dm3/min
  Order(const std::string &name, int vol, int dischargeRate, int pumpLength, int preferredStation,
        bool maxVolumeAllowed, int startTime, int setupTime, int numStations)
  : _name(name),
    _totalVolume(vol), _dischargeRate(dischargeRate), 
    _requiredPumpLength(pumpLength), _preferredStation(preferredStation),
    _startTime(startTime), _setupTime(setupTime), _maxVolumeAllowed(maxVolumeAllowed)
  {  
    for(int i = 0; i < numStations; i++){
      _dTimeFromStations.push_back(MAX_TRAVEL_TIME);
      _dTimeToStations.push_back(MAX_TRAVEL_TIME);
    }
  }
  
  virtual ~Order(){}

  std::string name() { return _name; }
  
  int requiredPumpLength() const {return _requiredPumpLength;}

  // dm3
  int totalVolume() const {return _totalVolume;}

  int preferredStation() const {return _preferredStation;}

  int timeStart() const {return _startTime;}

  int dTimeSetup() const {return _setupTime;}

  // dm3 / min
  int dischargeRate() const {return _dischargeRate;}
  
  bool maxVolumeAllowed() const { return _maxVolumeAllowed; }
  
  //returns the time necessary to travel from station to current order
  int fromStation(int station) const {
    return _dTimeFromStations[station];
  }
  //returns the time necessary to travel from current order to station
  int toStation(int station) const {
    return _dTimeToStations[station];
  }

  void setFromYard(int i, int val) {
    _dTimeFromStations[i]=val;
  }

  void setToYard(int i, int val) {
    _dTimeToStations[i]=val;
  }

private:
  std::string _name;
  
  int _totalVolume;
  int _dischargeRate;
  int _requiredPumpLength;
  int _preferredStation;
  int _startTime;
  int _setupTime;
  bool _maxVolumeAllowed;

  std::vector<int> _dTimeToStations;
  std::vector<int> _dTimeFromStations;
};


class Vehicle {
public:
  /// volumes in dm3, rates in dm3/min
  Vehicle(const std::string &name, int pumpLength, int maxDischargeRate, int normalVolume, 
          int maxVolume, int availFrom)
  : _name(name),
    _pumpLenght(pumpLength), _maxDischargeRate(maxDischargeRate), 
    _normalVolume(normalVolume), _maxVolume(maxVolume), _availableFrom(availFrom)
  {}
  
  virtual ~Vehicle() {}
  
  std::string name() const { return _name; }
  
  int pumpLength() const {
    return _pumpLenght;
  }
  
  /// dm3 / min
  int maxDischargeRate() const {
    return _maxDischargeRate;
  }
  
  // dm3
  int volume(bool maxVolume) const {
    if (_maxVolume == _normalVolume) {
      return _maxVolume;
    }
    if (maxVolume && _maxVolume) {
      return _maxVolume;
    }
    return _normalVolume;
  }
  
  int availableFrom() const {
    return _availableFrom;
  }

private:
  std::string _name;
  
  int _pumpLenght;
  int _maxDischargeRate;
  int _normalVolume;
  int _maxVolume;
  int _availableFrom;
};

class Station {
public:
  Station(const std::string &name, int loadingMinutes) 
  : _name(name), _loadingMinutes(loadingMinutes) 
  {}
  
  std::string name() const { return _name; }
  
  int loadingMinutes() const { return _loadingMinutes; }
  
private:
  std::string _name;
  int _loadingMinutes;
};

class Delivery {
public:
  Delivery()
  {}

  virtual ~Delivery() {}
};

class RMCInput { 
public:
  RMCInput() {}
  
  void loadProblem(const char *filename);
  
  int getTimeMax() const { return 100; }
  
  int getAlpha1() const { return 10; }
  
  int getAlpha2() const { return 10; }
  
  int getAlpha3() const { return 1; }
 
  int getAlpha4() const { return 20; }
  
  int getAlpha5() const { return 20; }
  
  
  int getNumVehicles() const { return _vehicles.size(); }
  
  int getNumStations() const { return _stations.size(); }
  
  int getNumOrders() const { return _orders.size(); }

  // Per order
  int getMaxDeliveries() const { return _maxDeliveries; }
  
  // overall deliveries
  int getMaxTotalDeliveries() const { return _orders.size() * _vehicles.size() * _maxDeliveries; }
  
  int getMaxTimeStamp() const { return _maxTimeStamp; }
  
  int getStation(const std::string &code) const;
  
  const std::vector<Order*>& getOrders() const { return _orders; }
  
  const std::vector<Vehicle*>& getVehicles() const { return _vehicles; }
  
  const std::vector<Station*>& getStations() const { return _stations; }
  
  const Order &getOrder(int idx) const { return *_orders[idx]; }
  
  const Vehicle &getVehicle(int idx) const { return *_vehicles[idx]; }
  
  const Station &getStation(int idx) const { return *_stations[idx]; }
  

  const int* getOrderStartTimes() const { return _orderStartTimes; }  

  const int* getOrderTotalVolumes() const { return _orderTotalVolumes; }

  // [order, vehicle]
  const int* getOrderVehicleVolumes() const { return _orderVehicleVolumes; }
  
  const int* getOrderReqDischargeRates() const { return _orderReqDischargeRates; }
  
  const int* getOrderReqPipeLengths() const { return _orderReqPipeLength; }
  
  const int* getOrderSetupTimes() const { return _orderSetupTimes; }
  
  const int* getOrderPreferredStations() const { return _orderPreferredStations; }
  
  // [order, station]
  const int* getTravelTimesToYards() const { return _travelTimesTo; }
  
  // [order, station]
  const int* getTravelTimesFromYards() const { return _travelTimesFrom; }
  
  const int* getStationLoadTimes() const { return _stationLoadTimes; }
  
  
private:
  void setTimesForOrder(Order *order, XMLOrder &xmlorder);
  
  void buildValueArrays();
  
  std::vector<Order*> _orders;
  std::vector<Vehicle*> _vehicles;
  std::vector<Station*> _stations;
  
  std::map<std::string, int> _stationCodes;
  
  int _maxDeliveries;
  int _maxTimeStamp;
  int _maxTravelTime;
  
  time_t _baseTimeStamp;
  
  // Arrays 
  int*   _orderStartTimes;
  int*   _orderTotalVolumes;
  int*   _orderReqDischargeRates;
  int*   _orderReqPipeLength;
  int*   _orderSetupTimes;
  int*   _orderPreferredStations;
  
  int*   _stationLoadTimes;
  
  // orders * vehicles  
  int*   _orderVehicleVolumes;
  
  // orders * stations
  int*   _travelTimesTo;
  int*   _travelTimesFrom;
};

class RMCOutput {
  
};

#endif
