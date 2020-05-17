#pragma once

template<typename VectorType>
class NewtonKrylov
{
public:
    NewtonKrylov()
    : q(K)
    , H(K, K-1)
    {
        H.setZero();
    }

    // using the GMRES routine, perform Newton-Raphson iteration
    // x : an initial guess, also the result when finished
    void Run(VectorType& x)
    {
        MakeCleanDir("ICs");

        VectorType dx; // update, solve into here to save reallocating memory

        stratifloat bestResidual;

        VectorType xPrevious;
        VectorType linStartPrevious;
        VectorType linEndPrevious;
        VectorType rhsPrevious;

        stratifloat Delta = 100;
        int step = 0;
        stratifloat targetResidual = 1e-7;
        int worsefor = 0;
        while(true)
        {
            step++;

            x.SaveToFile("ICs/"+std::to_string(step));
            x.PlotAll("ICs/"+std::to_string(step));

            // first nonlinearly evolve current state
            VectorType rhs = EvalFunction(x);
            linearAboutStart = x;
            linearAboutEnd = rhs;
            stratifloat residual = rhs.Norm();

            std::cout << "NEWTON STEP " << step << ", RESIDUAL: " << residual << std::endl;

            if (true)//residual < bestResidual || step == 1)
            {
                if (residual < targetResidual)
                {
                    return;
                }

                bestResidual = residual;

                xPrevious = x;
                linStartPrevious = linearAboutStart;
                linEndPrevious = linearAboutEnd;
                rhsPrevious = rhs;

                worsefor = 0;
            }
            else
            {
                worsefor++;
            }

            if (false)//worsefor > 0)
            {
                // not good enough, reduce trust region size and retry
                Delta /= 2;
                std::cout << "Delta: " << Delta << std::endl;

                // reset snapshots to previous
                // the time spent on this is small compared to GMRES time
                x = xPrevious;
                linearAboutStart = linStartPrevious;
                linearAboutEnd = linEndPrevious;
                rhs = rhsPrevious;

                // if (worsefor == 1)
                // {
                //    vectorsToReuse = 0;
                //    H.setZero();
                // }
            }
            else
            {
                // will have to construct Krylov space from scratch
                vectorsToReuse = 0;
                H.setZero();
            }

            // solve matrix system
            GMRES(rhs, dx, 0.01, Delta);

            // update
            x += dx;

            // do this a few times, as relative precision can mean it gets better
            EnforceConstraints(x);
            EnforceConstraints(x);
            EnforceConstraints(x);
        }
    }

    virtual void EnforceConstraints(VectorType& at) {}

protected:
    virtual VectorType EvalFunction(const VectorType& at) = 0;

    stratifloat T = 11; // time interval for integration

private:
    VectorType linearAboutStart;
    VectorType linearAboutEnd;

    VectorType EvalDerivative(const VectorType& at)
    {
        const stratifloat eps = 1e-7*linearAboutStart.Norm()/at.Norm();
        VectorType temp = linearAboutStart;

        temp.MulAdd(eps, at);
        temp = EvalFunction(temp);

        temp -= linearAboutEnd;

        temp *= 1/eps;

        return temp;
    }

    // solves A x = G-x0 for x
    // where A = I-G_x
    // GMRES is a Krylov-subspace method, hence Newton-Krylov
    // Delta is a maximum size for x in the least squares solution
    void GMRES(const VectorType& rhs, VectorType& x, stratifloat epsilon, stratifloat Delta=0)
    {
        VectorX y; // result in new basis

        q[0] = rhs;
        q[0].EnforceBCs();

        stratifloat beta = q[0].Norm();
        std::cout << beta << std::endl;
        q[0] *= 1/beta;

        K = q.size();
        for (int k=1; k<K; k++)
        {
            if (k > vectorsToReuse) // if we need to construct this basis vector
            {
                vectorsToReuse = k;

                // Arnoldi Algorithm
                // find orthogonal basis q1,...,qn
                // from x, A x, A^2 x, ...

                // q_k = A q_k-1
                q[k] = EvalDerivative(q[k-1]);
                q[k] *= -1.0; // factor of -1 for Newton iteration

                // remove component in direction of preceding vectors
                for (int j=0; j<k; j++)
                {
                    H(j,k-1) = q[j].Dot(q[k]);
                    q[k].MulAdd(-H(j,k-1), q[j]);
                }

                // normalise
                H(k,k-1) = q[k].Norm();
                q[k] *= 1/H(k,k-1);

                // enforce BCs
                q[k].EnforceBCs();
            }

            // Construct least squares problem in this basis
            VectorX Beta(k+1);
            Beta.setZero();
            Beta[0] = beta;

            MatrixX subH = H.block(0,0,k+1,k);

            // Now we solve Hy = Beta

            // follows notation of Chandler & Kerswell 2013

            // first H = UDV*
            JacobiSVD<MatrixX> svd(subH, ComputeFullU | ComputeFullV);
            MatrixX U = svd.matrixU();
            MatrixX V = svd.matrixV();
            ArrayX d = svd.singularValues();

            // solve problem in space of singular vectors
            VectorX p = U.transpose() * Beta; // p = U* Beta
            VectorX z = p.array()/d; // D z = p

            // enforce trust region
            stratifloat mu = 0;
            while (z.norm() > Delta)
            {
                mu += 0.00001;

                for (int j=0; j<z.size(); j++)
                {
                    z(j) = p(j)*d(j)/(d(j)*d(j)+mu);
                }
            }

            std::cout << "For |z|<Delta, mu=" << mu << std::endl;

            // z = V* y
            y = V*z;


            stratifloat residual = (subH*y - Beta).norm()/beta;

            std::cout << "GMRES STEP " << k << ", RESIDUAL: " << residual << std::endl;

            if (residual < epsilon)
            {
                K = k+1;
                break;
            }
        }

        // Now compute the solution using the basis vectors
        x.Zero();
        for (int k=0; k<K-1; k++)
        {
            x.MulAdd(y[k], q[k]);
        }
    }

    int K = 2048; // max iterations
    int vectorsToReuse = 0;
    std::vector<VectorType> q;
    MatrixX H; // upper Hessenberg matrix
};
