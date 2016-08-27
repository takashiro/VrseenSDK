#include "VRotationSensor.h"
#include "VLockless.h"
#include "VCircularQueue.h"
#include "VAlgorithm.h"
#include "VLog.h"
#include "VMutex.h"
#include "VTimer.h"
#include "VQuat.h"

#include <jni.h>
#include <fcntl.h>

NV_NAMESPACE_BEGIN

struct VRotationSensor::Private
{
    VLockless<VRotationState> state;
    VLockless<VQuatf> recenter;
    VMutex recenterMutex;
    bool initialized;

    Private()
        : initialized(false)
    {
    }
};

void VRotationSensor::setState(const VRotationState &state)
{
    if (!d->initialized) {
        float yaw, pitch, roll;
        state.GetEulerAngles<VAxis_Y, VAxis_X, VAxis_Z>(&yaw, &pitch, &roll);
        d->recenter.setState(VQuatf(VAxis_Y, -yaw));
        d->initialized = true;
    } else {
        VRotationState recentered = state;
        VQuatf base = d->recenter.state() * state;
        recentered.w = base.w;
        recentered.x = base.x;
        recentered.y = base.y;
        recentered.z = base.z;
        d->state.setState(state);
    }
}

VRotationState VRotationSensor::state() const
{
    return d->state.state();
}

void VRotationSensor::recenterYaw()
{
    // get the current state
    VRotationState state = this->state();

    // get the yaw in the current state
    float yaw, pitch, roll;
    state.GetEulerAngles<VAxis_Y, VAxis_X, VAxis_Z>(&yaw, &pitch, &roll);

    // get the pose that adjusts the yaw
    VQuatf yawAdjustment(VAxis_Y, -yaw);
    state = yawAdjustment;

    // To allow RecenterYaw() to be called from multiple threads we need a mutex
    // because LocklessUpdater is only safe for single producer cases.
    d->recenterMutex.lock();
    d->state.setState(state);
    d->recenterMutex.unlock();
}

void VRotationSensor::setYaw(float newYaw)
{
    // get the current state
    VRotationState state = this->state();

    // get the yaw in the current state
    float yaw, pitch, roll;
    state.GetEulerAngles<VAxis_Y, VAxis_X, VAxis_Z>(&yaw, &pitch, &roll);

    // get the pose that adjusts the yaw
    VQuatf yawAdjustment(VAxis_Y, newYaw - yaw);
    state = yawAdjustment;

    // To allow SetYaw() to be called from multiple threads we need a mutex
    // because LocklessUpdater is only safe for single producer cases.
    d->recenterMutex.lock();
    d->state.setState(state);
    d->recenterMutex.unlock();
}

VRotationSensor *VRotationSensor::instance()
{
    static VRotationSensor sensor;
    return &sensor;
}

VRotationSensor::~VRotationSensor()
{
    delete d;
}

static VQuatf calcPredictedPose(const VRotationState &pose, float predictionDt)
{
    float speed = pose.gyro.length();

    const float slope = 0.2; // The rate at which the dynamic prediction interval varies
    float candidateDt = slope * speed; // TODO: Replace with smoothstep function

    float dynamicDt = predictionDt;

    // Choose the candidate if it is shorter, to improve stability
    if (candidateDt < predictionDt)
        dynamicDt = candidateDt;

    const float MAX_DELTA_TIME = 1.0f / 10.0f;
    dynamicDt = std::min(std::max(dynamicDt, 0.0f), MAX_DELTA_TIME);

    VQuatf state = pose;
    if (speed > 0.001)
        state = pose * VQuatf(pose.gyro, speed * dynamicDt);

    if (state.x == 0 && state.z == 0 && state.y == 0)
        return pose;

    return state;
}

VRotationState VRotationSensor::predictState(double timestamp) const
{
    //lockless state fetch
    VRotationState state = this->state();


    // Delta time from the last processed message
    double pdt = timestamp - state.timestamp;
    const VQuatf recenter = d->recenter.state();

    // Do prediction logic
    VQuatf orientation = recenter * calcPredictedPose(state, pdt);
    state = orientation;
    state.timestamp = timestamp;

    return state;
}

VRotationSensor::VRotationSensor()
    : d(new Private)
{
}


struct KTrackerSensorZip {
    uint8_t SampleCount;
    uint16_t Timestamp;
    uint16_t LastCommandID;
    int16_t Temperature;

    struct RawData {
        int32_t AccelX, AccelY, AccelZ;
        int32_t GyroX, GyroY, GyroZ;
    };
    RawData Samples[3];

    int16_t MagX, MagY, MagZ;
};

struct KTrackerMessage {
    VVect3f Acceleration;
    VVect3f RotationRate;
    VVect3f MagneticField;
    float Temperature;
    float TimeDelta;
    double AbsoluteTimeSeconds;
};

class USensor
{
public:
    USensor();
    ~USensor();
    bool update(uint8_t *buffer);
    longlong getLatestTime();
    VQuat<float> getSensorQuaternion();
    VVect3f getAngularVelocity();

private:
    bool pollSensor(KTrackerSensorZip* data,uint8_t  *buffer);
    void process(KTrackerSensorZip* data);
    void updateQ(KTrackerMessage *msg);
    VVect3f gyrocorrect(const VVect3f &gyro, const VVect3f &accel, const float DeltaT);

private:
    VRotationState m_state;
    bool first_;
    int step_;
    uint first_real_time_delta_;
    vuint16 last_timestamp_;
    vuint32 full_timestamp_;
    vuint8 last_sample_count_;
    VVect3f last_acceleration_;
    VVect3f last_rotation_rate_;
    VVect3f gyro_offset_;

    class Filter: public VCircularQueue<float>
    {
    public:
        Filter(int capacity = 20)
            : VCircularQueue(capacity)
            , m_total(0.0f)
        {
        }

        void append(float e) {
            if (isFull()) {
                float removed = first();
                m_total -= removed;
            }
            m_total += e;
            VCircularQueue::append(e);
        }

        float total() const { return m_total; }

        float mean() const { return VCircularQueue::isEmpty() ? 0.0f : (total() / (float) size()); }

    private:
        float m_total;
    };
    Filter tilt_filter_;
};

static long long getCurrentTime()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); // Works on Linux
    long long time = static_cast<long long>(ts.tv_sec)
            * static_cast<long long>(1000000000)
            + static_cast<long long>(ts.tv_nsec);
    return time;
}

USensor::USensor()
    : first_(true)
    , step_(0)
    , first_real_time_delta_(0.0f)
    , last_timestamp_(0)
    , full_timestamp_(0)
    , last_sample_count_(0)
{
}

USensor::~USensor() {
}

bool USensor::update(uint8_t  *buffer) {
    KTrackerSensorZip data;
    /*
    if (!pollSensor(&data, buffer)) {
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
        return false;
    }
    */
    pollSensor(&data, buffer);

    m_state.timestamp = VTimer::Seconds();
    process(&data);

    VRotationSensor::instance()->setState(m_state);

    return true;
}

longlong USensor::getLatestTime()
{
    return m_state.timestamp * 1000000000;
}

VQuat<float> USensor::getSensorQuaternion()
{
    return m_state;
}

VVect3f USensor::getAngularVelocity()
{
    return m_state.gyro;
}

bool USensor::pollSensor(KTrackerSensorZip* data,uint8_t  *buffer)
{
   /* if (fd_ < 0) {
        fd_ = open("/dev/ovr0", O_RDONLY);
    }
    if (fd_ < 0) {
        return false;
    }

    struct pollfd pfds;
    pfds.fd = fd_;
    pfds.events = POLLIN;

    int n = poll(&pfds, 1, 100);
    if (n > 0 && (pfds.revents & POLLIN)) {
        uint8_t buffer[100];
        int r = read(fd_, buffer, 100);
        if (r < 0) {
            LOGI("OnSensorEvent() read error %d", r);
            return false;
        }*/

        data->SampleCount = buffer[1];
        data->Timestamp = (uint16_t)(*(buffer + 3) << 8)
                | (uint16_t)(*(buffer + 2));
        data->LastCommandID = (uint16_t)(*(buffer + 5) << 8)
                | (uint16_t)(*(buffer + 4));
        data->Temperature = (int16_t)(*(buffer + 7) << 8)
                | (int16_t)(*(buffer + 6));

        //LOGI("data:%d,%d,%d",buffer[8] ,buffer[9],buffer[10]);

        for (int i = 0; i < (data->SampleCount > 3 ? 3 : data->SampleCount);
                ++i)

        {
            //int i=0;



            struct {
                int32_t x :21;
            } s;


            //s.x = 0;

            //LOGI("sx = %d ", s.x);

            //LOGI("data:%d,%d,%d",(buffer[0 + 8 + 16 * i] << 13) ,(buffer[1 + 8 + 16 * i] << 5),((buffer[2 + 8 + 16 * i] & 0xF8) >> 3));

            data->Samples[i].AccelX = s.x = (buffer[0 + 8 + 16 * i] << 13)
                    | (buffer[1 + 8 + 16 * i] << 5)
                    | ((buffer[2 + 8 + 16 * i] & 0xF8) >> 3);
            data->Samples[i].AccelY = s.x = ((buffer[2 + 8 + 16 * i] & 0x07)
                    << 18) | (buffer[3 + 8 + 16 * i] << 10)
                    | (buffer[4 + 8 + 16 * i] << 2)
                    | ((buffer[5 + 8 + 16 * i] & 0xC0) >> 6);
            data->Samples[i].AccelZ = s.x = ((buffer[5 + 8 + 16 * i] & 0x3F)
                    << 15) | (buffer[6 + 8 + 16 * i] << 7)
                    | (buffer[7 + 8 + 16 * i] >> 1);

            data->Samples[i].GyroX = s.x = (buffer[0 + 16 + 16 * i] << 13)
                    | (buffer[1 + 16 + 16 * i] << 5)
                    | ((buffer[2 + 16 + 16 * i] & 0xF8) >> 3);
            data->Samples[i].GyroY = s.x = ((buffer[2 + 16 + 16 * i] & 0x07)
                    << 18) | (buffer[3 + 16 + 16 * i] << 10)
                    | (buffer[4 + 16 + 16 * i] << 2)
                    | ((buffer[5 + 16 + 16 * i] & 0xC0) >> 6);
            data->Samples[i].GyroZ = s.x = ((buffer[5 + 16 + 16 * i] & 0x3F)
                    << 15) | (buffer[6 + 16 + 16 * i] << 7)
                    | (buffer[7 + 16 + 16 * i] >> 1);

        }

        data->MagX = (int16_t)(*(buffer + 57) << 8) | (int16_t)(*(buffer + 56));
        data->MagY = (int16_t)(*(buffer + 59) << 8) | (int16_t)(*(buffer + 58));
        data->MagZ = (int16_t)(*(buffer + 61) << 8) | (int16_t)(*(buffer + 60));


        //uint8_t a,b,c,d;


        //a = (data->Samples[0].AccelX & 0xff000000)>>24;
        //b = (data->Samples[0].AccelX & 0x00ff0000)>>16;
        //c = (data->Samples[0].AccelX & 0x0000ff00)>>8;
        //d = (data->Samples[0].AccelX & 0x000000ff);


        //if (a == 0xff){
        //	LOGI("count %d raw data:%ld,%d,%d,%d,%d ",data->SampleCount,data->Samples[0].AccelX,a,b,c,d);
        //}
        //LOGI("raw data:%lu,%lu,%",data->Samples[0].AccelX,data->Samples[0].AccelY,data->Samples[0].AccelZ);


        return true;


    //return false;
}

void USensor::process(KTrackerSensorZip* data) {
    const float timeUnit = (1.0f / 1000.f);

    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    const double now = tp.tv_sec + tp.tv_nsec * 0.000000001;

    //double absoluteTimeSeconds = 0.0;

    if (first_) {
        last_acceleration_ = VVect3f(0, 0, 0);
        last_rotation_rate_ = VVect3f(0, 0, 0);
        first_ = false;

        // This is our baseline sensor to host time delta,
        // it will be adjusted with each new message.
        full_timestamp_ = data->Timestamp;
        first_real_time_delta_ = now - (full_timestamp_ * timeUnit);
    } else {
        uint timestampDelta;

        if (data->Timestamp < last_timestamp_) {
            // The timestamp rolled around the 16 bit counter, so FullTimeStamp
            // needs a high word increment.
            full_timestamp_ += 0x10000;
            timestampDelta = ((((int) data->Timestamp) + 0x10000)
                    - (int) last_timestamp_);
        } else {
            timestampDelta = (data->Timestamp - last_timestamp_);
        }
        // Update the low word of FullTimeStamp
        full_timestamp_ = (full_timestamp_ & ~0xffff) | data->Timestamp;

        // If this timestamp, adjusted by our best known delta, would
        // have the message arriving in the future, we need to adjust
        // the delta down.
        if (full_timestamp_ * timeUnit + first_real_time_delta_ > now) {
            first_real_time_delta_ = now - (full_timestamp_ * timeUnit);
        } else {
            // Creep the delta by 100 microseconds so we are always pushing
            // it slightly towards the high clamping case, instead of having to
            // worry about clock drift in both directions.
            first_real_time_delta_ += 0.0001;
        }
        // This will be considered the absolute time of the last sample in
        // the message.  If we are double or tripple stepping the samples,
        // their absolute times will be adjusted backwards.
        /*absoluteTimeSeconds = full_timestamp_ * timeUnit
                + first_real_time_delta_;*/

        // If we missed a small number of samples, replicate the last sample.
        if ((timestampDelta > last_sample_count_) && (timestampDelta <= 254)) {
            KTrackerMessage sensors;
            sensors.TimeDelta = (timestampDelta - last_sample_count_)
                    * timeUnit;
            sensors.Acceleration = last_acceleration_;
            sensors.RotationRate = last_rotation_rate_;

            updateQ(&sensors);
        }
    }

    KTrackerMessage sensors;
    int iterations = data->SampleCount;

    if (data->SampleCount > 3) {
        iterations = 3;
        sensors.TimeDelta = (data->SampleCount - 2) * timeUnit;
    } else {
        sensors.TimeDelta = timeUnit;
    }

    for (int i = 0; i < iterations; ++i) {
        sensors.Acceleration = VVect3f(data->Samples[i].AccelX,
                data->Samples[i].AccelY, data->Samples[i].AccelZ) * 0.0001f;
        sensors.RotationRate = VVect3f(data->Samples[i].GyroX,
                data->Samples[i].GyroY, data->Samples[i].GyroZ) * 0.0001f;

        updateQ(&sensors);

        // TimeDelta for the last two sample is always fixed.
        sensors.TimeDelta = timeUnit;
    }

    last_sample_count_ = data->SampleCount;
    last_timestamp_ = data->Timestamp;
    last_acceleration_ = sensors.Acceleration;
    last_rotation_rate_ = sensors.RotationRate;
}

void USensor::updateQ(KTrackerMessage *msg) {
    // Put the sensor readings into convenient local variables
    VVect3f gyro = msg->RotationRate;
    VVect3f accel = msg->Acceleration;
    const float DeltaT = msg->TimeDelta;
    m_state.gyro = gyrocorrect(gyro, accel, DeltaT);

    // Update the orientation quaternion based on the corrected angular velocity vector
    float gyro_length = m_state.gyro.length();
    if (gyro_length != 0.0f) {
        float angle = gyro_length * DeltaT;
        VQuatf q = m_state * VQuatf(m_state.gyro.normalized() * sin(angle * 0.5f), angle);
        m_state.w = q.w;
        m_state.x = q.x;
        m_state.y = q.y;
        m_state.z = q.z;
    }

    step_++;

    // Normalize error
    if (step_ % 500 == 0) {
        m_state.Normalize();
    }
}

VVect3f USensor::gyrocorrect(const VVect3f &gyro, const VVect3f &accel, const float DeltaT) {
    // Small preprocessing
    VQuatf Qinv = m_state.Inverted();
    VVect3f up = Qinv.Rotate(VVect3f(0, 1, 0));
    VVect3f gyroCorrected = gyro;

    bool EnableGravity = true;
    bool valid_accel = accel.length() > 0.001f;

    if (EnableGravity && valid_accel) {
        gyroCorrected -= gyro_offset_;

        const float spikeThreshold = 0.01f;
        const float gravityThreshold = 0.1f;
        float proportionalGain = 0.25f, integralGain = 0.0f;

        VVect3f accel_normalize = accel.normalized();
        VVect3f up_normalize = up.normalized();
        VVect3f correction = accel_normalize.crossProduct(up_normalize);
        float cosError = accel_normalize.dotProduct(up_normalize);
        const float Tolerance = 0.00001f;
        VVect3f tiltCorrection = correction * sqrtf(2.0f / (1 + cosError + Tolerance));

        if (step_ > 5) {
            // Spike detection
            float tiltAngle = up.angleTo(accel);
            tilt_filter_.append(tiltAngle);
            if (tiltAngle > tilt_filter_.mean() + spikeThreshold)
                proportionalGain = integralGain = 0;
            // Acceleration detection
            const float gravity = 9.8f;
            if (fabs(accel.length() / gravity - 1) > gravityThreshold)
                integralGain = 0;
        } else {
            // Apply full correction at the startup
            proportionalGain = 1 / DeltaT;
            integralGain = 0;
        }

        gyroCorrected += (tiltCorrection * proportionalGain);
        gyro_offset_ -= (tiltCorrection * integralGain * DeltaT);
    } else {
        vInfo("invalidaccel");
    }

    return gyroCorrected;
}

NV_NAMESPACE_END

NV_USING_NAMESPACE

extern "C" {

JNIEXPORT jlong JNICALL Java_com_vrseen_sensor_NativeUSensor_ctor(JNIEnv *, jclass)
{
    return reinterpret_cast<jlong>(new USensor());
}

JNIEXPORT jboolean JNICALL Java_com_vrseen_sensor_NativeUSensor_update
  (JNIEnv *env, jclass, jlong jk_sensor, jbyteArray data)
{
    USensor* u_sensor = reinterpret_cast<USensor*>(jk_sensor);
    jbyte tmp[100];
    uint8_t tmp1[100];


    (*env).GetByteArrayRegion(data,0,100,tmp);

    for(int i=0; i<100; i++){
        tmp1[i] = tmp[i];
    }

    return u_sensor->update(tmp1);
}

JNIEXPORT jlong JNICALL Java_com_vrseen_sensor_NativeUSensor_getTimeStamp
  (JNIEnv *, jclass, jlong jk_sensor)
{
    USensor* u_sensor = reinterpret_cast<USensor*>(jk_sensor);
     return u_sensor->getLatestTime();
}

JNIEXPORT jfloatArray JNICALL Java_com_vrseen_sensor_NativeUSensor_getData
  (JNIEnv *env, jclass, jlong jk_sensor)
{
    USensor* u_sensor = reinterpret_cast<USensor*>(jk_sensor);

    VQuat<float> rotation = u_sensor->getSensorQuaternion();
    VVect3f angular_velocity = u_sensor->getAngularVelocity();

    jfloatArray jdata = env->NewFloatArray(7);
    jfloat data[7];
    data[0] = rotation.w;
    data[1] = rotation.x;
    data[2] = rotation.y;
    data[3] = rotation.z;
    data[4] = angular_velocity.x;
    data[5] = angular_velocity.y;
    data[6] = angular_velocity.z;
    env->SetFloatArrayRegion(jdata, 0, 7, data);

    return jdata;
}

JNIEXPORT jlong JNICALL Java_com_vrseen_sensor_NativeTime_getCurrentTime
  (JNIEnv *, jclass)
{
    return getCurrentTime();
}

JNIEXPORT void JNICALL Java_com_vrseen_sensor_RotationSensor_update
  (JNIEnv *, jclass, jlong timestamp, jfloat w, jfloat x, jfloat y, jfloat z, jfloat gryoX, jfloat gryoY, jfloat gryoZ)
{
    VRotationState state;
    state.timestamp = timestamp * 0.000000001;
    state.gyro.x = gryoX;
    state.gyro.y = gryoY;
    state.gyro.z = gryoZ;

    static VQuatf correct(VAxis_X, -90 * 3.1415926 / 180);
    VQuatf raw(-y, x, z, w);
    raw = correct * raw;
    state.w = raw.w;
    state.x = raw.x;
    state.y = raw.y;
    state.z = raw.z;

    VRotationSensor::instance()->setState(state);
}

}
