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
  Order(char *name, float vol, float dischargeRate, int pumpLength, int preferredStation,
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
  
  int requiredPumpLength() {return _requiredPumpLength;}

  float totalVolume() {return _totalVolume;}

  int preferredStation() {return _preferredStation;}

  int timeStart() {return _startTime;}

  int dTimeSetup() {return _setupTime;}

  float dischargeRate() {return _dischargeRate;}
  
  bool maxVolumeAllowed() { return _maxVolumeAllowed; }
  
  //returns the time necessary to travel from station to current order
  int fromStation(int station) {
    return _dTimeFromStations[station];
  }
  //returns the time necessary to travel from current order to station
  int toStation(int station) {
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
  
  float _totalVolume;
  float _dischargeRate;
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
  Vehicle(char *name, int pumpLength, float maxDischargeRate, int normalVolume, 
          int maxVolume, int availFrom)
  : _name(name),
    _pumpLenght(pumpLength), _maxDischargeRate(maxDischargeRate), 
    _normalVolume(normalVolume), _maxVolume(maxVolume), _availableFrom(availFrom)
  {}
  
  virtual ~Vehicle() {}
  
  std::string name() { return _name; }
  
  int pumpLength() {
    return _pumpLenght;
  }
  
  float maxDischargeRate() {
    return _maxDischargeRate;
  }
  
  float volume(bool maxVolume) {
    if (_maxVolume == _normalVolume) {
      return _maxVolume;
    }
    if (maxVolume && _maxVolume) {
      return _maxVolume;
    }
    return _normalVolume;
  }
  
  int availableFrom() {
    return _availableFrom;
  }

private:
  std::string _name;
  
  int _pumpLenght;
  float _maxDischargeRate;
  int _normalVolume;
  int _maxVolume;
  int _availableFrom;
};

class Station {
public:
  Station(char *name, int loadingMinutes) 
  : _name(name), _loadingMinutes(loadingMinutes) 
  {}
  
  std::string name() { return _name; }
  
  int loadingMinutes() { return _loadingMinutes; }
  
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
  
  void loadProblem(char *filename);
  
  int getTimeMax() { return 100; }
  
  int getAlpha1() { return 10; }
  
  int getAlpha2() { return 10; }
  
  int getAlpha3() { return 1; }
 
  int getAlpha4() { return 20; }
  
  int getAlpha5() { return 20; }
  
  
  int getNumVehicles() const { return _vehicles.size(); }
  
  int getNumStations() const { return _stations.size(); }
  
  int getNumOrders() const { return _orders.size(); }
  
  int getMaxDeliveries() const { return _maxDeliveries; }
  
  int getMaxTimeStamp() const { return _maxTimeStamp; }
  
  int getStation(const char *code) const;
  
  const std::vector<Order*>& getOrders() { return _orders; }
  
  const std::vector<Vehicle*>& getVehicles() { return _vehicles; }
  
  const std::vector<Station*>& getStations() { return _stations; }
  
  Order &getOrder(int idx) { return *_orders[idx]; }
  
  Vehicle &getVehicle(int idx) { return *_vehicles[idx]; }
  
  Station &getStation(int idx) { return *_stations[idx]; }
  

  const int* getOrderStartTimes() { return _orderStartTimes; }  

  
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
  int* _orderStartTimes;
};

class RMCOutput {
  
};

#endif
