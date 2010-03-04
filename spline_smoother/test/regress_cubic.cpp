/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2009, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/** \author Sachin Chitta */

#include <gtest/gtest.h>
#include <motion_planning_msgs/JointTrajectoryWithLimits.h>
#include <spline_smoother/cubic_trajectory.h>


TEST(TestCubicTrajectory, TestCubicTrajectory)
{
  spline_smoother::CubicTrajectory traj;

  // create the input:
  int length = 4;
  int joints = 2;

  motion_planning_msgs::JointTrajectoryWithLimits wpt;
  wpt.trajectory.points.resize(length);
  wpt.trajectory.joint_names.resize(joints);
  wpt.trajectory.joint_names[0] = std::string("test0");
  wpt.trajectory.joint_names[1] = std::string("test1");

  wpt.limits.resize(joints);
  wpt.limits[0].max_velocity = 0.5;
  wpt.limits[0].has_velocity_limits = 1;

  wpt.limits[1].max_velocity = 0.25;
  wpt.limits[1].has_velocity_limits = 1;

  for (int i=0; i<length; i++)
  {
    wpt.trajectory.points[i].positions.resize(joints);
    wpt.trajectory.points[i].velocities.resize(joints);
    wpt.trajectory.points[i].accelerations.resize(joints);
    for(int j=0; j<joints; j++)
    {
      wpt.trajectory.points[i].positions[j] = i+j;
      wpt.trajectory.points[i].velocities[j] = 0.0;
      wpt.trajectory.points[i].accelerations[j] = 0.0;
    }
    wpt.trajectory.points[i].time_from_start = ros::Duration(0.0);
  }

  spline_smoother::SplineTrajectory spline;
  bool success = traj.parameterize(wpt.trajectory,wpt.limits,spline);

  double total_time;
  bool ss = spline_smoother::getTotalTime(spline,total_time);
  EXPECT_TRUE(ss);

  double dT = 0.01;
  int sample_length = (int) (total_time/dT);

  std::vector<double> times;
  times.resize(sample_length);
  for (int i=0; i<sample_length; i++)
  {
    times[i] = dT*i;
  }

  EXPECT_TRUE(success);

  trajectory_msgs::JointTrajectory wpt_out;
  std::vector<double> times_out;
  times_out.resize(length);
  for (int i=0; i<length; i++)
  {
    times_out[i] = i;
  }
  spline_smoother::sampleSplineTrajectory(spline,times_out,wpt_out);
  EXPECT_NEAR(wpt_out.points[0].positions[0], 0.0, 1e-5);
  EXPECT_NEAR(wpt_out.points[1].positions[0], 0.0740741, 1e-5);
  EXPECT_NEAR(wpt_out.points[2].positions[0], 0.259259, 1e-5);
  EXPECT_NEAR(wpt_out.points[3].positions[0], 0.5, 1e-5);

  EXPECT_NEAR(wpt_out.points[0].positions[1], 1.0, 1e-5);
  EXPECT_NEAR(wpt_out.points[1].positions[1], 1.0740741, 1e-5);
  EXPECT_NEAR(wpt_out.points[2].positions[1], 1.259259, 1e-5);
  EXPECT_NEAR(wpt_out.points[3].positions[1], 1.5, 1e-5);
}

TEST(TestCubicTrajectory, TestWithAccelerationLimits1)
{
  spline_smoother::CubicTrajectory traj;

  // create the input:
  int length = 2;
  int joints = 1;

  motion_planning_msgs::JointTrajectoryWithLimits wpt;
  wpt.trajectory.points.resize(length);
  wpt.trajectory.joint_names.resize(joints);
  wpt.trajectory.joint_names[0] = std::string("test0");

  wpt.limits.resize(joints);
  wpt.limits[0].max_velocity = 0.2;
  wpt.limits[0].has_velocity_limits = 1;

  wpt.limits[0].max_acceleration = 0.1;
  wpt.limits[0].has_acceleration_limits = 1;

  for (int i=0; i<length; i++)
  {
    wpt.trajectory.points[i].positions.resize(joints);
    wpt.trajectory.points[i].velocities.resize(joints);
    wpt.trajectory.points[i].accelerations.resize(joints);
    wpt.trajectory.points[i].time_from_start = ros::Duration(0.0);
  }
  wpt.trajectory.points[1].positions[0] = 1.0;
  spline_smoother::SplineTrajectory spline;
  bool success = traj.parameterize(wpt.trajectory,wpt.limits,spline);
  EXPECT_TRUE(success);

  double total_time;
  bool ss = spline_smoother::getTotalTime(spline,total_time);
  EXPECT_TRUE(ss);
  EXPECT_NEAR(total_time,7.745967,1e-5);

  trajectory_msgs::JointTrajectory wpt_out;
  std::vector<double> times_out;
  times_out.push_back(total_time);
  spline_smoother::sampleSplineTrajectory(spline,times_out,wpt_out);

  EXPECT_NEAR(wpt.trajectory.points[1].positions[0],wpt_out.points[0].positions[0],1e-5);
  EXPECT_NEAR(wpt.trajectory.points[1].velocities[0],wpt_out.points[0].velocities[0],1e-5);
}

TEST(TestCubicTrajectory, TestWithAccelerationLimits2)
{
  spline_smoother::CubicTrajectory traj;

  // create the input:
  int length = 2;
  int joints = 1;

  motion_planning_msgs::JointTrajectoryWithLimits wpt;
  wpt.trajectory.points.resize(length);
  wpt.trajectory.joint_names.resize(joints);
  wpt.trajectory.joint_names[0] = std::string("test0");

  wpt.limits.resize(joints);
  wpt.limits[0].max_velocity = 0.2;
  wpt.limits[0].has_velocity_limits = 1;

  wpt.limits[0].max_acceleration = 0.1;
  wpt.limits[0].has_acceleration_limits = 1;

  for (int i=0; i<length; i++)
  {
    wpt.trajectory.points[i].positions.resize(joints);
    wpt.trajectory.points[i].velocities.resize(joints);
    wpt.trajectory.points[i].accelerations.resize(joints);
    wpt.trajectory.points[i].time_from_start = ros::Duration(0.0);
  }
  wpt.trajectory.points[1].positions[0] = 1.0;
  wpt.trajectory.points[1].velocities[0] = -0.2;
  spline_smoother::SplineTrajectory spline;
  bool success = traj.parameterize(wpt.trajectory,wpt.limits,spline);
  EXPECT_TRUE(success);

  double total_time;
  bool ss = spline_smoother::getTotalTime(spline,total_time);
  EXPECT_TRUE(ss);
  EXPECT_NEAR(total_time,12.717798,1e-5);


  wpt.trajectory.points[1].velocities[0] = 0.2;
  success = traj.parameterize(wpt.trajectory,wpt.limits,spline);
  EXPECT_TRUE(success);
  ss = spline_smoother::getTotalTime(spline,total_time);
  EXPECT_TRUE(ss);
  EXPECT_NEAR(total_time,10.6066,1e-5);

  trajectory_msgs::JointTrajectory wpt_out;
  std::vector<double> times_out;
  times_out.push_back(total_time);
  spline_smoother::sampleSplineTrajectory(spline,times_out,wpt_out);

  EXPECT_NEAR(wpt.trajectory.points[1].positions[0],wpt_out.points[0].positions[0],1e-5);
  EXPECT_NEAR(wpt.trajectory.points[1].velocities[0],wpt_out.points[0].velocities[0],1e-5);
}


int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
