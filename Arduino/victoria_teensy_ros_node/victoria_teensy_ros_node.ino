/*
 * Victoria Teensy ROS Node
 * 
 * This code is the ROS node that executes on the Teensy processor
 * for the Victoria RoboMagellan team. It uses the rosserial
 * library to communicate with ROS. It uses Arduino and Teensy
 * libraries to interact with systems and circuits connected to the
 * Teensy processor. This code assumes that a Teensy 3.5 is used.
 */

// Teensy includes
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>  // 
#include <i2c_t3.h>

// ROS includes
#include <ros.h>
#include <geometry_msgs/Twist.h>
#include <nav_msgs/Odometry.h>
#include <tf/transform_broadcaster.h>

// Teensy pin definitions
#define ENCODER_LEFT_PIN_1 29   // Teensy digital pin 29
#define ENCODER_LEFT_PIN_2 30   // Teensy digital pin 30
#define ENCODER_RIGHT_PIN_1 31  // Teensy digital pin 31
#define ENCODER_RIGHT_PIN_2 32  // Teensy digital pin 32
#define BUMPER_LEFT_PIN 14      // Teensy analog pin 14
#define BUMPER_RIGHT_PIN 15     // Teensy analog pin 15

// TRex motor controller
HardwareSerial trex = HardwareSerial();

// Motor encoders
Encoder encoder_left(ENCODER_LEFT_PIN_1, ENCODER_LEFT_PIN_2);
Encoder encoder_right(ENCODER_RIGHT_PIN_1, ENCODER_RIGHT_PIN_2);
unsigned long encoder_left_pos;
unsigned long encoder_right_pos;

// Bumper sensors
#define BUMPER_THRESHOLD 500  // Threshold that will indicate robot should stop
unsigned int bumper_left_base;
unsigned int bumper_right_base;

// ROS node handle
ros::NodeHandle ros_nh;

// ROS cmd_vel subscriber
void cmdVelCallback(const geometry_msgs::Twist& twist_msg);
ros::Subscriber<geometry_msgs::Twist> ros_cmd_vel_sub("cmd_vel", cmdVelCallback);

// ROS Odometry publisher
nav_msgs::Odometry ros_odom_msg;
ros::Publisher ros_odom_pub("odom", &ros_odom_msg);

// ROS Odometry broadcaster
geometry_msgs::TransformStamped ros_odom_transform;
tf::TransformBroadcaster ros_odom_broadcaster;

ros::Time current_time;
ros::Time last_time;
ros::Time last_cmd_vel_time;

char ros_odom_header_frame_id[] = "/odom";
char ros_odom_child_frame_id[] = "/base_link";
  
void setup() {
  // Setup basic I2C master mode pins 18/19, external pullups, 400kHz, 200ms default timeout
  Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000);
  Wire.setDefaultTimeout(200000); // 200ms
    
  // Start serial port to TRex
  trex.begin(19200);

  // Initialize ros node
  ros_nh.initNode();

  // Initialize broadcasters
  ros_odom_transform.header.frame_id = ros_odom_header_frame_id;
  ros_odom_transform.child_frame_id = ros_odom_child_frame_id;
  ros_odom_broadcaster.init(ros_nh);

  // Initialize ros publishers
  ros_odom_msg.header.frame_id = ros_odom_header_frame_id;
  ros_odom_msg.child_frame_id = ros_odom_child_frame_id;
  ros_nh.advertise(ros_odom_pub);

  // Initialize ros subscribers
  ros_nh.subscribe(ros_cmd_vel_sub);
  
  current_time = ros_nh.now();
  last_time = current_time;
  last_cmd_vel_time = current_time;
  encoder_left_pos = 0.0;
  encoder_right_pos = 0.0;
  bumper_left_base = analogRead(BUMPER_LEFT_PIN);
  bumper_right_base = analogRead(BUMPER_RIGHT_PIN);
}

void loop() {
  ros_nh.spinOnce();
  current_time = ros_nh.now();

  // If we have not received any cmd_vel messages for a while,
  // something is wrong, stop the robot.
  
  // Read bumper sensors
  unsigned int new_bumper_left = analogRead(BUMPER_LEFT_PIN);
  unsigned int new_bumper_right = analogRead(BUMPER_RIGHT_PIN);

  // Check bumper thresholds to stop robot
  
  // Read encoders
  unsigned long new_encoder_left_pos = encoder_left.read();
  unsigned long new_encoder_right_pos = encoder_right.read();

  // Read imu data

  // Broadcast odometry transform
  ros_odom_transform.header.stamp = current_time;

  // TODO: set all the transform data

  ros_odom_broadcaster.sendTransform(ros_odom_transform);
  
  // Publish Odometry
  ros_odom_msg.header.stamp = current_time;
  
  // TODO: Set all the odometry data
  
  ros_odom_pub.publish(&ros_odom_msg);
  
  last_time = current_time;
}

void cmdVelCallback(const geometry_msgs::Twist& twist_msg) {
  last_cmd_vel_time = ros_nh.now();
  
  // Do something interesting with the cmd_vel
  setMotors(100, 100);
}

/*
 * Sets the speed for the motors. Values are expected
 * to be between -255 and 255. Values outside this range
 * will be pinned.
 */
void setMotors(int motor_left_speed, int motor_right_speed) {
  byte command_byte_left = 0xC4;
  if (motor_left_speed < 0) {
    command_byte_left = command_byte_left | 0x01;
  } else {
    command_byte_left = command_byte_left | 0x02;
  }
  motor_left_speed = abs(motor_left_speed);
  if (motor_left_speed > 255) {
    motor_left_speed = 255;
  }

  byte command_byte_right = 0xCC;
  if (motor_right_speed < 0) {
    command_byte_right = command_byte_right | 0x01;
  } else {
    command_byte_right = command_byte_right | 0x02;
  }
  motor_right_speed = abs(motor_right_speed);
  if (motor_right_speed > 255) {
    motor_right_speed = 255;
  }
  
  trex.write(command_byte_left);
  trex.write((byte)motor_left_speed);
  trex.write(command_byte_right);
  trex.write((byte)motor_right_speed);
}

