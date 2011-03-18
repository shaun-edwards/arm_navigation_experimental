/*********************************************************************
 *
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2008, Willow Garage, Inc.
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
 *
 *  \author Sachin Chitta, Ioan Sucan
 *********************************************************************/
#include <ros/ros.h>

#include <planning_environment/models/collision_models_interface.h>
#include <planning_environment/models/model_utils.h>

#include <planning_environment_msgs/GetJointTrajectoryValidity.h>
#include <planning_environment_msgs/GetStateValidity.h>

namespace planning_environment
{
class PlanningSceneValidityServer
{
public:	
  PlanningSceneValidityServer() :
  private_handle_("~")
  {
    std::string robot_description_name = root_handle_.resolveName("robot_description", true);
    collision_models_interface_ = new planning_environment::CollisionModelsInterface(robot_description_name);

    get_trajectory_validity_service_ = private_handle_.advertiseService("get_trajectory_validity", &PlanningSceneValidityServer::getTrajectoryValidity, this);
    get_state_validity_service_ = private_handle_.advertiseService("get_state_validity", &PlanningSceneValidityServer::getStateValidity, this);
  }
	
  virtual ~PlanningSceneValidityServer()
  {
    delete collision_models_interface_;
  }

  bool getTrajectoryValidity(planning_environment_msgs::GetJointTrajectoryValidity::Request &req, 
                             planning_environment_msgs::GetJointTrajectoryValidity::Response &res) 
  {
    return true;
  }

  bool getStateValidity(planning_environment_msgs::GetStateValidity::Request &req, 
                        planning_environment_msgs::GetStateValidity::Response &res) 
  {
    if(collision_models_interface_->getPlanningSceneState() == NULL) {
      res.error_code.val = res.error_code.COLLISION_CHECKING_UNAVAILABLE;
      return true;
    }
    planning_environment::setRobotStateAndComputeTransforms(req.robot_state,
                                                            *collision_models_interface_->getPlanningSceneState());
    motion_planning_msgs::Constraints goal_constraints;
    if(req.check_goal_constraints) {
      goal_constraints = req.goal_constraints;
    }
    motion_planning_msgs::Constraints path_constraints;
    if(req.check_path_constraints) {
      path_constraints = req.path_constraints;
    }
    std::vector<std::string> joint_names;
    if(req.check_joint_limits) {
      joint_names = req.robot_state.joint_state.name;
    }
    collision_models_interface_->isKinematicStateValid(*collision_models_interface_->getPlanningSceneState(),
                                                       joint_names,
                                                       res.error_code,
                                                       goal_constraints,
                                                       path_constraints);
    return true;
  }
	
private:
		
	
  ros::NodeHandle root_handle_, private_handle_;
  planning_environment::CollisionModelsInterface *collision_models_interface_;

  ros::ServiceServer get_trajectory_validity_service_;
  ros::ServiceServer get_state_validity_service_;
  
};    
}

int main(int argc, char** argv)
{
  ros::init(argc, argv, "planning_scene_validity_server");
  //figuring out whether robot_description has been remapped

  ros::AsyncSpinner spinner(1); 
  spinner.start();
  planning_environment::PlanningSceneValidityServer validity_server;
  ros::waitForShutdown();
  return 0;
}
