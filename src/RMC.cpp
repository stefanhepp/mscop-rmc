
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
  : Deliveries(*this, input.getNumVehicles(), 0, input.getNumOrders() * input.getMaxDeliveries() - 1),
    D_Order(*this, input.getMaxTotalDeliveries(), 0, input.getNumOrders() - 1),
    D_Station(*this, input.getMaxTotalDeliveries(), 0, input.getNumStations() - 1),
    D_tLoad(*this, input.getMaxTotalDeliveries(), 0, input.getMaxTimeStamp()), 
    D_tUnload(*this, input.getMaxTotalDeliveries(), 0, input.getMaxTimeStamp()),
    Cost(*this, 0, Int::Limits::max),
    O_poured(*this, input.getNumOrders(), 0, Int::Limits::max),
    O_Deliveries(*this, input.getNumOrders(), 0, input.getMaxDeliveries())
  {
    int numD = input.getMaxDeliveries();
    int numV = input.getNumVehicles();
    int numO = input.getNumOrders();
    int numVD = numO * numD;
    int numOD = numV * numD;
    
    // Create a matrix view on all the delivery variables
    Matrix<IntVarArray> mD_Order(D_Order, numV, numVD);
    Matrix<IntVarArray> mD_Station(D_Station, numV, numVD);
    Matrix<IntVarArray> mD_tLoad(D_tLoad, numV, numVD);
    Matrix<IntVarArray> mD_tUnload(D_tUnload, numV, numVD);
    
    // Set boolean flags for all active deliveries
    BoolVarArgs D_Used(*this, numV * numVD, 0, 1);
    Matrix<BoolVarArgs> mD_Used(D_Used, numV, numVD);
    
    for (int i = 0; i < numV; i++) {
      for (int d = 0; d < numVD; d++) {
        rel(*this, mD_Used(i, d) == (d < Deliveries[i]));
      }
    }
    
    // Force all unused deliveries to some value
    for (int i = 0; i < numV; i++) {
      for (int d = 0; d < numVD; d++) {
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
    IntVarArgs D_dT_travelTo(*this, numV * numVD, 0, Int::Limits::max);
    Matrix<IntVarArgs> mD_dT_travelTo(D_dT_travelTo, numV, numVD);
    
    for (int i = 0; i < numV * numVD; i++) {
      rel(*this, (D_dT_travelTo[i] == element(O_dt_travelTo, D_Order[i] * numVD + D_Station[i]) && D_Used[i]) ||
                 (D_dT_travelTo[i] == 0 && !D_Used[i]));
    }
    
    // Time to travel back to station
    // We ignore the trip back from the last delivery.. since the time to travel back
    // only depends on the station to travel to, we can just assume we travel back to a fixed station and eliminate this value    
    IntVarArgs D_dT_travelFrom(*this, numV * numVD, 0, Int::Limits::max);
    Matrix<IntVarArgs> mD_dT_travelFrom(D_dT_travelFrom, numV, numVD);
    
    for (int i = 0; i < numV; i++) {
      for (int d = 1; d < numVD; d++) {
        rel(*this, (mD_dT_travelFrom(i, d-1) == element(O_dt_travelFrom, mD_Order(i, d-1) * numO + mD_Station(i,d)) && mD_Used(i,d)) ||
                   (mD_dT_travelFrom(i, d-1) == 0 && !mD_Used(i,d)));
      }
      rel(*this, mD_dT_travelFrom(i, numVD-1) == 0);
    }

    // Timestamp of arrival at yard
    IntVarArgs D_t_arrival(*this, numV * numVD, 0, Int::Limits::max);
    Matrix<IntVarArgs> mD_t_arrival(D_t_arrival, numV, numVD);
    
    for (int i = 0; i < numV * numVD; i++) {
      rel(*this, D_t_arrival[i] == D_tLoad[i] + element(S_tLoad, D_Station[i]) + D_dT_travelTo[i]);
    }

    // Amount of concrete delivered by a delivery 
    IntVarArgs D_delivered(*this, numV * numVD, 0, Int::Limits::max);
    Matrix<IntVarArgs> mD_delivered(D_delivered, numV, numVD);
    
    for (int i = 0; i < numV; i++) {
      for (int d = 0; d < numVD; d++) {
        rel(*this, (mD_delivered(i,d) == element(V_volumes, mD_Order(i, d) * numO + i) && mD_Used(i,d)) ||
                   (mD_delivered(i,d) == 0 && !mD_Used(i,d)));
      }
    }
    
    // Time required for unloading
    // TODO in case D_tUnload + D_dT_Unloading - D_tLoad > Tmax, we might unload faster, but we do not want this anyway.
    
    IntVarArgs D_dT_Unloading(*this, numV * numVD, 0, Int::Limits::max);
    Matrix<IntVarArgs> mD_dT_Unloading(D_dT_Unloading, numV, numVD);
    
    for (int i = 0; i < numV; i++) {
      for (int d = 0; d < numVD; d++) {
        rel(*this, mD_dT_Unloading(i, d) == mD_delivered(i,d) / element(O_reqDischargeRates, mD_Order(i, d)));
      }
    }
    
    // Amount of concrete poured by a delivery (excluding bad concrete)
    IntVarArgs D_poured(*this, numV * numVD, 0, Int::Limits::max);
    Matrix<IntVarArgs> mD_poured(D_poured, numV, numVD);
    
    for (int i = 0; i < numV; i++) {
      for (int d = 0; d < numVD; d++) {
        rel(*this, mD_poured(i,d) == min( mD_delivered(i,d), 
                                          (input.getTimeMax() - mD_tUnload(i,d) + mD_tLoad(i,d)) * 
                                              element(O_reqDischargeRates, mD_Order(i,d)) 
                                        ) );
      }
    }

    
    // Total amount poured per order
    for (int i = 0; i < numO; i++) {
      Order &o = input.getOrder(i);
      
      IntVarArgs volume(*this, numV * numVD, 0, Int::Limits::max);
      
      for (int d= 0; d < numV * numVD; d++) {
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
    IntVar tmpCount(*this, 0, numV * numVD);
    count(*this, D_Order, 0, IRT_EQ, tmpCount);
    rel(*this, O_Deliveries[0] == tmpCount - (numV * numVD - sum(D_Used)));
    
    
    /// ---- add constraints ----
    
    // Loading of vehicle i must not start before V_i.available
    for (int i = 0; i < numV; i++) {
      rel(*this, (mD_tLoad(i, 0) >= input.getVehicle(i).availableFrom()) || !mD_Used(i, 0));
    }
    
    // Unloading must not start before the order starts
    for (int d = 0; d < numV * numVD; d++) {
      rel(*this, D_tUnload[d] >= element(O_tStart, D_Order[d]) || !D_Used[d]);
    }
    
    // Vehicles start at station 0
    for (int i = 0; i < numV; i++) {
      rel(*this, mD_Station(i, 0) == 0);
    }
    
    // Vehicle must have required pipeline length and discharge rate for orders
    for (int i = 0; i < numV; i++) {
      Vehicle &v = input.getVehicle(i);
      for (int d = 0; d < numVD; d++) {
        rel(*this, element(O_reqPipeLengths, mD_Order(i,d)) <= v.pumpLength() || !mD_Used(i,d));
        rel(*this, element(O_reqDischargeRates, mD_Order(i,d)) <= v.maxDischargeRate() || !mD_Used(i,d));
      }
    }
    
    // Loading can only start after vehicle arrived back at the station
    for (int i = 0; i < numV; i++) {
      for (int d = 1; d < numVD; d++) {
        rel(*this, mD_tUnload(i,d-1) + mD_dT_Unloading(i,d-1) + mD_dT_travelFrom(i,d-1) <= mD_tLoad(i,d) || !mD_Used(i,d));
      }
    }
    
    // Unloading can only start after the vehicle arrived at the yard
    for (int i = 0; i < numV; i++) {
      for (int d = 0; d < numVD; d++) {
        rel(*this, mD_t_arrival(i,d) <= mD_tUnload(i,d) - element(O_dT_setup, D_Order[i]) || !mD_Used(i,d));
      }
    }
    
    // Only one vehicle can be loaded at a station at a time
    for (int i = 0; i < input.getNumStations(); i++) {
      Station &s = input.getStation(i);
      
      // True for every delivery loaded at station i
      BoolVarArgs AtStation(*this, numV * numVD, 0, 1);
      IntArgs LoadTime = IntArgs::create(numV * numVD, s.loadingMinutes(), 0);
      
      for (int d = 0; d < numV * numVD; d++) {
        rel(*this, AtStation[d] == (D_Station[d] == i));
      }
      
      unary(*this, D_tLoad, LoadTime, AtStation);
    }
    
    // Only one vehicle can be unloaded at a construction site at a time
    // TODO this should be per construction yard, not order
    for (int i = 0; i < numO; i++) {
      Order &o = input.getOrder(i);
      
      BoolVarArgs AtYard(*this, numV * numVD, 0, 1);
      IntVarArgs D_t_unloaded(*this, numV * numVD, 0, Int::Limits::max);
      
      for (int d = 0; d < numV * numVD; d++) {
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
    BoolVarArgs Preferred(*this, numV * numVD, 0, 1);
    
    for (int d = 0; d < numV * numVD; d++) {
      rel(*this, Preferred[d] == (D_Station[d] == element(O_preferredStation, D_Order[d])));
    }
    
    
    // Calculate lateness of first delivery and time lag of other deliveries
    IntVarArgs O_Lateness(*this, numO, 0, Int::Limits::max);
    IntVarArgs O_tLag(*this, numO * numOD, 0, Int::Limits::max);
    Matrix<IntVarArgs> mO_tLag(O_tLag, numO, numOD); 
    
    // First create an array containing unloading start times per order
    IntVarArgs O_tUnload(*this, numO * numOD, 0, Int::Limits::max);
    Matrix<IntVarArgs> mO_tUnload(O_tUnload, numO, numOD);
    
    // Enforce sorting of O_tUnload
    for (int o = 0; o < numO; o++) {
      for (int d = 1; d < numOD; d++) {
        rel(*this, (mO_tUnload(o, d-1) < mO_tUnload(o,d) || (d >= O_Deliveries[o])));
      }
    }
    
    // Create a permutation of D_tUnload onto O_tUnload
    IntVarArgs ODMap(*this, numO * numOD, 0, numV * numVD - 1);
    Matrix<IntVarArgs> mODMap(ODMap, numO, numOD);
        
    // - All values must be distinct
    distinct(*this, ODMap);
    
    // - Map to deliveries from same order
    for (int i = 0; i < numO; i++) {
      for (int d = 0; d < numOD; d++) {
        rel(*this, element(D_Order, mODMap(i,d)) == i || d > O_Deliveries[i]);
      }
    }
    
    // - Break symmetries for unused deliveries
    for (int i = 0; i < numO; i++) {
      for (int d = 1; d < numOD; d++) {
        // ODMap[o, d-1] < ODMap[o, d] if !Used[d-1]
        rel(*this, mODMap(i, d-1) < mODMap(i, d) || d-1 < O_Deliveries[i]);
      }      
    }
    for (int i = 1; i < numO; i++) {
      // ODMap[o-1, max] < ODMap[o, min(unused)]
      rel(*this, mODMap(i-1, numOD-1) < element(ODMap, i * numOD + O_Deliveries[i]) || 
                 O_Deliveries[i-1] == numOD-1 || O_Deliveries[i] == numOD-1);
    }
    
    
    // Define Lateness per order and TimeLags per order over order unloading times
    for (int i = 0; i < numO; i++) {
      Order &o = input.getOrder(i);
      rel(*this, O_Lateness[i] == mO_tUnload(i, 0) - o.timeStart());
      
      for (int d = 1; d < numOD; d++) {
        rel(*this, ((mO_tLag(i,d) == mO_tUnload(i, d) - mO_tUnload(i, d-1) - element(D_dT_Unloading, mODMap(i, d))) && (d < O_Deliveries[i])) ||
                   ((mO_tLag(i,d) == 0) && (d >= O_Deliveries[i])) );
      }
    }
    
    // Total costs
    rel(*this, Cost == sum(O_Lateness) * input.getAlpha1() + sum(Waste) * input.getAlpha2() +
                       sum(Preferred) * input.getAlpha3() + sum(O_tLag) * input.getAlpha4() +
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
    std::cout << "Number of fails: " << bab.statistics().fail << "\n";
    std::cout << "Number of nodes: " << bab.statistics().node << "\n";
    std::cout << "Max depth: " << bab.statistics().depth << "\n";
    std::cout << "Max memory: " << bab.statistics().memory << "\n";
  }
  
  return 0;
}
