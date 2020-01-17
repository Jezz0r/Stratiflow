#pragma once

#include "Constants.h"
#include "Eigen.h"
#include "FFT.h"

#include <cassert>

#include <vector>
#include <utility>
#include <functional>
#include <iostream>
#include <iterator>
#include <fstream>
#include <random>


ArrayX FourierPoints(stratifloat L, int N);

template<typename A, typename T, int N1, int N2, int N3>
class StackContainer
{
public:
    virtual A stack(int n1, int n2) const = 0;
};

template<typename A, typename T, int N1, int N2, int N3>
class Negate : public StackContainer<CwiseUnaryOp<internal::scalar_opposite_op<T>, const A>, T, N1, N2, N3>
{
public:
    Negate(const StackContainer<A, T, N1, N2, N3>* field)
    : field(field)
    {}

    virtual
        CwiseUnaryOp<internal::scalar_opposite_op<T>, const A>
        stack(int n1, int n2) const override
    {
        return -field->stack(n1, n2);
    }

private:
    const StackContainer<A, T, N1, N2, N3>* field;
};

template<typename A, typename T, int N1, int N2, int N3>
class ScalarProduct : public StackContainer<CwiseBinaryOp<internal::scalar_product_op<T, T>, const CwiseNullaryOp<internal::scalar_constant_op<T>,const Array<T, -1, 1>>, const A>, T, N1, N2, N3>
{
public:
    ScalarProduct(T scalar, const StackContainer<A, T, N1, N2, N3>* other)
    : scalar(scalar)
    , rhs(other)
    {}

    virtual
        CwiseBinaryOp<internal::scalar_product_op<T, T>, const CwiseNullaryOp<internal::scalar_constant_op<T>,const Array<T, -1, 1>>, const A>
        stack(int n1, int n2) const override
    {
        return scalar*rhs->stack(n1, n2);
    }

private:
    const StackContainer<A, T, N1, N2, N3>* rhs;
    T scalar;
};

template<typename A, typename B, typename T, int N1, int N2, int N3>
class ComponentwiseSum : public StackContainer<CwiseBinaryOp<internal::scalar_sum_op<T, T>, const A, const B>, T, N1, N2, N3>
{
public:
    ComponentwiseSum(const StackContainer<A, T, N1, N2, N3>* lhs, const StackContainer<B, T, N1, N2, N3>* rhs)
    : lhs(lhs)
    , rhs(rhs)
    {
    }
    virtual
        CwiseBinaryOp<internal::scalar_sum_op<T, T>, const A, const B>
        stack(int n1, int n2) const override
    {
        return lhs->stack(n1, n2) + rhs->stack(n1, n2);
    }

private:
    const StackContainer<A, T, N1, N2, N3>* lhs;
    const StackContainer<B, T, N1, N2, N3>* rhs;
};

template<typename A, typename B, typename T, int N1, int N2, int N3>
class ComponentwiseProduct : public StackContainer<CwiseBinaryOp<internal::scalar_product_op<T, T>, const A, const B>, T, N1, N2, N3>
{
public:
    ComponentwiseProduct(const StackContainer<A, T, N1, N2, N3>* lhs, const StackContainer<B, T, N1, N2, N3>* rhs)
    : lhs(lhs)
    , rhs(rhs)
    {
    }
    virtual
        CwiseBinaryOp<internal::scalar_product_op<T, T>, const A, const B>
        stack(int n1, int n2) const override
    {
        return lhs->stack(n1, n2) * rhs->stack(n1, n2);
    }

private:
    const StackContainer<A, T, N1, N2, N3>* lhs;
    const StackContainer<B, T, N1, N2, N3>* rhs;
};

template<typename A, typename T1, typename T2, int N1, int N2, int N3>
class Dim1MatMul : public StackContainer<CwiseBinaryOp<internal::scalar_product_op<T1, T2>, const CwiseNullaryOp<internal::scalar_constant_op<T1>,const Array<T1, -1, 1>>, const A>, T2, N1, N2, N3>
{
public:
    Dim1MatMul(const DiagonalMatrix<T1, -1>& matrix, const StackContainer<A, T2, N1, N2, N3>& field)
    : matrix(matrix)
    , field(field)
    {}

    virtual
        CwiseBinaryOp<internal::scalar_product_op<T1, T2>, const CwiseNullaryOp<internal::scalar_constant_op<T1>,const Array<T1, -1, 1>>, const A>
        stack(int n1, int n2) const override
    {
        return matrix.diagonal()(n1)*field.stack(n1, n2);
    }

private:
    const StackContainer<A, T2, N1, N2, N3>& field;
    const DiagonalMatrix<T1, -1>& matrix;
};

template<typename A, typename T1, typename T2, int N1, int N2, int N3>
class Dim2MatMul : public StackContainer<CwiseBinaryOp<internal::scalar_product_op<T1, T2>, const CwiseNullaryOp<internal::scalar_constant_op<T1>,const Array<T1, -1, 1>>, const A>, T2, N1, N2, N3>
{
public:
    Dim2MatMul(const DiagonalMatrix<T1, -1>& matrix, const StackContainer<A, T2, N1, N2, N3>& field)
    : matrix(matrix)
    , field(field)
    {}

    virtual
        CwiseBinaryOp<internal::scalar_product_op<T1, T2>, const CwiseNullaryOp<internal::scalar_constant_op<T1>,const Array<T1, -1, 1>>, const A>
        stack(int n1, int n2) const override
    {
        return matrix.diagonal()(n2)*field.stack(n1, n2);
    }

private:
    const StackContainer<A, T2, N1, N2, N3>& field;
    const DiagonalMatrix<T1, -1>& matrix;
};

template<typename A, typename T1, typename T2, int N1, int N2, int N3>
class Dim3MatMul : public StackContainer<ArrayWrapper<const Product<Matrix<T1,-1,-1>, MatrixWrapper<A>>>, T2, N1, N2, N3>
{
public:
    Dim3MatMul(const Matrix<T1, -1, -1>& matrix, const StackContainer<A, T2, N1, N2, N3>& field)
    : matrix(matrix)
    , field(field)
    {}

    virtual
        ArrayWrapper<const Product<Matrix<T1,-1,-1>, MatrixWrapper<A>>>
        stack(int n1, int n2) const override
    {
        return (matrix*field.stack(n1, n2).matrix()).array();
    }

private:
    const StackContainer<A, T2, N1, N2, N3>& field;
    const Matrix<T1, -1, -1>& matrix;
};

template<typename A, typename T1, typename T2, int N1, int N2, int N3>
class MatMul : public StackContainer<ArrayWrapper<const Product<Matrix<T1,-1,-1>, MatrixWrapper<A>>>, T2, N1, N2, N3>
{
public:
    MatMul(const std::vector<Matrix<T1, -1, -1>>& matrices, const StackContainer<A, T2, N1, N2, N3>& field)
    : matrices(matrices)
    , field(field)
    {}

    virtual
        ArrayWrapper<const Product<Matrix<T1,-1,-1>, MatrixWrapper<A>>>
        stack(int n1, int n2) const override
    {
        return (matrices[n1*N2+n2]*field.stack(n1, n2).matrix()).array();
    }

private:
    const StackContainer<A, T2, N1, N2, N3>& field;
    const std::vector<Matrix<T1, -1, -1>>& matrices;
};

template<typename T, int N1, int N2, int N3>
class Field : public StackContainer<Map<const Array<T, -1, 1>, Aligned16>,T,N1,N2,N3>
{
public:
    Field()
    : _data(N1*N2*N3, 0)
    {
    }

    Field(const Field<T, N1, N2, N3>& other)
    : _data(other._data)
    {
    }

    void Reset()
    {
        Zero();
    }

    template<typename A>
    const Field<T, N1, N2, N3>& operator=(const StackContainer<A,T,N1,N2,N3>& other)
    {
        ParallelPerStack([&other,this](int j1, int j2){
            stack(j1, j2) = other.stack(j1,j2);
        });

        return *this;
    }

    // seem to explicitly require this
    const Field<T, N1, N2, N3>& operator=(const Field<T, N1, N2, N3>& other)
    {
        ParallelPerStack([&other,this](int j1, int j2){
            stack(j1, j2) = other.stack(j1,j2);
        });

        return *this;
    }

    bool operator==(const Field<T, N1, N2, N3>& other) const
    {

        // because of the way isApprox works, need to fail on both slice and stack to count
        bool failedSlice = false;
        for (int j=0; j<N3; j++)
        {
            if (!(slice(j)+ 0.001).isApprox(other.slice(j) + 0.001, 0.05))
            {
                failedSlice = true;
            }
        }
        if(failedSlice)
        {
            for (int j1=0; j1<N1; j1++)
            {
                for (int j2=0; j2<N2; j2++)
                {
                    if (!(stack(j1, j2)+0.001).isApprox(other.stack(j1, j2) + 0.001, 0.05))
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    bool operator!=(const Field<T, N1, N2, N3>& other) const
    {
        return !operator==(other);
    }

    const Field<T, N1, N2, N3>& operator*=(T mult)
    {
        ParallelPerStack([mult,this](int j1, int j2){
            stack(j1, j2) *= mult;
        });

        return *this;
    }

    template<typename A>
    const Field<T, N1, N2, N3>& operator+=(const StackContainer<A, T, N1, N2, N3>& other)
    {
        ParallelPerStack([&other,this](int j1, int j2){
            stack(j1, j2) += other.stack(j1, j2);
        });

        return *this;
    }
    template<typename A>
    const Field<T, N1, N2, N3>& operator-=(const StackContainer<A, T, N1, N2, N3>& other)
    {
        ParallelPerStack([&other,this](int j1, int j2){
            stack(j1, j2) -= other.stack(j1, j2);
        });

        return *this;
    }

    using Slice = Map<Array<T, -1, -1>, Unaligned, Stride<N3*N1, N3>>;
    using Stack = Map<Array<T, -1, 1>, Aligned16>;
    using ConstSlice = Map<const Array<T, -1, -1>, Unaligned, Stride<N3*N1, N3>>;
    using ConstStack = Map<const Array<T, -1, 1>, Aligned16>;

    Slice slice(int n3)
    {
        assert(n3>=0 && n3<N3);
        return Slice(&Raw()[n3], N1, N2);
    }
    ConstSlice slice(int n3) const
    {
        assert(n3>=0 && n3<N3);
        return ConstSlice(&Raw()[n3], N1, N2);
    }

    Stack stack(int n1, int n2)
    {
        assert(n1>=0 && n1<N1);
        assert(n2>=0 && n2<N2);
        return Stack(&Raw()[(N1*n2 + n1)*N3], N3);
    }
    virtual ConstStack stack(int n1, int n2) const override
    {
        assert(n1>=0 && n1<N1);
        assert(n2>=0 && n2<N2);
        return ConstStack(&Raw()[(N1*n2 + n1)*N3], N3);
    }

    T& operator()(int n1, int n2, int n3)
    {
        return Raw()[(N1*n2 + n1)*N3 + n3];
    }
    T operator()(int n1, int n2, int n3) const
    {
        return Raw()[(N1*n2 + n1)*N3 + n3];
    }

    T* Raw()
    {
        return _data.data();
    }
    const T* Raw() const
    {
        return _data.data();
    }

    void Zero()
    {
        for(T& datum : _data)
        {
            datum = 0;
        }
    }


    template<typename Solver>
    void Solve(std::vector<Solver, aligned_allocator<Solver>>& solvers, Field<T, N1, N2, N3>& result) const
    {
        assert(solvers.size() == N1*N2);
        ParallelPerStack(
            [&solvers,&result,this](int j1, int j2)
            {
                Dim3Solve(solvers[j1*N2+j2], j1, j2, result);
            }
        );
    }

    template<typename Solver>
    void Solve(Solver& solver, Field<T, N1, N2, N3>& result) const
    {
        ParallelPerStack(
            [&solver,&result,this](int j1, int j2)
            {
                Dim3Solve(solver, j1, j2, result);
            }
        );
    }

    virtual void ParallelPerStack(std::function<void(int j1, int j2)> f) const
    {
        #pragma omp parallel for collapse(2)
        for (int j2=0; j2<N2; j2++)
        {
            for (int j1=0; j1<N1; j1++)
            {
                f(j1, j2);
            }
        }
    }

    void Save(std::ofstream& filestream)
    {
        filestream.write(reinterpret_cast<char*>(Raw()), sizeof(T)*N1*N2*N3);
    }

    void Load(std::ifstream& filestream)
    {
        filestream.read(reinterpret_cast<char*>(Raw()), sizeof(T)*N1*N2*N3);
    }

private:

    template<typename Solver>
    void Dim3Solve(Solver& solver, int j1, int j2, Field<stratifloat, N1, N2, N3>& result) const
    {
        assert(solver.rows() == N3);

        Matrix<stratifloat, N3, 1> col = stack(j1, j2);
        Matrix<stratifloat, N3, 1> res = solver.solve(col);
        result.stack(j1, j2) = res;
    }

    template<typename Solver>
    void Dim3Solve(Solver& solver, int j1, int j2, Field<complex, N1, N2, N3>& result) const
    {
        assert(solver.rows() == N3);

        Matrix<stratifloat, N3, 1> col = stack(j1, j2).real();
        Matrix<stratifloat, N3, 1> res = solver.solve(col);
        result.stack(j1, j2).real() = res;

        col = stack(j1, j2).imag();
        res = solver.solve(col);
        result.stack(j1, j2).imag() = res;
    }


    // stored in column-major ordering of size (N1, N2, N3)
    std::vector<T, aligned_allocator<T>> _data;
};

template<int N1, int N2, int N3>
class ModalField;

template<int N1, int N2, int N3>
class NodalField : public Field<stratifloat, N1, N2, N3>
{
public:
    template<typename A>
    const NodalField<N1, N2, N3>& operator=(const StackContainer<A,stratifloat,N1,N2,N3>& other)
    {
        Field<stratifloat, N1, N2, N3>::operator=(other);
        return *this;
    }

    NodalField()
    : Field<stratifloat, N1, N2, N3>()
    {
        int dims[] = {N2, N1, N3};
        int odims[] = {N2, N1, N3/2+1};

        std::vector<stratifloat, aligned_allocator<stratifloat>> inputData(N1*N2*N3);
        std::vector<complex, aligned_allocator<complex>> outputData(N1*N2*(N3/2+1));

        auto plan = f3_plan_many_dft_r2c(3,
                                        dims,
                                        1,
                                        inputData.data(),
                                        dims,
                                        1,
                                        1,
                                        reinterpret_cast<f3_complex*>(outputData.data()),
                                        odims,
                                        1,
                                        1,
                                        FFTW_PATIENT);

    }

    void ToModal(ModalField<N1,N2,N3>& other, bool filter = true) const
    {

        // do FFT in 1st and 2nd dimensions

        int dims[] = {N2, N1, N3};
        int odims[] = {N2, N1, N3/2+1};
        auto plan = f3_plan_many_dft_r2c(3,
                                        dims,
                                        1,
                                        const_cast<stratifloat*>(this->Raw()),
                                        dims,
                                        1,
                                        1,
                                        reinterpret_cast<f3_complex*>(other.Raw()),
                                        odims,
                                        1,
                                        1,
                                        FFTW_PATIENT);
        f3_execute(plan);
        f3_destroy_plan(plan);

        if (filter)
        {
            other *= 1/static_cast<stratifloat>(N1*N2*N3);
            other.Filter();
        }
        else
        {
            for (int j=0; j<N1*N2*(N3/2+1); j++)
            {
                other.Raw()[j] *= 1/static_cast<stratifloat>(N1*N2*N3);
            }
        }
    }

    stratifloat Max() const
    {
        stratifloat max = 0;

        for (int j3=0; j3<N3; j3++)
        {
            stratifloat norm = this->slice(j3).matrix().template lpNorm<Infinity>();
            if (norm > max)
            {
                max = norm;
            }
        }

        return max;
    }

    void SetValue(std::function<stratifloat(stratifloat,stratifloat,stratifloat)> f, stratifloat L1, stratifloat L2, stratifloat L3)
    {
        ArrayX x = FourierPoints(L1, N1);
        ArrayX y = FourierPoints(L2, N2);
        ArrayX z = FourierPoints(L3, N3);

        for (int j1=0; j1<N1; j1++)
        {
            for (int j2=0; j2<N2; j2++)
            {
                for (int j3=0; j3<N3; j3++)
                {
                    (*this)(j1,j2,j3) = f(x(j1), y(j2), z(j3));
                }
            }
        }

    }

    NodalField& operator-=(stratifloat other)
    {
        this->ParallelPerStack([other,this](int j1, int j2){
            this->stack(j1, j2) -= other;
        });

        return *this;
    }

    using Field<stratifloat, N1, N2, N3>::operator-=;
};

template<int N1, int N2, int N3>
class ModalField : public Field<complex, N1, N2, N3/2+1>
{
    static constexpr int actualN3 = N3/2 + 1;
public:
    template<typename A>
    const ModalField<N1, N2, N3>& operator=(const StackContainer<A,complex, N1, N2, N3/2+1>& other)
    {
        Field<complex, N1, N2, N3/2+1>::operator=(other);
        return *this;
    }

    ModalField()
    : Field<complex, N1, N2, N3/2+1>()
    {
        inputData.resize(N1*N2*actualN3);

        std::vector<stratifloat, aligned_allocator<stratifloat>> outputData(N1*N2*N3);

        int dims[] = {N2, N1, N3};
        int idims[] = {N2, N1, actualN3};

        auto plan = f3_plan_many_dft_c2r(3,
                                dims,
                                1,
                                reinterpret_cast<f3_complex*>(inputData.data()),
                                idims,
                                1,
                                1,
                                outputData.data(),
                                dims,
                                1,
                                1,
                                FFTW_PATIENT);

    }


    void ToNodal(NodalField<N1, N2, N3>& other) const
    {
        // do IFT in 1st and 2nd dimensions

        // make a copy of the input data as it is modified by the transform
        #pragma omp parallel for
        for (unsigned int j=0; j<actualN3*N2*N1; j++)
        {
            inputData[j] = this->Raw()[j];
        }

        int dims[] = {N2, N1, N3};
        int idims[] = {N2, N1, actualN3};
        auto plan = f3_plan_many_dft_c2r(3,
                                        dims,
                                        1,
                                        reinterpret_cast<f3_complex*>(inputData.data()),
                                        idims,
                                        1,
                                        1,
                                        other.Raw(),
                                        dims,
                                        1,
                                        1,
                                        FFTW_PATIENT);
        f3_execute(plan);
        f3_destroy_plan(plan);
    }

    void Filter()
    {
        if (N1>2)
        {
            #pragma omp parallel for collapse(2)
            for (int j2=0; j2<N2; j2++)
            {
                for (int j1=N1/3; j1<=N1-(N1/3); j1++)
                {
                    this->stack(j1, j2).setZero();
                }
            }
        }

        if (N2>2)
        {
            #pragma omp parallel for collapse(2)
            for (int j2=(N2/3); j2<=N2-(N2/3); j2++)
            {
                for (int j1=0; j1<N1; j1++)
                {
                    this->stack(j1, j2).setZero();
                }
            }
        }

        if (N3>2)
        {
            #pragma omp parallel for
            for (int j3=(N3/3); j3<actualN3; j3++)
            {
                this->slice(j3).setZero();
            }
        }
    }

    void RandomizeCoefficients(stratifloat cutoff)
    {
        this->Zero();

        assert(cutoff < 2.0/3.0);

        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_real_distribution<stratifloat> rng(-1.0,1.0);

        int j3min = 1;
        int j3max = N3-1;

        for (int j1=0; j1<0.5*cutoff*N1; j1++)
        {
            for (int j3=j3min; j3<actualN3; j3++)
            {
                for (int j2=0; j2<0.5*cutoff*N2; j2++)
                {
                    this->operator()(j1,j2,j3) = rng(generator) + i*rng(generator);
                }
                for (int j2=N2-1; j2>(1-0.5*cutoff)*N2; j2--)
                {
                    this->operator()(j1,j2,j3) = rng(generator) + i*rng(generator);
                }
            }
        }
    }

    void ExciteLowWavenumbers()
    {
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_real_distribution<stratifloat> rng(-1.0,1.0);

        for (int j1=-N2/6; j1<N1/6; j1++)
        {
            for (int j2=-N2/6; j2<=N2/6; j2++)
            {
                for (int j3=0; j3<=N3/6; j3++)
                {
                    int actualj1 = j1;
                    if (actualj1<0) actualj1 += N1;

                    int actualj2 = j2;
                    if (actualj2<0) actualj2 += N2;

                    this->operator()(actualj1,actualj2,j3) = pow(j1*j1+j2*j2+j3*j3+1,-5.0/3.0)*(rng(generator) + i*rng(generator));
                }
            }
        }
    }

    void MakeMode2()
    {
        for (int j2=0; j2<N2; j2++)
        {
            for (int j1=1; j1<N1; j1+=2)
            {
                this->stack(j1,j2).setZero();
            }
        }
    }

    virtual void ParallelPerStack(std::function<void(int j1, int j2)> f) const override
    {
        int maxN1 = N1/3;
        int maxN2 = N2/3;
        int minN2 = N2-(N2/3)+1;
        int minN1 = N1-(N1/3)+1;

        if(N2>1)
        {
            // #pragma omp parallel for collapse(2)
            for (int j2=0; j2<maxN2; j2++)
            {
                for (int j1=0; j1<maxN1; j1++)
                {
                    f(j1, j2);
                }
                for (int j1=minN1; j1<N1; j1++)
                {
                    f(j1, j2);
                }
            }

            // #pragma omp parallel for collapse(2)
            for (int j2=minN2; j2<N2; j2++)
            {
                for (int j1=0; j1<maxN1; j1++)
                {
                    f(j1, j2);
                }
                for (int j1=minN1; j1<N1; j1++)
                {
                    f(j1, j2);
                }
            }
        }
        else
        {
            #pragma omp parallel for
            for (int j1=0; j1<maxN1; j1++)
            {
                f(j1, 0);
            }

            #pragma omp parallel for
            for (int j1=minN1; j1<N1; j1++)
            {
                f(j1, 0);
            }
        }
    }

    void PhaseShift(stratifloat shift)
    {
        ParallelPerStack([this, shift](int j1, int j2)
        {
            this->stack(j1,j2) *= exp(i*(j1*shift));
        });
    }


private:
    static std::vector<complex, aligned_allocator<complex>> inputData;
};

template<int N1, int N2, int N3>
constexpr int ModalField<N1,N2,N3>::actualN3;

template<int N1, int N2, int N3>
std::vector<complex, aligned_allocator<complex>> ModalField<N1,N2,N3>::inputData;

template<typename A, typename T, int N1, int N2, int N3>
ScalarProduct<A, T, N1, N2, N3> operator*(T scalar,
                                       const StackContainer<A, T, N1, N2, N3>& field)
{
    return ScalarProduct<A, T, N1, N2, N3>(scalar, &field);
}

template<typename A, int N1, int N2, int N3>
ScalarProduct<A, complex, N1, N2, N3> operator*(stratifloat scalar,
                                       const StackContainer<A, complex, N1, N2, N3>& field)
{
    return ScalarProduct<A, complex, N1, N2, N3>(scalar, &field);
}

template<typename A, typename T, int N1, int N2, int N3>
ScalarProduct<A, T, N1, N2, N3> operator*(notstratifloat scalar,
                                       const StackContainer<A, T, N1, N2, N3>& field)
{
    return ScalarProduct<A, T, N1, N2, N3>((stratifloat)scalar, &field);
}


template<typename A, typename B, typename T, int N1, int N2, int N3>
ComponentwiseSum<A, B, T, N1, N2, N3> operator+(const StackContainer<A, T, N1, N2, N3>& lhs, const StackContainer<B, T, N1, N2, N3>& rhs)
{
    return ComponentwiseSum<A, B, T, N1, N2, N3>(&lhs, &rhs);
}

template<typename A, typename B, typename T, int N1, int N2, int N3>
ComponentwiseProduct<A, B, T, N1, N2, N3> operator*(const StackContainer<A, T, N1, N2, N3>& lhs, const StackContainer<B, T, N1, N2, N3>& rhs)
{
    return ComponentwiseProduct<A, B, T, N1, N2, N3>(&lhs, &rhs);
}

template<typename A, typename T, int N1, int N2, int N3>
Negate<A, T, N1, N2, N3> operator-(const StackContainer<A, T, N1, N2, N3>& field)
{
    return Negate<A, T, N1, N2, N3>(&field);
}
