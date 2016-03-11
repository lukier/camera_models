/**
 * ****************************************************************************
 * Copyright (c) 2015, Robert Lukierski.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * Neither the name of [project] nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ****************************************************************************
 * Pinhole, inverse in image planes (not distance).
 * Modified Brown-Conrady distortions (cf Intel librealsense).
 * @warning Not invertible, inverse != forward.
 * @see https://github.com/IntelRealSense/librealsense/blob/master/doc/projection.md
 * ****************************************************************************
 */

#ifndef PINHOLE_DISPARITY_BROWN_CONRADY_CAMERA_MODEL_HPP
#define PINHOLE_DISPARITY_BROWN_CONRADY_CAMERA_MODEL_HPP

#include <CameraModelHelpers.hpp>
#include <PinholeDisparityCameraModel.hpp>

// fwd
namespace camera 
{
template<typename _Scalar, int _Options = 0> class PinholeDisparityBrownConradyCameraModel;

namespace internal
{
static constexpr unsigned int PinholeDisparityBrownConradyCameraModelParameterCount = 12;
}
}

// Eigen Traits 
namespace Eigen 
{
    namespace internal 
    {
        template<typename _Scalar, int _Options>
        struct traits<camera::PinholeDisparityBrownConradyCameraModel<_Scalar,_Options> > 
        {
            typedef _Scalar Scalar;
            typedef Matrix<Scalar,camera::internal::PinholeDisparityBrownConradyCameraModelParameterCount,1> ComplexType;
        };
        
        template<typename _Scalar, int _Options>
        struct traits<Map<camera::PinholeDisparityBrownConradyCameraModel<_Scalar>, _Options> > : traits<camera::PinholeDisparityBrownConradyCameraModel<_Scalar, _Options> > 
        {
            typedef _Scalar Scalar;
            typedef Map<Matrix<Scalar,camera::internal::PinholeDisparityBrownConradyCameraModelParameterCount,1>,_Options> ComplexType;
        };
        
        template<typename _Scalar, int _Options>
        struct traits<Map<const camera::PinholeDisparityBrownConradyCameraModel<_Scalar>, _Options> > : traits<const camera::PinholeDisparityBrownConradyCameraModel<_Scalar, _Options> > 
        {
            typedef _Scalar Scalar;
            typedef Map<const Matrix<Scalar,camera::internal::PinholeDisparityBrownConradyCameraModelParameterCount,1>,_Options> ComplexType;
        };    
    }
}

namespace camera
{
/**
 * PinholeDisparityBrownConradyCameraModelBase, Model Specific Functions.
 */
template<typename Derived>
class PinholeDisparityBrownConradyCameraModelBase : public CameraFunctions<Derived>, public ComplexTypes<typename Eigen::internal::traits<Derived>::Scalar>
{
    typedef CameraFunctions<Derived> FunctionsBase;
public:
    // various helpers and types
    typedef typename Eigen::internal::traits<Derived>::Scalar Scalar;
    typedef typename Eigen::internal::traits<Derived>::ComplexType& ComplexReference;
    typedef const typename Eigen::internal::traits<Derived>::ComplexType& ConstComplexReference;
    static constexpr CameraModelType ModelType = CameraModelType::PinholeDisparityBrownConrady;
    
    using FunctionsBase::forward;
    using FunctionsBase::inverse;
    using FunctionsBase::inverseAtDistance;
    using FunctionsBase::twoFrameProject;
    using FunctionsBase::worldToCamera;
    using FunctionsBase::cameraToWorld;
    using FunctionsBase::pixelValid;
    using FunctionsBase::pixelValidSquare;
    using FunctionsBase::pixelValidCircular;
    using FunctionsBase::resizeViewport;
    
    template<typename NewScalarType>
    EIGEN_DEVICE_FUNC inline PinholeDisparityBrownConradyCameraModel<NewScalarType> cast() const { return PinholeDisparityBrownConradyCameraModel<NewScalarType>(access().template cast<NewScalarType>()); }
    
    template<typename OtherDerived> 
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE PinholeDisparityBrownConradyCameraModelBase<Derived>& operator=(const PinholeDisparityBrownConradyCameraModelBase<OtherDerived> & other) { access_nonconst() = other.access(); return *this; }
    
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE ConstComplexReference access() const  { return static_cast<const Derived*>(this)->access(); }
private:
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE ComplexReference access_nonconst()  { return static_cast<Derived*>(this)->access_nonconst(); }
public:
        
    static constexpr unsigned int NumParameters = camera::internal::PinholeDisparityBrownConradyCameraModelParameterCount;
    static constexpr unsigned int ParametersToOptimize = NumParameters - 2;
    static constexpr bool CalibrationSupported = true;
    
    template<typename T = Scalar>
    static EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE typename ComplexTypes<T>::PointT inverse(const Derived& ccd, T pixx, T pixy) 
    {
        typename ComplexTypes<T>::PointT ret;
        
        // to image plane
        const T x = (ccd.fx() * ccd.skew() * ccd.v0() - ccd.fy() * ccd.u0())/(ccd.fx() * ccd.fy()) - (pixy * ccd.skew())/ccd.fy() + pixx / ccd.fx();
        const T y = (pixy - ccd.v0()) / ccd.fy();

        const T r2  = x * x + y * y;
        const T f = T(1.0) + ccd.k1() * r2 + ccd.k2() * r2 * r2 + ccd.k3() * r2 * r2 * r2;
        
        const T new_x = x * f + T(2.0) * ccd.p1() * x * y + ccd.p2() * (r2 + T(2.0) * x * x);
        const T new_y = y * f + T(2.0) * ccd.p2() * x * y + ccd.p1() * (r2 + T(2.0) * y * y);
        
        // to point
        ret(0) = new_x;
        ret(1) = new_y;
        ret(2) = T(1.0);

        return ret;
    }
    
    template<typename T = Scalar>
    static EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE typename ComplexTypes<T>::PixelT forward(const Derived& ccd, const typename ComplexTypes<T>::PointT& tmp_pt) 
    {
        typename ComplexTypes<T>::PixelT ret;
        
        // distorted image plane point
        T x = tmp_pt(0) / tmp_pt(2);
        T y = tmp_pt(1) / tmp_pt(2);
        
        const T r2 = x * x + y * y;
        const T f = T(1.0) + ccd.k1() * r2 + ccd.k2() * r2 * r2 + ccd.k3() * r2 * r2 * r2;
        
        x *= f; 
        y *= f;
        
        // undistorted image plane point
        const T new_x = x + T(2.0) * ccd.p1() * x * y + ccd.p2() * (r2 + T(2.0) * x * x);
        const T new_y = y + T(2.0) * ccd.p2() * x * y + ccd.p1() * (r2 + T(2.0) * y * y);

        // intrinsics
        ret(0) = ccd.fx() * new_x + (ccd.fx() * ccd.skew()) * new_y + ccd.u0();
        ret(1) = ccd.fy() * new_y + ccd.v0();
        
        return ret;
    }
    
    template<typename T = Scalar>
    static EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE bool pixelValidSquare(const Derived& ccd, T x, T y) 
    {
        if((x >= T(0.0)) && (x < ccd.width()) && (y >= T(0.0)) && (y < ccd.height()))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    
    template<typename T = Scalar>
    static EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE bool pixelValidCircular(const Derived& ccd, T x, T y) 
    {
        return true;
    }
    
    template<typename T = Scalar>
    static EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE void resizeViewport(Derived& ccd, const T& new_width, const T& new_height)
    {
        using std::min;
        const T x_ratio = new_width / ccd.width();
        const T y_ratio = new_height / ccd.height();
        
        ccd.fx(ccd.fx() * x_ratio);
        ccd.fy(ccd.fy() * y_ratio);
        ccd.u0(ccd.u0() * x_ratio);
        ccd.v0(ccd.v0() * y_ratio);
        ccd.width(new_width);
        ccd.height(new_height);
    }
    
    // access to camera model parameters
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE const Scalar& fx() const { return data()[0]; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE const Scalar& fy() const { return data()[1]; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE const Scalar& u0() const { return data()[2]; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE const Scalar& v0() const { return data()[3]; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE const Scalar& k1() const { return data()[4]; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE const Scalar& k2() const { return data()[5]; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE const Scalar& k3() const { return data()[6]; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE const Scalar& p1() const { return data()[7]; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE const Scalar& p2() const { return data()[8]; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE const Scalar& skew() const { return data()[9]; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE const Scalar& width() const { return data()[10]; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE const Scalar& height() const { return data()[11]; }
    
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE void fx(const Scalar& v) { data()[0] = v; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE void fy(const Scalar& v) { data()[1] = v; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE void u0(const Scalar& v) { data()[2] = v; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE void v0(const Scalar& v) { data()[3] = v; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE void k1(const Scalar& v) { data()[4] = v; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE void k2(const Scalar& v) { data()[5] = v; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE void k3(const Scalar& v) { data()[6] = v; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE void p1(const Scalar& v) { data()[7] = v; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE void p2(const Scalar& v) { data()[8] = v; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE void skew(const Scalar& v) { data()[9] = v; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE void width(const Scalar& v) { data()[10] = v; }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE void height(const Scalar& v) { data()[11] = v; }
    
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE Scalar* data() { return access_nonconst().data(); }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE const Scalar* data() const { return access().data(); }
    
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE PinholeDisparityCameraModel<Scalar> getIdeal() const
    {
        return PinholeDisparityCameraModel<Scalar>(fx(),fy(),u0(),v0(),width(),height());
    }
    
#ifdef CAMERA_MODELS_HAVE_SERIALIZER
    template <typename Archive>
    void serialize(Archive & archive, std::uint32_t const version)
    { 
        CAMERA_MODELS_SERIALIZE(archive,"fx",data()[0]);
        CAMERA_MODELS_SERIALIZE(archive,"fy",data()[1]);
        CAMERA_MODELS_SERIALIZE(archive,"u0",data()[2]);
        CAMERA_MODELS_SERIALIZE(archive,"v0",data()[3]);
        CAMERA_MODELS_SERIALIZE(archive,"k1",data()[4]);
        CAMERA_MODELS_SERIALIZE(archive,"k2",data()[5]);
        CAMERA_MODELS_SERIALIZE(archive,"k3",data()[6]);
        CAMERA_MODELS_SERIALIZE(archive,"p1",data()[7]);
        CAMERA_MODELS_SERIALIZE(archive,"p2",data()[8]);
        CAMERA_MODELS_SERIALIZE(archive,"skew",data()[9]);
        CAMERA_MODELS_SERIALIZE(archive,"width",data()[10]);
        CAMERA_MODELS_SERIALIZE(archive,"height",data()[11]);
    }
#endif // CAMERA_MODELS_HAVE_SERIALIZER
};

/**
 * PinholeDisparityBrownConradyCameraModel, Eigen storage.
 */
template<typename _Scalar, int _Options>
class PinholeDisparityBrownConradyCameraModel : public PinholeDisparityBrownConradyCameraModelBase<PinholeDisparityBrownConradyCameraModel<_Scalar, _Options>>
{
    typedef PinholeDisparityBrownConradyCameraModelBase<PinholeDisparityBrownConradyCameraModel<_Scalar, _Options>> Base;
public:
    typedef typename Eigen::internal::traits<PinholeDisparityBrownConradyCameraModel<_Scalar,_Options> >::Scalar Scalar;
    typedef typename Eigen::internal::traits<PinholeDisparityBrownConradyCameraModel<_Scalar,_Options> >::ComplexType& ComplexReference;
    typedef const typename Eigen::internal::traits<PinholeDisparityBrownConradyCameraModel<_Scalar,_Options> >::ComplexType& ConstComplexReference;
    
    template<class OtherT> using GetOtherType = PinholeDisparityBrownConradyCameraModel<OtherT, _Options>;
    
    static constexpr unsigned int NumParameters = Base::NumParameters;
    static constexpr unsigned int ParametersToOptimize = Base::ParametersToOptimize;
    static constexpr bool CalibrationSupported = Base::CalibrationSupported;
    
    friend class PinholeDisparityBrownConradyCameraModelBase<PinholeDisparityBrownConradyCameraModel<_Scalar, _Options>>;
    
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    
    EIGEN_DEVICE_FUNC inline PinholeDisparityBrownConradyCameraModel(Scalar fx, Scalar fy, Scalar u0, Scalar v0, Scalar k1 = 0.0, Scalar k2 = 0.0, Scalar k3 = 0.0, Scalar p1 = 0.0, Scalar p2 = 0.0, Scalar skew = 0.0, Scalar w = 0.0, Scalar h = 0.0)
    {
        access_nonconst() << fx , fy , u0 , v0 , k1 , k2 , k3 , p1 , p2 , skew, w, h;
    }
    
    EIGEN_DEVICE_FUNC inline PinholeDisparityBrownConradyCameraModel() : parameters(Eigen::Matrix<Scalar,NumParameters,1>::Zero()) { }
    EIGEN_DEVICE_FUNC inline PinholeDisparityBrownConradyCameraModel(const typename Eigen::internal::traits<PinholeDisparityBrownConradyCameraModel<_Scalar,_Options> >::ComplexType& vec) : parameters(vec) { }
    
    EIGEN_DEVICE_FUNC inline PinholeDisparityBrownConradyCameraModel& operator=(const typename Eigen::internal::traits<PinholeDisparityBrownConradyCameraModel<_Scalar,_Options> >::ComplexType& vec) { access_nonconst() = vec; return *this; }
    
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE ConstComplexReference access() const { return parameters; }
protected:
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE ComplexReference access_nonconst() { return parameters; }
    Eigen::Matrix<Scalar,NumParameters,1> parameters;
};

template<typename T>
inline std::ostream& operator<<(std::ostream& os, const PinholeDisparityBrownConradyCameraModel<T>& p)
{
    os << "PinholeDisparityBrownConradyCameraModel(fx = " << p.fx() << ", fy = " << p.fy() << ", u0 = " << p.u0() << ", v0 = " << p.v0() << ", k1 = " << p.k1() << ", k2 = " << p.k2() << ", k3 = " << p.k3() << ", p1 = " << p.p1() << ", p2 = " << p.p2() << ", s = " << p.skew() << "," << p.width() << " x " << p.height() << ")";
    return os;
}

}

namespace Eigen 
{
/**
 * PinholeDisparityBrownConradyCameraModel, Eigen Map.
 */
template<typename _Scalar, int _Options>
class Map<camera::PinholeDisparityBrownConradyCameraModel<_Scalar>, _Options> : public camera::PinholeDisparityBrownConradyCameraModelBase<Map<camera::PinholeDisparityBrownConradyCameraModel<_Scalar>, _Options>>
{
    typedef camera::PinholeDisparityBrownConradyCameraModelBase<Map<camera::PinholeDisparityBrownConradyCameraModel<_Scalar>, _Options>> Base;
    
public:
    typedef typename internal::traits<Map>::Scalar Scalar;
    typedef typename internal::traits<Map>::ComplexType& ComplexReference;
    typedef const typename internal::traits<Map>::ComplexType& ConstComplexReference;
    
    static constexpr unsigned int NumParameters = Base::NumParameters;
    static constexpr unsigned int ParametersToOptimize = Base::ParametersToOptimize;
    static constexpr bool CalibrationSupported = Base::CalibrationSupported;
    
    friend class camera::PinholeDisparityBrownConradyCameraModelBase<Map<camera::PinholeDisparityBrownConradyCameraModel<_Scalar>, _Options>>;
    
    EIGEN_INHERIT_ASSIGNMENT_EQUAL_OPERATOR(Map)
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE Map(Scalar* coeffs) : parameters(coeffs)  { }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE ConstComplexReference access() const { return parameters; }
protected:
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE ComplexReference access_nonconst() { return parameters; }
    
    Map<Matrix<Scalar,NumParameters,1>,_Options> parameters;
};

/**
 * PinholeDisparityBrownConradyCameraModel, Eigen Map const.
 */
template<typename _Scalar, int _Options>
class Map<const camera::PinholeDisparityBrownConradyCameraModel<_Scalar>, _Options> : public camera::PinholeDisparityBrownConradyCameraModelBase<Map<const camera::PinholeDisparityBrownConradyCameraModel<_Scalar>, _Options>>
{
    typedef camera::PinholeDisparityBrownConradyCameraModelBase<Map<const camera::PinholeDisparityBrownConradyCameraModel<_Scalar>, _Options>> Base;
public:
    typedef typename internal::traits<Map>::Scalar Scalar;
    typedef const typename internal::traits<Map>::ComplexType & ConstComplexReference;
    
    static constexpr unsigned int NumParameters = Base::NumParameters;
    static constexpr unsigned int ParametersToOptimize = Base::ParametersToOptimize;
    static constexpr bool CalibrationSupported = Base::CalibrationSupported;
    
    EIGEN_INHERIT_ASSIGNMENT_EQUAL_OPERATOR(Map)
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE Map(const Scalar* coeffs) : parameters(coeffs) { }
    EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE ConstComplexReference access() const  { return parameters; }
protected:
    const Map<const Matrix<Scalar,NumParameters,1>,_Options> parameters;
};
}
    
#endif // PINHOLE_DISPARITY_BROWN_CONRADY_CAMERA_MODEL_HPP
