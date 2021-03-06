#include <alps/transform/fourier.hpp>

#include <algorithm>
#include <cmath>
#include <cassert>

namespace fftw = alps::fftw;

namespace alps { namespace transform {

dft::dft(unsigned n, int direction, bool use_fftw)
    : fftw_()
    , n_(n)
    , direction_(direction)
{
    if (use_fftw)
        fftw_ = fftw::wrapper<>(fftw::plan_data(n, direction, FFTW_ESTIMATE));
}

void dft::operator() (const std::complex<double> *in, std::complex<double> *out)
{
    if (use_fftw()) {
        // copy to internal buffers
        std::copy(in, in + n_, fftw().in());
        fftw().execute();
        for (unsigned i = 0; i != n_; ++i)
            out[i] += fftw().out()[i];
    } else {
        naive(in, out);
    }
}

void dft::naive(const std::complex<double> *in, std::complex<double> *out) const
{
    // naive implementation of the formula
    //         f_hat[k] = sum_j exp(+2pi i/N k j) f_conv[j]
    double p2piiIn = direction_*2*M_PI/n_;
    for (unsigned k = 0; k != n_; ++k) {
        std::complex<double> f_hatk = 0;
        for (unsigned j = 0; j != n_; ++j) {
            f_hatk += std::exp(std::complex<double>(0, p2piiIn * k * j))
                        * in[j];
        }
        *(out++) += f_hatk;
    }
}

iw_to_tau_real::iw_to_tau_real(unsigned niw, unsigned ntau,
                               double beta, statistics stat, bool use_fftw)
    : niw_(niw)
    , ntau_(ntau)
    , beta_(beta)
    , stat_(stat)
    , fft_()
{
    // To be on the safe side, make sure that the tau axis is always larger
    // than the frequency axis (we can always "pad" the frequencies with
    // zeros).
    oversampling_ = std::ceil(1 * niw/ntau);

    if (use_fftw) {
        fft_ = fftw::wrapper<>(fftw::plan_data(ntau_ * oversampling_,
                                               -1, FFTW_ESTIMATE));
    }
}

void iw_to_tau_real::operator() (const std::complex<double> *in, double *out)
{
    if (!fft_.is_initialized()) {
        naive(in, out);
        return;
    }

    std::copy(in, in + niw_, fft_.in());
    fft_.execute();

    for (unsigned n = 0; n != ntau_; ++n) {
        std::complex<double> ftau = fft_.out()[n * oversampling_];
        ftau *= 2/beta_;
        if (stat_ == fermionic)
            ftau *= std::exp(std::complex<double>(0, -M_PI * n / ntau_));

        // this quantity should now be real!
        assert(std::abs(ftau.imag()) < 1e-10 * std::abs(ftau.real()));
        out[n] += ftau.real();
    }
}

void iw_to_tau_real::naive(const std::complex<double> *in, double *out) const
{
    for (unsigned i = 0; i != ntau_; ++i) {
        for (unsigned k = 0; k != niw_; ++k) {
            double wt = M_PI * (2*k + (unsigned)stat_) * i/ntau_;
            out[i] += 2/beta_ * (cos(wt)*in[k].real() + sin(wt)*in[k].imag());
        }
    }
}


iw_to_tau_model_real::iw_to_tau_model_real(
                            unsigned niw, unsigned ntau, double beta,
                            statistics stat, const std::vector<double> &moments,
                            bool use_fftw)
    : transform_(niw, ntau, beta, stat, use_fftw)
    , model_(moments, beta, stat)
    , in_buffer_(transform_.in_size())
{ }

void iw_to_tau_model_real::operator() (const std::complex<double> *in, double *out)
{
    std::copy(in, in + transform_.in_size(), in_buffer_.begin());

    // remove model in frequency space
    for (unsigned n = 0; n != transform_.in_size(); ++n)
        in_buffer_[n] -= model_.fiw(n);

    // do transform
    transform_(&in_buffer_[0], out);

    // add model in tau space
    for (unsigned n = 0; n != transform_.out_size(); ++n)
        out[n] += model_.ftau(transform_.tau_value(n));
}


tau_to_iw_real::tau_to_iw_real(unsigned ntau, unsigned niw, double beta,
                               statistics stat, bool use_fftw)
    : niw_(niw)
    , ntau_(ntau)
    , beta_(beta)
    , stat_(stat)
    , fft_()
{
    // To be on the safe side, make sure that the tau axis is always larger
    // than the frequency axis (we can always "pad" the frequencies with
    // zeros).
    oversampling_ = std::ceil(1 * niw/ntau);

    if (use_fftw) {
        fft_ = fftw::wrapper<>(fftw::plan_data(ntau_ * oversampling_,
                                               +1, FFTW_ESTIMATE));
    }
}

void tau_to_iw_real::operator() (const double *in, std::complex<double> *out)
{
    if (!fft_.is_initialized()) {
        naive(in, out);
        return;
    }

    const double norm = beta_/ntau_;
    for (unsigned n = 0; n != ntau_; ++n) {
        std::complex<double> ftau = in[n];
        if (stat_ == fermionic)
            ftau *= std::exp(std::complex<double>(0, M_PI * n / ntau_));

        fft_.in()[n * oversampling_] = ftau * norm;
    }

    fft_.execute();
    for (unsigned i = 0; i != niw_; ++i)
        out[i] += fft_.out()[i];
}

void tau_to_iw_real::naive(const double *in, std::complex<double> *out) const
{
    for (unsigned k = 0; k != niw_; ++k) {
        for (unsigned i = 0; i != ntau_; ++i) {
            double wt = M_PI * (2*k + (unsigned)stat_) * i/ntau_;
            out[k] += beta_/ntau_
                      * std::complex<double>(cos(wt) * in[i], sin(wt) * in[i]);
        }
    }
}

}}
