
#include "Problem.hpp"

#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include <gecode/driver.hh>

#include <gecode/gist.hh>

#include <iostream>
#include <vector>

using namespace Gecode;

class RMC : public MinimizeSpace {
  
protected:
  
  // ------------- Decision Variables ----------------
  
  // Number of deliveries per vehicle
  IntVarArray Deliveries;
  
  // Order number per delivery (i * numV + d)
  IntVarArray D_Order;
  
  // Index of start station
  IntVarArray D_Station;
  
  // Timestamp when loading starts for delivery d
  IntVarArray D_tLoad;
  
  // Timestamp when unloading starts for delivery d
  IntVarArray D_tUnload;
  
  // --------------- Optimization Goal ---------------
  
  // Cost function value
  IntVar Cost;
  
  // --------------- Result values -------------------
  
  // Total amount of concrete poured per order
  IntVarArray O_poured;

  // Amount of deliveries per order
  IntVarArray O_Deliveries;
  
public:
  /// problem construction

  RMC(RMCInput &input)
  : Deliveries(*this, input.getNumVehicles(), 0, input.getMaxDeliveries() - 1),
    D_Order(*this, input.getNumVehicles() * input.getMaxDeliveries(), 0, input.getNumOrders() - 1),
    D_Station(*this, input.getNumVehicles() * input.getMaxDeliveries(), 0, input.getNumStations() - 1),
    D_tLoad(*this, input.getNumVehicles() * input.getMaxDeliveries(), 0, input.getMaxTimeStamp()), 
    D_tUnload(*this, input.getNumVehicles() * input.getMaxDeliveries(), 0, input.getMaxTimeStamp()),
    Cost(*this, 0, Int::Limits::max),
    O_poured(*this, input.getNumOrders(), 0, Int::Limits::max),
    O_Deliveries(*this, input.getNumOrders(), 0, input.getMaxDeliveries())
  {
    int numD = input.getMaxDeliveries();
    int numV = input.getNumVehicles();
    int numO = input.getNumOrders();
    
    // Create a matrix view on all the delivery variables
    Matrix<IntVarArray> mD_Order(D_Order, numV, numD);
    Matrix<IntVarArray> mD_Station(D_Station, numV, numD);
    Matrix<IntVarArray> mD_tLoad(D_tLoad, numV, numD);
    Matrix<IntVarArray> mD_tUnload(D_tUnload, numV, numD
    );
    
    // Set boolean flags for all active deliveries
    BoolVarArgs D_Used(*this, numV * numD, 0, 1);
    Matrix<BoolVarArgs> mD_Used(D_Used, numV, numD);
    
    for (int i = 0; i < numV; i++) {
      for (int d = 0; d < numD; d++) {
        rel(*this, mD_Used(i, d) == (d < Deliveries[i]));
      }
    }
    
    // Force all unused deliveries to some value
    for (int i = 0; i < numV; i++) {
      for (int d = 0; d < numD; d++) {
        rel(*this, mD_Used(i, d) || (mD_Order(i, d) == 0));
        rel(*this, mD_Used(i, d) || (mD_Station(i, d) == 0));
        rel(*this, mD_Used(i, d) || (mD_tLoad(i, d) == 0));
        rel(*this, mD_Used(i, d) || (mD_tUnload(i, d) == 0));
      }
    }
    
    /// ----- helper variables per delivery -----

    // Start times of orders
    IntArgs O_tStart(input.getNumOrders(), input.getOrderStartTimes() );
    
    // Required discharge rates of orders
    IntArgs O_reqDischargeRates(input.getNumOrders(), input.getOrderReqDischargeRates());
    
    // Required volumes of orders
    IntArgs O_reqPipeLengths(input.getNumOrders(), input.getOrderReqPipeLengths());
    
    IntArgs O_preferredStation(input.getNumOrders(), input.getOrderPreferredStations());
    
    // Setup time per order
    IntArgs O_dT_setup(input.getNumOrders(), input.getOrderSetupTimes());
    
    // Total volume to pour per order
    IntArgs O_totalVolumes(input.getNumOrders(), input.getOrderTotalVolumes());
    
    // Volume of vehicles per order
    IntArgs V_volumes(input.getNumOrders() * input.getNumVehicles(), input.getOrderVehicleVolumes());
    
    // Travel time from stations to yards
    IntArgs O_dt_travelTo(input.getNumOrders() * input.getNumStations(), input.getTravelTimesToYards());
    
    // Travel time from yards to stations
    IntArgs O_dt_travelFrom(input.getNumOrders() * input.getNumStations(), input.getTravelTimesFromYards());
    
    // Station load times
    IntArgs S_tLoad(input.getNumStations(), input.getStationLoadTimes());
        
    
    // Time to travel to yard
    IntVarArgs D_dT_travelTo(*this, numV * numD, 0, Int::Limits::max);
    Matrix<IntVarArgs> mD_dT_travelTo(D_dT_travelTo, numV, numD);
    
    for (int i = 0; i < numV * numD; i++) {
      rel(*this, (D_dT_travelTo[i] == element(O_dt_travelTo, D_Order[i] * numO + D_Station[i]) && D_Used[i]) ||
                 (D_dT_travelTo[i] == 0 && !D_Used[i]));
    }
    
    // Time to travel back to station
    // We ignore the trip back from the last delivery.. since the time to travel back
    // only depends on the station to travel to, we can just assume we travel back to a fixed station and eliminate this value    
    IntVarArgs D_dT_travelFrom(*this, numV * numD, 0, Int::Limits::max);
    Matrix<IntVarArgs> mD_dT_travelFrom(D_dT_travelFrom, numV, numD);
    
    for (int i = 0; i < numV; i++) {
      for (int d = 1; d < numD; d++) {
        rel(*this, (mD_dT_travelFrom(i, d-1) == element(O_dt_travelFrom, mD_Order(i, d-1) * numO + mD_Station(i,d)) && mD_Used(i,d)) ||
                   (mD_dT_travelFrom(i, d-1) == 0 && !mD_Used(i,d)));
      }
      rel(*this, mD_dT_travelFrom(i, numD-1) == 0);
    }

    // Timestamp of arrival at yard
    IntVarArgs D_t_arrival(*this, numV * numD, 0, Int::Limits::max);
    Matrix<IntVarArgs> mD_t_arrival(D_t_arrival, numV, numD);
    
    for (int i = 0; i < numV * numD; i++) {
      rel(*this, D_t_arrival[i] == D_tLoad[i] + element(S_tLoad, D_Station[i]) + D_dT_travelTo[i]);
    }

    // Amount of concrete delivered by a delivery 
    IntVarArgs D_delivered(*this, numV * numD, 0, Int::Limits::max);
    Matrix<IntVarArgs> mD_delivered(D_delivered, numV, numD);
    
    for (int i = 0; i < numV; i++) {
      for (int d = 0; d < numD; d++) {
        rel(*this, (mD_delivered(i,d) == element(V_volumes, mD_Order(i, d) * numO + i) && mD_Used(i,d)) ||
                   (mD_delivered(i,d) == 0 && !mD_Used(i,d)));
      }
    }
    
    // Time required for unloading
    // TODO in case D_tUnload + D_dT_Unloading - D_tLoad > Tmax, we might unload faster, but we do not want this anyway.
    
    IntVarArgs D_dT_Unloading(*this, numV * numD, 0, Int::Limits::max);
    Matrix<IntVarArgs> mD_dT_Unloading(D_dT_Unloading, numV, numD);
    
    for (int i = 0; i < numV; i++) {
      for (int d = 0; d < numD; d++) {
        rel(*this, mD_dT_Unloading(i, d) == mD_delivered(i,d) / element(O_reqDischargeRates, mD_Order(i, d)));
      }
    }
    
    // Amount of concrete poured by a delivery (excluding bad concrete)
    IntVarArgs D_poured(*this, numV * numD, 0, Int::Limits::max);
    Matrix<IntVarArgs> mD_poured(D_poured, numV, numD);
    
    for (int i = 0; i < numV; i++) {
      for (int d = 0; d < numD; d++) {
        rel(*this, mD_poured(i,d) == min( mD_delivered(i,d), 
                                          (input.getTimeMax() - mD_tUnload(i,d) + mD_tLoad(i,d)) * 
                                              element(O_reqDischargeRates, mD_Order(i,d)) 
                                        ) );
      }
    }

    
    // Total amount poured per order
    for (int i = 0; i < numO; i++) {
      Order &o = input.getOrder(i);
      
      IntVarArgs volume(*this, numV * numD, 0, Int::Limits::max);
      
      for (int d= 0; d < numV * numD; d++) {
        // Only in Gecode 4.2
        // ite(*this, D_Used[d], D_poured[d], 0, volume[d]);
        
        rel(*this, (volume[d] == D_poured[d] && D_Used[d]) ||
                   (volume[d] == 0 && !D_Used[d]));
      }
      
      rel(*this, O_poured[i] == sum(volume));
    }

    
    // Deliveries per order
    for (int i = 1; i < numO; i++) {
      count(*this, D_Order, i, IRT_EQ, O_Deliveries[i]);
    }
    // Order 0 is special, need to substract all unused deliveries
    IntVarArgs tmpCount(*this, 2, 0, numD * numD);
    count(*this, D_Order, 0, IRT_EQ, tmpCount[0]);
    rel(*this, O_Deliveries[0] == tmpCount[0] - (numD * numV - sum(D_Used)));
    
    
    /// ---- add constraints ----
    
    // Loading of vehicle i must not start before V_i.available
    for (int i = 0; i < numV; i++) {
      rel(*this, (mD_tLoad(i, 0) >= input.getVehicle(i).availableFrom()) || !mD_Used(i, 0));
    }
    
    // Unloading must not start before the order starts
    for (int d = 0; d < numD * numV; d++) {
      rel(*this, D_tUnload[d] >= element(O_tStart, D_Order[d]) || !D_Used[d]);
    }
    
    // Vehicles start at station 0
    for (int i = 0; i < numV; i++) {
      rel(*this, mD_Station(i, 0) == 0);
    }
    
    // Vehicle must have required pipeline length and discharge rate for orders
    for (int i = 0; i < numV; i++) {
      Vehicle &v = input.getVehicle(i);
      for (int d = 0; d < numD; d++) {
        rel(*this, element(O_reqPipeLengths, mD_Order(i,d)) <= v.pumpLength() || !mD_Used(i,d));
        rel(*this, element(O_reqDischargeRates, mD_Order(i,d)) <= v.maxDischargeRate() || !mD_Used(i,d));
      }
    }
    
    // Loading can only start after vehicle arrived back at the station
    for (int i = 0; i < numV; i++) {
      for (int d = 1; d < numD; d++) {
        rel(*this, mD_tUnload(i,d-1) + mD_dT_Unloading(i,d-1) + mD_dT_travelFrom(i,d-1) <= mD_tLoad(i,d) || !mD_Used(i,d));
      }
    }
    
    // Unloading can only start after the vehicle arrived at the yard
    for (int i = 0; i < numV; i++) {
      for (int d = 0; d < numD; d++) {
        rel(*this, mD_t_arrival(i,d) <= mD_tUnload(i,d) - element(O_dT_setup, D_Order[i]) || !mD_Used(i,d));
      }
    }
    
    // Only one vehicle can be loaded at a station at a time
    for (int i = 0; i < input.getNumStations(); i++) {
      Station &s = input.getStation(i);
      
      // True for every delivery loaded at station i
      BoolVarArgs AtStation(*this, numV * numD, 0, 1);
      IntArgs LoadTime(numV * numD);
      
      for (int d = 0; d < numV * numD; d++) {
        rel(*this, AtStation[d] == (D_Station[d] == i));
        
        LoadTime[d] = s.loadingMinutes();
      }
      
      unary(*this, D_tLoad, LoadTime, AtStation);
    }
    
    // Only one vehicle can be unloaded at a construction site at a time
    // TODO this should be per construction yard, not order
    for (int i = 0; i < numO; i++) {
      Order &o = input.getOrder(i);
      
      BoolVarArgs AtYard(*this, numV * numD, 0, 1);
      IntVarArgs D_t_unloaded(*this, numV * numD, 0, Int::Limits::max);
      
      for (int d = 0; d < numV * numD; d++) {
        rel(*this, AtYard[d] == (D_Order[d] == i));
        
        rel(*this, D_t_unloaded[d] == D_tUnload[d] + D_dT_Unloading[d]);
      }
      
      unary(*this, D_tUnload, D_dT_Unloading, D_t_unloaded, AtYard);
    }
    
    // All orders must be fullfilled
    for (int i = 0; i < numO; i++) {
      Order &o = input.getOrder(i);
      rel(*this, O_poured[i] >= o.totalVolume());
    }
    
    
    /// ------ define cost function ----
    
    // Calculate waste
    IntVarArgs Waste(*this, numO, 0, Int::Limits::max);
    
    for (int i = 0; i < numO; i++) {
      Order &o = input.getOrder(i);
      
      rel(*this, Waste[i] == O_poured[i] - o.totalVolume());
    }
    
    // Calculate preferred stations
    BoolVarArgs Preferred(*this, numV * numD, 0, 1);
    
    for (int d = 0; d < numV * numD; d++) {
      rel(*this, Preferred[d] == (D_Station[d] == element(O_preferredStation, D_Order[d])));
    }
    
    // Calculate lateness of first delivery and time lag of other deliveries
    IntVarArgs Lateness(*this, numO, 0, Int::Limits::max);
    IntVarArgs TimeLag(*this, numV * numD, 0, Int::Limits::max);

    // First create an array containing unloading start times per order
    IntVarArgs O_tUnload(*this, numV * numD, 0, Int::Limits::max);
    Matrix<IntVarArgs> mO_tUnload(O_tUnload, numV, numD);
    
    // Enforce sorting
    for (int i = 0; i < numV; i++) {
      for (int d = 1; d < numD; d++) {
//        rel(*this, (mO_tUnload(i, d-1) < mO_tUnload(i,d) || (d >= O_Deliveries[
      }
    }
    
    for (int i = 0; i < numO; i++) {
      rel(*this, Lateness[i] == 0);
      rel(*this, TimeLag[i] == 0);
    }
    
    // Total costs
    rel(*this, Cost == sum(Lateness) * input.getAlpha1() + sum(Waste) * input.getAlpha2() +
                       sum(Preferred) * input.getAlpha3() + sum(TimeLag) * input.getAlpha4() +
                       (sum(D_dT_travelTo) + sum(D_dT_travelFrom)) * input.getAlpha5());
    
    
    /// ----------- branching -----------
    
    branch(*this, Deliveries, INT_VAR_REGRET_MIN_MIN, INT_VAL_MIN);
    branch(*this, D_Order,    INT_VAR_REGRET_MIN_MIN, INT_VAL_MIN);
    branch(*this, D_Station,  INT_VAR_REGRET_MIN_MIN, INT_VAL_MIN);
    branch(*this, D_tLoad,    INT_VAR_REGRET_MIN_MIN, INT_VAL_MIN);
    branch(*this, D_tUnload,  INT_VAR_REGRET_MIN_MIN, INT_VAL_MIN);
  }

  virtual ~RMC() {}

  /// copy support
  
  RMC(bool share, RMC &rmc) 
  : MinimizeSpace(share, rmc) 
  {
    Deliveries.update(*this, share, rmc.Deliveries);
    D_Order.update(*this, share, rmc.D_Order);
    D_Station.update(*this, share, rmc.D_Station);
    D_tLoad.update(*this, share, rmc.D_tLoad);
    D_tUnload.update(*this, share, rmc.D_tUnload);
    Cost.update(*this, share, rmc.Cost);
    O_poured.update(*this, share, rmc.O_poured);
    O_Deliveries.update(*this, share, rmc.O_Deliveries);
  }

  virtual Space* copy(bool share) {
    return new RMC(share, *this);
  }
  
  /// optimisation
  
  virtual IntVar cost(void) const {
    return Cost;
  }
  
  /// printing 
  
  void print() {
    // TODO print out per vehicle, skip unused deliveries
    std::cout << "Orders per delivery:\n";
    std::cout << D_Order << std::endl;
    std::cout << "Stations per delivery:\n";
    std::cout << D_Station << std::endl;
    std::cout << "Load Times:\n";
    std::cout << D_tLoad << std::endl;
    std::cout << "Unload Times:\n";
    std::cout << D_tUnload << std::endl;
    
    std::cout << "Number of deliveries per order:\n";
    std::cout << O_Deliveries << std::endl;
    std::cout << "Number of deliveries per vehicle:\n";
    std::cout << Deliveries << std::endl;
    std::cout << "Concrete poured per order:\n";
    std::cout << O_poured << std::endl;
    std::cout << "Cost: " << Cost << std::endl;
  }

};

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Missing input file.\n";
    return 1;
  }
  
  RMCInput input;
  
  input.loadProblem(argv[1]);
  
  RMC *rmc = new RMC(input);

  Gist::bab(rmc);
  BAB<RMC> bab(rmc);
  
  delete rmc;
  
  while (rmc = bab.next()) {
    rmc->print();
  }
  
  return 0;
}
