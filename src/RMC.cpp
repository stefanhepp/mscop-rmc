
#include "Problem.hpp"

#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include <gecode/driver.hh>

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
    
    // Create a matrix view on all the delivery variables
    Matrix<IntVarArray> mD_Order(D_Order, numD, numV);
    Matrix<IntVarArray> mD_Station(D_Station, numD, numV);
    Matrix<IntVarArray> mD_tLoad(D_tLoad, numD, numV);
    Matrix<IntVarArray> mD_tUnload(D_tUnload, numD, numV);
    
    // Create an array of start times of orders
    IntArgs O_tStart(input.getNumOrders(), input.getOrderStartTimes() );
    
    // Set boolean flags for all active deliveries
    BoolVarArgs D_Used(*this, numD * numV, 0, 1);
    Matrix<BoolVarArgs> mD_Used(D_Used, numD, numV);
    
    for (int i = 0; i < numV; i++) {
      for (int d = 0; d < numD; d++) {
        rel(*this, mD_Used(d, i) == (d < Deliveries[i]));
      }
    }

    /// ---- add constraints ----
    
    // Force all unused deliveries to some value
    for (int i = 0; i < numV; i++) {
      for (int d = 0; d < numD; d++) {
        rel(*this, mD_Used(d, i) || (mD_Order(d, i) == 0));
        rel(*this, mD_Used(d, i) || (mD_Station(d, i) == 0));
        rel(*this, mD_Used(d, i) || (mD_tLoad(d, i) == 0));
        rel(*this, mD_Used(d, i) || (mD_tUnload(d, i) == 0));
      }
    }
    
    // First delivery of vehicle i must not start before V_i.available
    for (int i = 0; i < numV; i++) {
      rel(*this, (mD_tLoad(0, i) >= input.getVehicle(i).availableFrom()) || !mD_Used(0, i));
    }
    
    // Unloading must not start before the order starts
    for (int d = 0; d < numV * numD; d++) {
      rel(*this, D_tUnload[d] >= element(O_tStart, D_Order[d]) || !D_Used[d]);
    }
    
    
    
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
  
  BAB<RMC> bab(rmc);
  
  delete rmc;
  
  while (rmc = bab.next()) {
    rmc->print();
  }
  
  return 0;
}
