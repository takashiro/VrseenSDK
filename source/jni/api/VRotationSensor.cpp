#include "VRotationSensor.h"
#include "VLockless.h"
#include "VCircularQueue.h"
#include "VQuat.h"

#include <jni.h>
#include <time.h>
#include <fcntl.h>

NV_NAMESPACE_BEGIN

struct KTrackerSensorRawData {
    int32_t AccelX, AccelY, AccelZ;
    int32_t GyroX, GyroY, GyroZ;
};

struct KTrackerSensorZip {
    uint8_t SampleCount;
    uint16_t Timestamp;
    uint16_t LastCommandID;
    int16_t Temperature;

    KTrackerSensorRawData Samples[3];

    int16_t MagX, MagY, MagZ;
};

struct KTrackerMessage {
    V3Vect<float> Acceleration;
    V3Vect<float> RotationRate;
    V3Vect<float> MagneticField;
    float Temperature;
    float TimeDelta;
    double AbsoluteTimeSeconds;
};

class USensor
{
public:
    USensor();
    ~USensor();
    bool update(uint8_t  *buffer);
    long long getLatestTime();
    VQuat<float> getSensorQuaternion();
    V3Vect<float> getAngularVelocity();
    void closeSensor();

private:
    bool pollSensor(KTrackerSensorZip* data,uint8_t  *buffer);
    void process(KTrackerSensorZip* data);
    void updateQ(KTrackerMessage *msg);
    V3Vect<float> gyrocorrect(const V3Vect<float> &gyro, const V3Vect<float> &accel, const float DeltaT);

private:
    int fd_;
    VQuat<float> q_;
    bool first_;
    int step_;
    uint first_real_time_delta_;
    vuint16 last_timestamp_;
    vuint32 full_timestamp_;
    vuint8 last_sample_count_;
    longlong latest_time_;
    V3Vect<float> last_acceleration_;
    V3Vect<float> last_rotation_rate_;
    V3Vect<float> last_corrected_gyro_;
    V3Vect<float> gyro_offset_;

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
                float removed = VDeque::takeFirst();
                m_total -= removed;
            }
            m_total += e;
            VDeque::append(e);
        }

        float total() const { return m_total; }

        float mean() const { return VDeque::isEmpty() ? 0.0f : (total() / (float) size()); }

    private:
        float m_total;
    };
    Filter tilt_filter_;
};

struct VRotationSensor::Private
{
    VLockless<VRotationState> state;
};

void VRotationSensor::setState(const VRotationState &state)
{
    d->state.setState(state);
}

VRotationState VRotationSensor::state() const
{
    return d->state.state();
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

VRotationState VRotationSensor::predictState(double timestamp) const
{
    //@to-do: implement this
    VRotationState state = this->state();
    state.timestamp = timestamp;
    return state;
}

VRotationSensor::VRotationSensor()
    : d(new Private)
{
}

NV_NAMESPACE_END

NV_USING_NAMESPACE

extern "C" {

JNIEXPORT jlong JNICALL Java_com_vrseen_sensor_NativeUSensor_ctor
  (JNIEnv *env, jclass obj)
{
    return reinterpret_cast<jlong>(new USensor());
}

JNIEXPORT jboolean JNICALL Java_com_vrseen_sensor_NativeUSensor_update
  (JNIEnv *env, jclass obj, jlong jk_sensor, jbyteArray data)
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
  (JNIEnv *env, jclass obj, jlong jk_sensor)
{
    USensor* u_sensor = reinterpret_cast<USensor*>(jk_sensor);
     return u_sensor->getLatestTime();
}

JNIEXPORT jfloatArray JNICALL Java_com_vrseen_sensor_NativeUSensor_getData
  (JNIEnv *env, jclass obj, jlong jk_sensor)
{
    USensor* u_sensor = reinterpret_cast<USensor*>(jk_sensor);

    VQuat<float> rotation = u_sensor->getSensorQuaternion();
    V3Vect<float> angular_velocity = u_sensor->getAngularVelocity();

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

static long long getCurrentTime() {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts); // Works on Linux
    long long time = static_cast<long long>(ts.tv_sec)
            * static_cast<long long>(1000000000)
            + static_cast<long long>(ts.tv_nsec);
    return time;
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
    state.timestamp = timestamp;
    state.w = w;
    state.y = y;
    state.z = z;
    state.gyroX = gryoX;
    state.gyroY = gryoY;
    state.gyroZ = gryoZ;
    VRotationSensor::instance()->setState(state);
}

}


USensor::USensor() :
        fd_(-1), q_(), first_(true), step_(0), first_real_time_delta_(0.0f), last_timestamp_(
                0), full_timestamp_(0), last_sample_count_(0), latest_time_(0), last_acceleration_(
                0.0f, 0.0f, 0.0f), last_rotation_rate_(0.0f, 0.0f, 0.0f), last_corrected_gyro_(
                0.0f, 0.0f, 0.0f), gyro_offset_(0.0f, 0.0f, 0.0f), tilt_filter_() {
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

    latest_time_ = getCurrentTime();

    process(&data);

    VRotationState state;
    state.w = q_.w;
    state.x = q_.x;
    state.y = q_.y;
    state.z = q_.z;
    state.gyroX = last_corrected_gyro_.x;
    state.gyroY = last_corrected_gyro_.y;
    state.gyroZ = last_corrected_gyro_.z;
    state.timestamp = latest_time_;
    VRotationSensor::instance()->setState(state);

    return true;
}

long long USensor::getLatestTime() {
    return latest_time_;
}

VQuat<float> USensor::getSensorQuaternion() {
    return q_;
}

V3Vect<float> USensor::getAngularVelocity() {
    return last_corrected_gyro_;
}

void USensor::closeSensor() {
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
}

bool USensor::pollSensor(KTrackerSensorZip* data,uint8_t  *buffer) {
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
        last_acceleration_ = V3Vect<float>(0, 0, 0);
        last_rotation_rate_ = V3Vect<float>(0, 0, 0);
        first_ = false;

        // This is our baseline sensor to host time delta,
        // it will be adjusted with each new message.
        full_timestamp_ = data->Timestamp;
        first_real_time_delta_ = now - (full_timestamp_ * timeUnit);
    } else {
        unsigned timestampDelta;

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
        sensors.Acceleration = V3Vect<float>(data->Samples[i].AccelX,
                data->Samples[i].AccelY, data->Samples[i].AccelZ) * 0.0001f;
        sensors.RotationRate = V3Vect<float>(data->Samples[i].GyroX,
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
    V3Vect<float> gyro = msg->RotationRate;
    V3Vect<float> accel = msg->Acceleration;
    const float DeltaT = msg->TimeDelta;
    V3Vect<float> gyroCorrected = gyrocorrect(gyro, accel, DeltaT);
    last_corrected_gyro_ = gyroCorrected;

    // Update the orientation quaternion based on the corrected angular velocity vector
    float gyro_length = gyroCorrected.Length();
    if (gyro_length != 0.0f) {
        float angle = gyro_length * DeltaT;
        q_ = q_ * VQuat<float>(gyroCorrected.Normalized() * sin(angle * 0.5f), angle);
    }

    step_++;

    // Normalize error
    if (step_ % 500 == 0) {
        q_.Normalize();
    }
}

V3Vect<float> USensor::gyrocorrect(const V3Vect<float> &gyro, const V3Vect<float> &accel, const float DeltaT) {
    // Small preprocessing
    VQuat<float> Qinv = q_.Inverted();
    V3Vect<float> up = Qinv.Rotate(V3Vect<float>(0, 1, 0));
    V3Vect<float> gyroCorrected = gyro;

    bool EnableGravity = true;
    bool valid_accel = accel.Length() > 0.001f;

    if (EnableGravity && valid_accel) {
        gyroCorrected -= gyro_offset_;

        const float spikeThreshold = 0.01f;
        const float gravityThreshold = 0.1f;
        float proportionalGain = 0.25f, integralGain = 0.0f;

        V3Vect<float> accel_normalize = accel.Normalized();
        V3Vect<float> up_normalize = up.Normalized();
        V3Vect<float> correction = accel_normalize.Cross(up_normalize);
        float cosError = accel_normalize.Dot(up_normalize);
        const float Tolerance = 0.00001f;
        V3Vect<float> tiltCorrection = correction
                * sqrtf(2.0f / (1 + cosError + Tolerance));

        if (step_ > 5) {
            // Spike detection
            float tiltAngle = up.Angle(accel);
            tilt_filter_.append(tiltAngle);
            if (tiltAngle > tilt_filter_.mean() + spikeThreshold)
                proportionalGain = integralGain = 0;
            // Acceleration detection
            const float gravity = 9.8f;
            if (fabs(accel.Length() / gravity - 1) > gravityThreshold)
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
