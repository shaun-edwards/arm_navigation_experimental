/*********************************************************************
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
*********************************************************************/

/** \author E. Gil Jones */

#ifndef PLANNING_ENVIRONMENT_MODELS_COLLISION_MODELS_INTERFACE_
#define PLANNING_ENVIRONMENT_MODELS_COLLISION_MODELS_INTERFACE_

#include "planning_environment/models/collision_models.h"
#include <planning_environment_msgs/SetPlanningSceneAction.h>
#include <actionlib/server/simple_action_server.h>

namespace planning_environment
{

/** \brief A class capable of loading a robot model from the parameter server */
    
class CollisionModelsInterface : public CollisionModels
{
public:

  //
  // Constructors
  //
	
  CollisionModelsInterface(const std::string &description);

  virtual ~CollisionModelsInterface(void);
 
  void setPlanningSceneCallback(const planning_environment_msgs::SetPlanningSceneGoalConstPtr& scene);
  
  void addSetPlanningSceneCallback(const boost::function<void(const planning_environment_msgs::PlanningScene &scene)> &callback)
  {
    set_planning_scene_callback_ = callback;
  }

  void addRevertPlanningSceneCallback(const boost::function<void(void)> &callback) 
  {
    revert_planning_scene_callback_ = callback;
  }

  planning_models::KinematicState* getPlanningSceneState() const{
    return planning_scene_state_;
  }

  void resetToStartState(planning_models::KinematicState& state) const;

  collision_space::EnvironmentModel* getOde() {
    return ode_collision_model_;
  }
  
protected:

  planning_models::KinematicState* planning_scene_state_;
  planning_environment_msgs::PlanningScene last_planning_scene_;

  boost::function<void(const planning_environment_msgs::PlanningScene &scene)> set_planning_scene_callback_;
  boost::function<void(void)> revert_planning_scene_callback_;
  ros::ServiceClient env_server_register_client_;

  actionlib::SimpleActionServer<planning_environment_msgs::SetPlanningSceneAction> *action_server_;
};
    
	
}

#endif

