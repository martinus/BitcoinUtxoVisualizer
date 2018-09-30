#pragma once

namespace bv {

class LinearFunction
{
public:
    LinearFunction(double x1, double y1, double x2, double y2)
        : m_k((y2 - y1) / (x2 - x1)),
          m_d(y1 - m_k * x1)
    {
    }

    double k() const
    {
        return m_k;
    }

    double d() const
    {
        return m_d;
    }

    double operator()(double x) const
    {
        return m_k * x + m_d;
    }

private:
    double const m_k;
    double const m_d;
};


} // namespace bv