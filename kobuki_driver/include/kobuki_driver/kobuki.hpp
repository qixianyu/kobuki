/**
 * @file /include/kobuki/kobuki.hpp
 *
 * @brief Cpp device driver core interface.
 **/
/*****************************************************************************
 ** Ifdefs
 *****************************************************************************/

#ifndef KOBUKI_HPP_
#define KOBUKI_HPP_

/*****************************************************************************
 ** Includes
 *****************************************************************************/

#include <iostream>
#include <string>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <ecl/threads.hpp>
#include <ecl/devices.hpp>
#include <ecl/time.hpp>
#include <ecl/mobile_robot.hpp>
#include <ecl/exceptions/standard_exception.hpp>
#include "packet_handler/packet_finder.hpp"
#include "parameters.hpp"
#include "modules/cliff.hpp"
#include "modules/core_sensors.hpp"
#include "modules/current.hpp"
#include "modules/gp_input.hpp"
#include "modules/inertia.hpp"
#include "modules/dock_ir.hpp"
#include "modules/firmware.hpp"
#include "modules/hardware.hpp"
#include "command.hpp"
#include "simulation.hpp"
#include "led_array.hpp"
#include "version_info.hpp"

/*****************************************************************************
 ** Namespaces
 *****************************************************************************/

namespace kobuki
{

/*****************************************************************************
 ** Definitions
 *****************************************************************************/

union union_sint16
{
  short word;
  unsigned char byte[2];
};

/*****************************************************************************
** Parent Interface
*****************************************************************************/

class PacketFinder : public PacketFinderBase
{
public:
  virtual ~PacketFinder() {}
  bool checkSum();
};

/*****************************************************************************
 ** Interface [Kobuki]
 *****************************************************************************/
/**
 * @brief  The core kobuki driver class.
 *
 * This connects to the outside world via sigslots and get accessors.
 **/
class Kobuki : public ecl::Threadable
{
public:
  Kobuki() :
    last_velocity_left(0.0),
    last_velocity_right(0.0),
    is_connected(false), is_running(false), is_enabled(false),
    tick_to_mm(0.0845813406577f), tick_to_rad(0.00201384144460884f)
  {
  }
  ~Kobuki()
  {
    serial.close();
    is_connected = false;
    is_running = false;
    is_enabled = false;
  }

  /*********************
   ** Configuration
   **********************/
  void runnable();
  void init(Parameters &parameters) throw (ecl::StandardException);
  bool connected() const
  {
    return is_connected;
  }
  bool isEnabled() const
  {
    return is_enabled;
  }
  bool enable();
  bool disable();
  void close();

  /******************************************
  ** User Friendly Api
  *******************************************/
  ecl::Angle<double> getHeading() const;
  double getAngularVelocity() const;
  VersionInfo versionInfo() const { return VersionInfo(firmware.data.version, hardware.data.version); }

  /******************************************
  ** Raw Data Api
  *******************************************/
  // streamed
  void getCoreSensorData(CoreSensors::Data&) const;
  void getDockIRData(DockIR::Data&) const;
  void getCliffData(Cliff::Data&) const;
  void getCurrentData(Current::Data&) const;
  void getGpInputData(GpInput::Data&) const;

  /*********************
  ** Feedback
  **********************/
  void getWheelJointStates(double &wheel_left_angle, double &wheel_left_angle_rate,
                            double &wheel_right_angle, double &wheel_right_angle_rate);
  void updateOdometry(ecl::Pose2D<double> &pose_update,
                      ecl::linear_algebra::Vector3d &pose_update_rates);
  /*********************
  ** Soft Commands
  **********************/
  void resetOdometry();

  /*********************
  ** Hard Commands
  **********************/
  void toggleLed(const enum LedNumber &number, const enum LedColour &colour);
  void setBaseControlCommand(double, double);
  void sendBaseControlCommand();
  void sendCommand(Command command);

private:
  ecl::StopWatch stopwatch;

  unsigned short last_timestamp;
  double last_velocity_left, last_velocity_right;
  double last_diff_time;

  unsigned short last_tick_left, last_tick_right;
  double last_rad_left, last_rad_right;
  double last_mm_left, last_mm_right;

  short v, w;
  short radius;
  short speed;
  double bias; //wheelbase, wheel_to_wheel, in [m]
  double wheel_radius;
  int imu_heading_offset;

  std::string device_type;
  std::string protocol_version;
  bool is_connected; // True if there's a serial/usb connection open.
  bool is_running;
  bool is_enabled;

  unsigned int count;
  const double tick_to_mm, tick_to_rad;

  ecl::Serial serial;

  // Streamed Data
  CoreSensors core_sensors;
  Inertia inertia;
  DockIR dock_ir;
  Cliff cliff;
  Current current;
  GpInput gp_input;
  // Service Payloads
  Hardware hardware;
  Firmware firmware;

  Simulation simulation;

  Command kobuki_command;

  PacketFinder packet_finder;
  PacketFinder::BufferType data_buffer;
  ecl::PushAndPop<unsigned char> command_buffer;
  ecl::Signal<> sig_stream_data, sig_version_info;

  ecl::Signal<const std::string&> sig_debug, sig_info, sig_warn, sig_error;

  boost::shared_ptr<ecl::DifferentialDrive::Kinematics> kinematics;
  bool is_simulation;
};

} // namespace kobuki

#endif /* KOBUKI_HPP_ */
