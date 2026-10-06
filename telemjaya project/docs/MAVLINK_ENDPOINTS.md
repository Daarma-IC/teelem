# Peta Endpoint Sensor dan MAVLink

## Prinsip utama

LilyGO tidak mengakses sensor internal Pixhawk/Matek. Sensor masuk ke driver
ArduPilot, diproses oleh estimator atau library terkait, lalu hasil yang dipilih
dikeluarkan oleh GCS/MAVLink layer melalui port TELEM.

```text
sensor -> ArduPilot backend/frontend -> estimator/status
       -> GCS_MAVLink -> SERIALx -> TelemJaya Air
       -> RF -> TelemJaya Ground -> Mission Planner
```

`Endpoint` di project ini berarti message MAVLink dan serial connection, bukan
alamat HTTP dan bukan pin sensor langsung.

## Peta data utama

| Sumber/fungsi | Pemilik di ArduPilot | MAVLink utama | Konsumen ground |
|---|---|---|---|
| Identitas dan mode | vehicle/GCS | `HEARTBEAT` | connection, mode, armed state |
| System health | vehicle/GCS | `SYS_STATUS` | health flags, load, voltage |
| Attitude | AHRS/EKF | `ATTITUDE`, `ATTITUDE_QUATERNION` | HUD/artificial horizon |
| Posisi global | AHRS/EKF/GPS | `GLOBAL_POSITION_INT` | map dan altitude |
| GPS receiver | AP_GPS | `GPS_RAW_INT`, `GPS2_RAW` | fix, satellites, HDOP |
| IMU | AP_InertialSensor | `RAW_IMU`, `SCALED_IMU*`, `HIGHRES_IMU` | diagnostics/raw views |
| Vibration/clipping | inertial sensor | `VIBRATION` | vibration monitor |
| Barometer | AP_Baro | `SCALED_PRESSURE*` | pressure/temperature |
| Compass | AP_Compass/AHRS | `RAW_IMU`, `SCALED_IMU*` dan status | heading/diagnostics |
| Airspeed | AP_Airspeed | `VFR_HUD`, `AIRSPEED`, status terkait | airspeed display |
| Battery/power | AP_BattMonitor | `SYS_STATUS`, `BATTERY_STATUS` | voltage/current/capacity |
| Rangefinder | AP_RangeFinder | `DISTANCE_SENSOR` | distance/terrain aid |
| Optical flow | AP_OpticalFlow | `OPTICAL_FLOW`, `OPTICAL_FLOW_RAD` | flow diagnostics |
| ESC telemetry | AP_ESC_Telem | `ESC_TELEMETRY_*`, `ESC_STATUS` | RPM/current/temp |
| RC input | RC input layer | `RC_CHANNELS` | radio calibration/status |
| Servo output | SRV_Channel | `SERVO_OUTPUT_RAW`, `ACTUATOR_OUTPUT_STATUS` | output monitor |
| EKF health | NavEKF/GCS | `EKF_STATUS_REPORT` | EKF status/failsafe |
| Terrain | AP_Terrain | `TERRAIN_*` | terrain transfer/status |
| ADS-B | AP_ADSB | `ADSB_VEHICLE` | traffic display |
| Proximity | AP_Proximity | `OBSTACLE_DISTANCE` | obstacle display/avoidance |
| Camera/gimbal | mount/camera libraries | `CAMERA_*`, `GIMBAL_*`, `MOUNT_*` | payload control/status |
| Text/event | vehicle libraries | `STATUSTEXT`, `EVENT` | messages tab/alerts |

Nama message tertentu tergantung versi ArduPilot, dialect MAVLink, jenis
vehicle, sensor yang aktif, dan request interval dari GCS.

## Endpoint kendali arah ground ke vehicle

| Operasi | MAVLink |
|---|---|
| Membaca parameter | `PARAM_REQUEST_*`, `PARAM_EXT_REQUEST_*` |
| Menulis parameter | `PARAM_SET`, `PARAM_EXT_SET` |
| Mengirim command | `COMMAND_LONG`, `COMMAND_INT` |
| Respons command | `COMMAND_ACK` |
| Mengubah mode | `SET_MODE` atau command terkait |
| Mission upload/download | `MISSION_*` |
| Meminta rate message | `MAV_CMD_SET_MESSAGE_INTERVAL` |
| RTCM correction | `GPS_RTCM_DATA` |
| Manual control | `MANUAL_CONTROL`, RC override bila diaktifkan |
| Timesync | `TIMESYNC`, `SYSTEM_TIME` |

Semua message di atas melewati link byte stream yang sama; firmware radio tidak
perlu membuat handler khusus untuk setiap sensor supaya message dapat lewat.

## Port serial ArduPilot

ArduPilot memilih fungsi konektor TELEM melalui parameter `SERIALx_PROTOCOL`.
Untuk TelemJaya:

```text
SERIALx_PROTOCOL = 2  # MAVLink2
SERIALx_BAUD = 57     # 57600 baud
```

`x` bergantung pada mapping board. Label fisik `TELEM1` tidak selalu berarti
`SERIAL1` pada semua hardware; cek hwdef atau dokumentasi Pixhawk/Matek yang
digunakan.

## Data live versus DataFlash

MAVLink live cocok untuk flight state dan monitoring. Data berikut dapat
menghabiskan bandwidth bila diminta terlalu cepat:

- `HIGHRES_IMU` dan multiple IMU instance;
- ESC telemetry dari banyak motor;
- high-rate actuator output;
- camera image/data payload;
- RTCM correction besar bersamaan dengan parameter transfer.

Untuk analisis getaran dan sensor berkecepatan tinggi, gunakan onboard
DataFlash log. TelemJaya membawa subset live yang diperlukan operator.

## Cara menambah endpoint baru

1. Pastikan sensor memiliki backend ArduPilot dan berstatus healthy.
2. Cari message MAVLink yang diterbitkan library/GCS layer.
3. Pastikan message masuk build dialect ArduPilot.
4. Minta interval message dari Mission Planner atau `MAV_CMD_SET_MESSAGE_INTERVAL`.
5. Ukur byte rate dan packet loss pada mLRS.
6. Catat message, rate, dan consumer baru pada tabel dokumen ini.

Jika sensor belum memiliki message MAVLink, perubahan dilakukan di ArduPilot
terlebih dahulu. HAL LilyGO biasanya tidak perlu diubah.

