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

struct _StationDuration_{
  char* _stationCode;
  int _drivingMinutes;
  /**
   * as a convention if the direction for the constructionYard is From then _direction is true
   * otherwise it is false;
   * */
  bool _direction;
};

struct _ConstructionYard_{
  char* _code;
  int _waitingMinutes;
  std::list<_StationDuration_> _stationDuration;
};

struct _Order_{
  char* _orderCode;
  int _priority;
  char* _preferredStationCode;
  float _volume;
  bool _maxVolumeAllowed;
  float _dischargeRate;
  int _pumpLength;
  char*_startTime;
  time_t _unixTimeStamp;
  bool _isPickup;
  _ConstructionYard_ _constructionYard;

  _Order_() : _orderCode(0), _priority(0), _preferredStationCode(0), _volume(0), _maxVolumeAllowed(false)
    , _dischargeRate(0), _pumpLength(0), _startTime(0),_unixTimeStamp(0), _isPickup(false), _constructionYard(){
  }
} ;

struct _Vehicle_{
  char* _vehicleCode;
  char* _vehicleType;
  int _pumpLength;
  int _normalVolume;
  int _maximumVolume;
  float _dischargeRate;
  char* _nextAvailableTime;
  time_t _nextAvailabelTimeStampUnix;
};

struct _Station_{
  char* _stationCode;
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
