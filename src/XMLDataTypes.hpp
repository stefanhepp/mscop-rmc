/*
 * DataTypes.hpp
 *
 *  Created on: Mar 4, 2014
 *  Author: ioan
 */

#ifndef XMLDATATYPES_HPP_
#define XMLDATATYPES_HPP_

#include <iostream>
#include <ctime>

#include <list>

#include <string>

struct _StationDuration_{
  std::string _stationCode;
  int _drivingMinutes;
  /**
   * as a convention if the direction for the constructionYard is From then _direction is true
   * otherwise it is false;
   * */
  bool _direction;
};

struct _ConstructionYard_{
  std::string _code;
  int _waitingMinutes;
  std::list<_StationDuration_> _stationDuration;
};

struct _Order_{
  std::string _orderCode;
  int _priority;
  std::string _preferredStationCode;
  float _volume;
  bool _maxVolumeAllowed;
  float _dischargeRate;
  int _pumpLength;
  std::string _startTime;
  time_t _unixTimeStamp;
  bool _isPickup;
  _ConstructionYard_ *_constructionYard;

  _Order_() : _priority(0), _volume(0), _maxVolumeAllowed(false)
    , _dischargeRate(0), _pumpLength(0), _unixTimeStamp(0), _isPickup(false), _constructionYard(0){
  }
  
  ~_Order_() {
    if (_constructionYard) delete _constructionYard;
  }
};

struct _Vehicle_{
  std::string _vehicleCode;
  std::string _vehicleType;
  int _pumpLength;
  int _normalVolume;
  int _maximumVolume;
  float _dischargeRate;
  std::string _nextAvailableTime;
  time_t _nextAvailabelTimeStampUnix;
};

struct _Station_{
  std::string _stationCode;
  int _loadingMinutes;
};


/*
 * struct that keeps the information regarding construction site => station duration
 * time from the station to the construction site and directions
 */
typedef struct _StationDuration_ XMLStationDuration;
/*
 * struct that keeps the information regarding each of the stations
 */
typedef struct _Station_ XMLStation;

/*
 * struct that keeps the information regarding each construction site. It contains a list of
 * StationDurations as member.
 */
typedef struct _ConstructionYard_ XMLConstructionYard;
/*
 * Struct that keeps information regarding a single vehicle
 */
typedef struct _Vehicle_ XMLVehicle;
/*
 * Struct that keeps the information regarding each order. Each of them has a member of type
 * ConstructionYard, in this way we keep track of what to be delivered and where
 */
typedef struct _Order_ XMLOrder;

#endif /* XMLDATATYPES_HPP_ */
