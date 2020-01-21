#include "ExtendedStateVector.h"
#include "Arnoldi.h"

int main(int argc, char* argv[])
{
    flowParams.Ri = std::stod(argv[1]);
    flowParams.Pr = std::stod(argv[2]);
    StateVector::ResetForParams();
    DumpParameters();

    StateVector state;
    if (argc>3)
    {
        state.LoadFromFile(argv[3]);
    }

    if (argc>4)
    {
        StateVector state2;
        state2.LoadFromFile(argv[4]);

        stratifloat mult=1;

        if (argc>5)
        {
            mult = std::stod(argv[5]);
        }

        state = state2 + mult*(state2-state);
    }

    StateVector perturbation;
    perturbation.ExciteLowWavenumbers(0.0005);

    stratifloat k = 0.25;
    stratifloat m = 3;
    stratifloat omega = sqrt(flowParams.Ri*k*k/(k*k+m*m));

    std::cout << "Horizontal phase speed " << omega/k << std::endl;


    Nodal U1;
    U1.SetValue([=](float x, float y, float z){return omega*(m/k)*sin(k*x+m*z)+sin(z);}, flowParams.L1, flowParams.L2, flowParams.L3);
    U1.ToModal(state.u1);

    Nodal U3;
    U3.SetValue([=](float x, float y, float z){return -omega*sin(k*x+m*z);}, flowParams.L1, flowParams.L2, flowParams.L3);
    U3.ToModal(state.u3);

    Nodal B;
    B.SetValue([=](float x, float y, float z){return cos(k*x+m*z);}, flowParams.L1, flowParams.L2, flowParams.L3);
    B.ToModal(state.b);

    // if (argc == 3)
    state += perturbation;

    state.FullEvolve(20, state, true, true);

}
