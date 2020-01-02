#include "StateVector.h"
#include "NewtonKrylov.h"
#include <iomanip>

class CriticalPoint
{
public:
    StateVector x;
    StateVector v;
    stratifloat p;

    stratifloat Dot(const CriticalPoint& other) const
    {
        return x.Dot(other.x) + v.Dot(other.v) + p*other.p;
    }

    stratifloat Norm2() const
    {
        return Dot(*this);
    }

    stratifloat Norm() const
    {
        return sqrt(Norm2());
    }

    void MulAdd(stratifloat b, const CriticalPoint& A)
    {
        x.MulAdd(b,A.x);
        v.MulAdd(b,A.v);
        p += b*A.p;
    }

    const CriticalPoint& operator+=(const CriticalPoint& other)
    {
        x += other.x;
        v += other.v;
        p += other.p;
        return *this;
    }

    const CriticalPoint& operator-=(const CriticalPoint& other)
    {
        x -= other.x;
        v -= other.v;
        p -= other.p;
        return *this;
    }

    const CriticalPoint& operator*=(stratifloat mult)
    {
        x *= mult;
        v *= mult;
        p *= mult;
        return *this;
    }

    void Zero()
    {
        x.Zero();
        v.Zero();
        p = 0;
    }

    void SaveToFile(const std::string& filename) const
    {
        x.SaveToFile(filename+".fields");
        v.SaveToFile(filename+"-eig.fields");
        std::ofstream paramFile(filename+".params");
        paramFile << std::setprecision(30);
        paramFile << p;
    }

    void LoadFromFile(const std::string& filename)
    {
        x.LoadFromFile(filename+".fields");
        v.LoadFromFile(filename+"-eig.fields");
        std::ifstream paramFile(filename+".params");
        paramFile >> p;
    }

    template<int K1, int K2, int K3>
    void LoadAndInterpolate(const std::string& filename)
    {
        x.LoadAndInterpolate<K1,K2,K3>(filename+".fields");
        v.LoadAndInterpolate<K1,K2,K3>(filename+"-eig.fields");
        std::ifstream paramFile(filename+".params");
        paramFile >> p;
    }


    void PlotAll(std::string directory) const
    {
        MakeCleanDir(directory);
        x.PlotAll(directory+"/x");
        v.PlotAll(directory+"/v");
    }
};

class FindCriticalPoint : public NewtonKrylov<CriticalPoint>
{
public:
    stratifloat weight = 1;

    virtual void EnforceConstraints(CriticalPoint& at)
    {
        flowParams.Ri = at.p;

        // make the eigenvectors orthogonal to the symmetry
        StateVector phaseShift;
        phaseShift.u1 = ddx(at.x.u1);
        phaseShift.u2 = ddx(at.x.u2);
        phaseShift.u3 = ddx(at.x.u3);
        phaseShift.b = ddx(at.x.b);

        if (phaseShift.Norm2()!=0)
        {
            stratifloat proj = at.v.Dot(phaseShift)/phaseShift.Norm2();
            at.v.MulAdd(-proj, phaseShift);
        }

        // Remove any average in eigenvector (another symmetry)
        RemoveAverage(at.v.u1, flowParams.L3);
        RemoveAverage(at.v.b, flowParams.L3);

        at.v.Rescale(weight);
    }
private:
    virtual CriticalPoint EvalFunction(const CriticalPoint& at) override
    {
        CriticalPoint result;

        flowParams.Ri = at.p;
        at.x.FullEvolve(T, result.x, false, false);
        at.v.LinearEvolve(T, at.x, result.v);

        result -= at;
        result.p = at.v.Energy() - weight;

        std:: cout << result.x.Norm2() << " " << result.v.Norm2() << " " << result.p*result.p << std::endl;

        return result;
    }
};

#include "Arnoldi.h"
#include "ExtendedStateVector.h"

int main(int argc, char *argv[])
{
    flowParams.Re = 4000;
    flowParams.Pr = std::stof(argv[1]);
    DumpParameters();
    StateVector::ResetForParams();

    CriticalPoint guess;


    if (argc == 6)
    {
        CriticalPoint x1;
        CriticalPoint x2;
        x1.LoadFromFile(argv[2]);
        x2.LoadFromFile(argv[3]);

        stratifloat shift = x1.x.RemovePhaseShift();
        RemoveAverage(x1.x.u1, flowParams.L3);
        RemoveAverage(x1.x.b, flowParams.L3);
        x1.v.RemovePhaseShift(shift);
        RemoveAverage(x1.v.u1, flowParams.L3);
        RemoveAverage(x1.v.b, flowParams.L3);

        shift = x2.x.RemovePhaseShift();
        RemoveAverage(x2.x.u1, flowParams.L3);
        RemoveAverage(x2.x.b, flowParams.L3);
        x2.v.RemovePhaseShift(shift);
        RemoveAverage(x2.v.u1, flowParams.L3);
        RemoveAverage(x2.v.b, flowParams.L3);

        stratifloat Pr1 = std::stof(argv[4]);
        stratifloat Pr2 = std::stof(argv[5]);

        CriticalPoint gradient = x2;
        gradient -= x1;
        gradient *= 1/(Pr2-Pr1);

        guess = x1;
        guess.MulAdd(flowParams.Pr-Pr1, gradient);
    }
    else
    {
        guess.LoadFromFile(argv[2]);
        //guess.LoadAndInterpolate<128,1,768>(argv[2]);
    }


    flowParams.Ri = guess.p;

    FindCriticalPoint solver;

    // scale v
    solver.EnforceConstraints(guess);

    solver.Run(guess);

    guess.SaveToFile("final");
}
