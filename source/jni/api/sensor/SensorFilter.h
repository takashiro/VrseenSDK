#pragma once

#include "VBasicmath.h"
#include "VDeque.h"
#include "Alg.h"

NV_NAMESPACE_BEGIN

// A base class for filters that maintains a buffer of sensor data taken over time and implements
// various simple filters, most of which are linear functions of the data history.
// Maintains the running sum of its elements for better performance on large capacity values
template <typename T>
class SensorFilterBase : public VCircularBuffer<T>
{
protected:
    T RunningTotal;               // Cached sum of the elements

public:
    SensorFilterBase(int capacity = 500 )
        : VCircularBuffer<T>(capacity), RunningTotal()
    {
        this->clear();
    };

    // The following methods are augmented to update the cached running sum value
    void append(const T &e)
    {
        VCircularBuffer<T>::append(e);
        RunningTotal += e;
        if (this->v_end == 0)
        {
            // update the cached total to avoid error accumulation
            RunningTotal = T();
            for (int i = 0; i < this->v_count; i++)
                RunningTotal += this->v_data[i];
        }
    }

    void prepend(const T &e)
    {
        VCircularBuffer<T>::preAppend(e);
        RunningTotal += e;
        if (this->v_start == 0)
        {
            // update the cached total to avoid error accumulation
            RunningTotal = T();
            for (int i = 0; i < this->v_count; i++)
                RunningTotal += this->v_data[i];
        }
    }

    T takeLast()
    {
        T e = VCircularBuffer<T>::takeLast();
        RunningTotal -= e;
        return e;
    }

    T takeFirst()
    {
        T e = VCircularBuffer<T>::takeFirst();
        RunningTotal -= e;
        return e;
    }

    void clear()
    {
        VCircularBuffer<T>::clear();
        RunningTotal = T();
    }

    // Simple statistics
    T Total() const
    {
        return RunningTotal;
    }

    T Mean() const
    {
        return this->isEmpty() ? T() : (Total() / (float) this->v_count);
    }

	T MeanN(int n) const
	{
        OVR_ASSERT(n > 0);
        OVR_ASSERT(this->v_capacity >= n);
		T total = T();
        for (int i = 0; i < n; i++)
        {
            total += this->peekLast(i);
		}
		return total / n;
	}

    // A popular family of smoothing filters and smoothed derivatives

    T SavitzkyGolaySmooth4()
    {
        OVR_ASSERT(this->v_capacity >= 4);
        return this->peekLast(0)*0.7f +
               this->peekLast(1)*0.4f +
               this->peekLast(2)*0.1f -
               this->peekLast(3)*0.2f;
    }

    T SavitzkyGolaySmooth8() const
    {
        OVR_ASSERT(this->v_capacity >= 8);
        return this->peekLast(0)*0.41667f +
               this->peekLast(1)*0.33333f +
               this->peekLast(2)*0.25f +
               this->peekLast(3)*0.16667f +
               this->peekLast(4)*0.08333f -
               this->peekLast(6)*0.08333f -
               this->peekLast(7)*0.16667f;
    }

    T SavitzkyGolayDerivative4() const
    {
        OVR_ASSERT(this->v_capacity >= 4);
        return this->peekLast(0)*0.3f +
               this->peekLast(1)*0.1f -
               this->peekLast(2)*0.1f -
               this->peekLast(3)*0.3f;
    }

    T SavitzkyGolayDerivative5() const
    {
            OVR_ASSERT(this->v_capacity >= 5);
            return this->peekLast(0)*0.2f +
                   this->peekLast(1)*0.1f -
                   this->peekLast(3)*0.1f -
                   this->peekLast(4)*0.2f;
   }

    T SavitzkyGolayDerivative12() const
    {
        OVR_ASSERT(this->v_capacity >= 12);
        return this->peekLast(0)*0.03846f +
               this->peekLast(1)*0.03147f +
               this->peekLast(2)*0.02448f +
               this->peekLast(3)*0.01748f +
               this->peekLast(4)*0.01049f +
               this->peekLast(5)*0.0035f -
               this->peekLast(6)*0.0035f -
               this->peekLast(7)*0.01049f -
               this->peekLast(8)*0.01748f -
               this->peekLast(9)*0.02448f -
               this->peekLast(10)*0.03147f -
               this->peekLast(11)*0.03846f;
    }

    T SavitzkyGolayDerivativeN(int n) const
    {
        OVR_ASSERT(this->v_capacity >= n);
        int m = (n-1)/2;
        T result = T();
        for (int k = 1; k <= m; k++)
        {
            int ind1 = m - k;
            int ind2 = n - m + k - 1;
            result += (this->peekLast(ind1) - this->peekLast(ind2)) * (float) k;
        }
        float coef = 3.0f/(m*(m+1.0f)*(2.0f*m+1.0f));
        result = result*coef;
        return result;
    }

    T Median() const
    {
        T* copy = (T*) OVR_ALLOC(this->v_count * sizeof(T));
        T result = Alg::Median(ArrayAdaptor(copy));
        OVR_FREE(copy);
        return result;
    }
};

// This class maintains a buffer of sensor data taken over time and implements
// various simple filters, most of which are linear functions of the data history.
template <typename T>
class SensorFilter : public SensorFilterBase<V3Vect<T> >
{
public:
    SensorFilter(int capacity = SensorFilterBase<V3Vect<T> >::DefaultCapacity) : SensorFilterBase<V3Vect<T> >(capacity) { };

    // Simple statistics
    V3Vect<T> Median() const;
    V3Vect<T> Variance() const; // The diagonal of covariance matrix
    VR3Matrix<T> Covariance() const;
    V3Vect<T> PearsonCoefficient() const;
};

typedef SensorFilter<float> SensorFilterf;
typedef SensorFilter<double> SensorFilterd;

// This filter operates on the values that are measured in the body frame and rotate with the device
class SensorFilterBodyFrame : public SensorFilterBase<V3Vectf>
{
private:
    // low pass filter gain
    float gain;
    // sum of squared norms of the values
    float runningTotalLengthSq;
    // cumulative rotation quaternion
    VQuatf Q;
    // current low pass filter output
    V3Vectf output;

    // make private so it isn't used by accident
    // in addition to the normal SensorFilterBase::PushBack, keeps track of running sum of LengthSq
    // for the purpose of variance computations
    void append(const V3Vectf &e)
    {
        runningTotalLengthSq += this->isFull() ? (e.LengthSq() - this->peekFirst().LengthSq()) : e.LengthSq();
        SensorFilterBase<V3Vectf>::append(e);
        if (this->v_end == 0)
        {
            // update the cached total to avoid error accumulation
            runningTotalLengthSq = 0;
            for (int i = 0; i < this->v_count; i++)
                runningTotalLengthSq += this->v_data[i].LengthSq();
        }
    }

public:
    SensorFilterBodyFrame(int capacity = 500)
        : SensorFilterBase<V3Vectf>(capacity), gain(2.5),
          runningTotalLengthSq(0), Q(), output()  { };

    // return the scalar variance of the filter values (rotated to be in the same frame)
    float Variance() const
    {
        return this->isEmpty() ? 0 : (runningTotalLengthSq / this->v_count - this->Mean().LengthSq());
    }

    // return the scalar standard deviation of the filter values (rotated to be in the same frame)
    float StdDev() const
    {
        return sqrt(Variance());
    }

    // confidence value based on the stddev of the data (between 0.0 and 1.0, more is better)
    float Confidence() const
    {
        return Alg::Clamp(0.48f - 0.1f * logf(StdDev()), 0.0f, 1.0f) * this->v_capacity / this->v_capacity;
    }

    // add a new element to the filter
    // takes rotation increment since the last update
    // in order to rotate the previous value to the current body frame
    void Update(V3Vectf value, float deltaT, VQuatf deltaQ = VQuatf())
    {
        if (this->isEmpty())
        {
            output = value;
        }
        else
        {
            // rotate by deltaQ
            output = deltaQ.Inverted().Rotate(output);
            // apply low-pass filter
            output += (value - output) * gain * deltaT;
        }

        // put the value into the fixed frame for the stddev computation
        Q = Q * deltaQ;
        append(Q.Rotate(output));
    }

    // returns the filter average in the current body frame
    V3Vectf GetFilteredValue() const
    {
        return Q.Inverted().Rotate(this->Mean());
    }
};

NV_NAMESPACE_END


