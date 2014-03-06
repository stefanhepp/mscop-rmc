
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
  
  
public:
  /// problem construction

  RMC(RMCInput &input)
  : Deliveries(*this, input.getNumVehicles(), 0, input.getMaxDeliveries() - 1),
    D_Order(*this, input.getNumVehicles() * input.getMaxDeliveries(), 0, input.getNumOrders() - 1),
    D_Station(*this, input.getNumVehicles() * input.getMaxDeliveries(), 0, input.getNumStations() - 1),
    D_tLoad(*this, input.getNumVehicles() * input.getMaxDeliveries(), 0, input.getMaxTimeStamp()), 
    D_tUnload(*this, input.getNumVehicles() * input.getMaxDeliveries(), 0, input.getMaxTimeStamp()),
    Cost(*this, 0, Int::Limits::max)
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
    
    // Total volume to pour per order
    IntArgs O_totalVolumes(input.getNumOrders(), input.getOrderTotalVolumes());
    
    // Volume of vehicles per order
    IntArgs V_volumes(input.getNumOrders() * input.getNumVehicles(), input.getOrderVehicleVolumes());
    Matrix<IntArgs> mV_volumes(V_volumes, input.getNumOrders(), input.getNumVehicles());
    
    // Travel time from stations to yards
    IntArgs O_dt_travelTo(input.getNumOrders() * input.getNumStations(), input.getTravelTimesToYards());
    Matrix<IntArgs> mO_dt_travelTo(O_dt_travelTo, input.getNumOrders(), input.getNumStations());
    
    // Travel time from yards to stations
    IntArgs O_dt_travelFrom(input.getNumOrders() * input.getNumStations(), input.getTravelTimesFromYards());
    Matrix<IntArgs> mO_dt_travelFrom(O_dt_travelFrom, input.getNumOrders(), input.getNumStations());    
    
    // Station load times
    IntArgs S_tLoad(input.getNumStations(), input.getStationLoadTimes());
    
    
    
    // Timestamp of arrival at yard
    IntVarArgs D_t_arrival(*this, numV * numD, 0, Int::Limits::max);
    Matrix<IntVarArgs> mD_t_arrival(D_t_arrival, numV, numD);
    
    for (int i = 0; i < numV * numD; i++) {
      rel(*this, D_t_arrival[i] == D_tLoad[i] + element(S_tLoad, D_Station[i]) +
                                   element(O_dt_travelTo, D_Order[i] * numO + D_Station[i]));
    }
    
    // Time required for unloading
    IntVarArgs D_dT_Unloading(*this, numV * numD, 0, Int::Limits::max);
    Matrix<IntVarArgs> mD_dT_Unloading(D_dT_Unloading, numV, numD);
    
    for (int i = 0; i < numV; i++) {
      for (int d = 0; d < numD; d++) {
        rel(*this, mD_dT_Unloading(i, d) == element(V_volumes, mD_Order(i, d) * numO + i) /
                                            element(O_reqDischargeRates, mD_Order(i, d)));
      }
    }
    
    
    /// ---- add constraints ----
    
    // First delivery of vehicle i must not start before V_i.available
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
    
    // Loading can only start after vehicle arrived back at the station
    
    // Unloading can only start after the vehicle arrived at the yard
    
    
    
    
    /// ------ define cost function ----
    
    
    
    // Temporary for now
    rel(*this, Cost == 0);
    
    
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
    
    // Is this needed??
    Cost.update(*this, share, rmc.Cost);
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
    std::cout << "Solution:\n";
    std::cout << Deliveries << std::endl;
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
  
  //Gist::bab(rmc);
  BAB<RMC> bab(rmc);
  
  delete rmc;
  
  while (rmc = bab.next()) {
    rmc->print();
  }
  
  return 0;
}
