
#include "Problem.hpp"

#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include <gecode/driver.hh>

#include <iostream>

using namespace Gecode;

class RMC : public MinimizeSpace {
  
protected:
    IntVarArray P;
    
    IntVar Cost;
public:
    /// problem construction
  
    RMC(RMCInput &input) 
      : P(*this, 1)
    {
      
      
    }
  
    virtual ~RMC() {}
  
    /// copy support
    
    RMC(bool share, RMC &rmc) 
    : MinimizeSpace(share, rmc) 
    {
      P.update(*this, share, rmc.P);
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
      std::cout << P << std::endl;
      std::cout << "Num Shifts: " << Cost << std::endl;
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
  
  BAB<RMC> dfs(rmc);
  
  delete rmc;
  
  while (rmc = dfs.next()) {
    rmc->print();
  }
  
  return 0;
}
