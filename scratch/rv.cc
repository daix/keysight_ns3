#include "ns3/core-module.h"

using namespace ns3;

int main(int argc, char **argv){
	//Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable>();
	Ptr<ParetoRandomVariable> x = CreateObject<ParetoRandomVariable> ();
	x->SetAttribute ("Scale", DoubleValue (5));
	x->SetAttribute ("Shape", DoubleValue (2));
	for (unsigned i = 0; i < 100; ++i)
		std::cout << x->GetValue() << std::endl;
	return 0;
}
