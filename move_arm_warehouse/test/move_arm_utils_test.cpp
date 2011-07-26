#include <move_arm_warehouse/move_arm_utils.h>
#include <gtest/gtest.h>
static const std::string VIS_TOPIC_NAME = "planning_scene_visualizer_markers";

//in 100 hz ticks
static const unsigned int CONTROL_SPEED = 5;

static const double BASE_TRANS_SPEED = .3;
static const double BASE_ROT_SPEED = .15;

static const double HAND_TRANS_SPEED = .05;
static const double HAND_ROT_SPEED = .15;

static const std::string EXECUTE_RIGHT_TRAJECTORY = "/r_arm_controller/follow_joint_trajectory";
static const std::string EXECUTE_LEFT_TRAJECTORY = "/l_arm_controller/follow_joint_trajectory";
static const std::string LEFT_IK_NAME = "/pr2_left_arm_kinematics/get_constraint_aware_ik";
static const std::string RIGHT_IK_NAME = "/pr2_right_arm_kinematics/get_constraint_aware_ik";

static const std::string NON_COLL_LEFT_IK_NAME = "/pr2_left_arm_kinematics/get_ik";
static const std::string NON_COLL_RIGHT_IK_NAME = "/pr2_right_arm_kinematics/get_ik";

static const std::string RIGHT_ARM_GROUP = "right_arm";
static const std::string LEFT_ARM_GROUP = "left_arm";

static const std::string RIGHT_ARM_REDUNDANCY = "r_upper_arm_roll_joint";
static const std::string LEFT_ARM_REDUNDANCY = "l_upper_arm_roll_joint";

static const std::string LEFT_IK_LINK = "l_wrist_roll_link";
static const std::string RIGHT_IK_LINK = "r_wrist_roll_link";

static const std::string PLANNER_SERVICE_NAME = "/ompl_planning/plan_kinematic_path";
static const std::string LEFT_INTERPOLATE_SERVICE_NAME = "/l_interpolated_ik_motion_plan";
static const std::string RIGHT_INTERPOLATE_SERVICE_NAME = "/r_interpolated_ik_motion_plan";
static const std::string TRAJECTORY_FILTER_SERVICE_NAME = "/trajectory_filter/filter_trajectory_with_constraints";
static const std::string PROXIMITY_SPACE_SERVICE_NAME = "/collision_proximity_server_test/get_distance_aware_plan";
static const std::string PROXIMITY_SPACE_VALIDITY_NAME = "/collision_proximity_server_test/get_state_validity";
static const std::string PROXIMITY_SPACE_PLANNER_NAME = "/collision_proximity_planner/plan";
static const std::string SET_PLANNING_SCENE_DIFF_NAME = "environment_server/set_planning_scene_diff";

using namespace collision_space;
using namespace kinematics_msgs;
using namespace arm_navigation_msgs;
using namespace head_monitor_msgs;
using namespace move_arm_warehouse;
using namespace planning_environment;
using namespace planning_models;
using namespace std;
using namespace trajectory_msgs;
using namespace planning_scene_utils;
using namespace ros::param;

PlanningSceneEditor* editor = NULL;
planning_scene_utils::PlanningSceneParameters params;

bool killThread = false;
bool inited = false;

/////
/// @brief Creates the editor and connects to the database.
/////
TEST(TestSuite, connectTest)
{
  editor = new PlanningSceneEditor(params);
}

/////
/// @brief Creates 10 new planning scenes
/////
TEST(TestSuite, createPlanningSceneTest)
{
  for(int i = 0; i < 10; i++)
  {
    std::string name = editor->createNewPlanningScene();
    inited = true;
    ROS_INFO("Created scene %s", name.c_str());
  }
}


/////
/// @brief Creates and saves a planning scene.
/////
TEST(TestSuite, createSaveTest)
{
  std::string name = editor->createNewPlanningScene();
  ROS_INFO("Created scene %s", name.c_str());
  ROS_INFO("Saving...");
  editor->savePlanningScene((*(editor->planning_scene_map_))[name]);
}

/////
/// @brief Creates and deletes 100 motion plan requests.
/////
TEST(TestSuite, createDeleteMotionPlanTest)
{
  ROS_INFO("Creating planning scene ...");
  std::string planningSceneID = editor->createNewPlanningScene();
  ROS_INFO("Created planning scene %s", planningSceneID.c_str());

  for(int i = 0; i < 100; i++)
  {
    std::string ID;
    std::string rightarm = RIGHT_ARM_GROUP;
    std::string rightik = RIGHT_IK_LINK;

    ROS_INFO("Creating motion plan request ...");
    editor->createMotionPlanRequest(*(editor->getRobotState()),
                                    *(editor->getRobotState()),
                                    rightarm, rightik,false,
                                    planningSceneID, ID,false);
    ROS_INFO("Deleting motion plan request %s", ID.c_str());
    editor->deleteMotionPlanRequest(ID);

  }
}

/////
/// @brief Plans a trajectory from the start state to 100 random positions.
/////
TEST(TestSuite, planTrajectoryTest)
{
  ROS_INFO("Creating planning scene ...");
  std::string planningSceneID = editor->createNewPlanningScene();
  ROS_INFO("Created planning scene %s", planningSceneID.c_str());

  std::string ID;
  std::string rightarm = RIGHT_ARM_GROUP;
  std::string rightik = RIGHT_IK_LINK;

  ROS_INFO("Creating motion plan request ...");
  editor->createMotionPlanRequest(*(editor->getRobotState()),
                                  *(editor->getRobotState()),
                                  rightarm, rightik,false,
                                  planningSceneID, ID,false);


  MotionPlanRequestData& data = (*(editor->motion_plan_map_))[ID];
  for(int i = 0; i < 100; i++)
  {
    ROS_INFO("Randomly Perturbing");
    editor->randomlyPerturb(data, GoalPosition);
    std::string trajID;
    ROS_INFO("Planning...");
    editor->planToRequest(ID, trajID);
    ROS_INFO("Got plan %s ", trajID.c_str());
    editor->playTrajectory((*(editor->motion_plan_map_))[ID],(*(editor->trajectory_map_))[trajID]);
    ROS_INFO("Deleting...");
    editor->deleteTrajectory(trajID);
  }
  editor->deleteMotionPlanRequest(ID);

}

//////
/// @brief Creates and filters 10 random trajectories.
//////
TEST(TestSuite, filterTrajectoryTest)
{
  ROS_INFO("Creating planning scene ...");
  std::string planningSceneID = editor->createNewPlanningScene();
  ROS_INFO("Created planning scene %s", planningSceneID.c_str());

  std::string ID;
  std::string rightarm = RIGHT_ARM_GROUP;
  std::string rightik = RIGHT_IK_LINK;

  ROS_INFO("Creating motion plan request ...");
  editor->createMotionPlanRequest(*(editor->getRobotState()),
                                  *(editor->getRobotState()),
                                  rightarm, rightik,false,
                                  planningSceneID, ID,false);


  MotionPlanRequestData& data = (*(editor->motion_plan_map_))[ID];
  for(int i = 0; i < 20; i++)
  {
    ROS_INFO("Randomly Perturbing");
    editor->randomlyPerturb(data, GoalPosition);
    std::string trajID;
    ROS_INFO("Planning...");
    editor->planToRequest(ID, trajID);
    ROS_INFO("Got plan %s ", trajID.c_str());
    editor->playTrajectory((*(editor->motion_plan_map_))[ID],(*(editor->trajectory_map_))[trajID]);
    std::string filterID;
    editor->filterTrajectory((*(editor->motion_plan_map_))[ID],(*(editor->trajectory_map_))[trajID], filterID);
    editor->playTrajectory((*(editor->motion_plan_map_))[ID],(*(editor->trajectory_map_))[filterID]);
    ROS_INFO("Deleting...");
    editor->deleteTrajectory(trajID);
    editor->deleteTrajectory(filterID);
  }
}

float randFloat(float minValue, float maxValue)
{
  return ((float)random() / (float)RAND_MAX) * (maxValue - minValue) + minValue;
}

TEST(TestSuite, randomGeneratorTest)
{
  for(int i = 0; i < 100; i++)
  {
    ASSERT_TRUE(randFloat(-100, 100) >= -100);
    ASSERT_TRUE(randFloat(-100, 100) <= 100);
  }
  ASSERT_FLOAT_EQ(5, randFloat(5, 5));
}

/////
/// @brief Creates 100 random objects, then deletes them all.
/////
TEST(TestSuite, collisionObjectTest)
{
  geometry_msgs::Pose pose;
  PlanningSceneEditor::GeneratedShape shape;
  float scaleX;
  float scaleY;
  float scaleZ;

  std::vector<std::string> objs;
  for(int i = 0; i < 100; i++)
  {
    pose.position.x = randFloat(-1.5f, 1.5f);
    pose.position.y = randFloat(-1.5f, 1.5f);
    pose.position.z = randFloat(-1.5f, 1.5f);
    pose.orientation.x = randFloat(0.0f, 1.0f);
    pose.orientation.y = randFloat(0.0f, 1.0f);
    pose.orientation.z = randFloat(0.0f, 1.0f);
    pose.orientation.w = 1.0f;

    float rand = randFloat(0.0f, 1.0f);
    if(rand < 0.333f)
    {
      shape = PlanningSceneEditor::Box;
    }
    else if(rand < 0.666f)
    {
      shape = PlanningSceneEditor::Cylinder;
    }
    else
    {
      shape = PlanningSceneEditor::Sphere;
    }

    scaleX = randFloat(0.01f, 0.2f);
    scaleY = randFloat(0.01f, 0.2f);
    scaleZ = randFloat(0.01f, 0.2f);

    ROS_INFO("Creating object with size %f %f %f and position %f %f %f",scaleX, scaleY, scaleZ, pose.position.x, pose.position.y, pose.position.z);
    std::string obj = editor->createCollisionObject(pose, shape, scaleX, scaleY, scaleZ);
    objs.push_back(obj);
  }

  for(size_t i = 0; i < objs.size(); i ++)
  {
    ROS_INFO("Deleting...");
    editor->deleteCollisionObject(objs[i]);
  }
}

/////
/// @brief Tests 1000 random operations
/////
TEST(TestSuite, stressTest)
{
  ROS_WARN("*************STARTING STRESS TEST**********");
  std::string currentPlanningScene = editor->createNewPlanningScene();
  std::string lastMotionPlan = "";
  std::string lastTrajectory = "";
  std::string lastCollisionObject = "";

  for(int i = 0; i < 1000; i++)
  {
    float randOperation = randFloat(0.0, 1.0f);

    if(randOperation < 0.05)
    {
      ROS_INFO("\t\tOperation %d:\tCreate New Scene", i);
      currentPlanningScene = editor->createNewPlanningScene();
    }
    else if(randOperation < 0.2)
    {
      ROS_INFO("\t\tOperation %d:\tCreate New MPR", i);
      std::string rightarm = RIGHT_ARM_GROUP;
      std::string rightik = RIGHT_IK_LINK;
      editor->createMotionPlanRequest(*(editor->getRobotState()),
                                      *(editor->getRobotState()),
                                      rightarm, rightik,false,
                                      currentPlanningScene, lastMotionPlan,false);
      editor->randomlyPerturb((*(editor->motion_plan_map_))[lastMotionPlan], GoalPosition);
    }
    else if(randOperation < 0.3)
    {
      if(lastMotionPlan != "")
      {
        ROS_INFO("\t\tOperation %d:\tDelete MPR", i);
        editor->deleteMotionPlanRequest(lastMotionPlan);
        lastMotionPlan = "";
        lastTrajectory = "";
      }
    }
    else if(randOperation < 0.4)
    {

      ROS_INFO("\t\tOperation %d:\tCreate Object", i);
      geometry_msgs::Pose pose;
      PlanningSceneEditor::GeneratedShape shape;
      float scaleX;
      float scaleY;
      float scaleZ;
      pose.position.x = randFloat(-1.5f, 1.5f);
      pose.position.y = randFloat(-1.5f, 1.5f);
      pose.position.z = randFloat(-1.5f, 1.5f);
      pose.orientation.x = randFloat(0.0f, 1.0f);
      pose.orientation.y = randFloat(0.0f, 1.0f);
      pose.orientation.z = randFloat(0.0f, 1.0f);
      pose.orientation.w = 1.0f;

      float rand = randFloat(0.0f, 1.0f);
      if(rand < 0.333f)
      {
        shape = PlanningSceneEditor::Box;
      }
      else if(rand < 0.666f)
      {
        shape = PlanningSceneEditor::Cylinder;
      }
      else
      {
        shape = PlanningSceneEditor::Sphere;
      }

      scaleX = randFloat(0.01f, 0.2f);
      scaleY = randFloat(0.01f, 0.2f);
      scaleZ = randFloat(0.01f, 0.2f);

      lastCollisionObject = editor->createCollisionObject(pose, shape, scaleX, scaleY, scaleZ);
    }
    else if(randOperation < 0.5)
    {
      if(lastCollisionObject != "")
      {
        ROS_INFO("\t\tOperation %d:\tDelete Object", i);
        editor->deleteCollisionObject(lastCollisionObject);
        lastCollisionObject = "";
      }
    }
    else if(randOperation < 0.6)
    {
      if(lastMotionPlan != "")
      {
        ROS_INFO("\t\tOperation %d:\tPlan Trajectory", i);
        editor->planToRequest(lastMotionPlan, lastTrajectory);
        editor->playTrajectory((*(editor->motion_plan_map_))[lastMotionPlan],(*(editor->trajectory_map_))[lastTrajectory]);
      }
    }
    else if(randOperation < 0.7)
    {
      if(lastTrajectory != "")
      {
        ROS_INFO("\t\tOperation %d:\tDelete Trajectory", i);
        editor->deleteTrajectory(lastTrajectory);
        lastTrajectory = "";
      }
    }
    else if(randOperation < 0.8)
    {
      if(lastTrajectory != "" && lastMotionPlan != "")
      {
        ROS_INFO("\t\tOperation %d:\tFilter Trajectory", i);
        editor->filterTrajectory((*(editor->motion_plan_map_))[lastMotionPlan],(*(editor->trajectory_map_))[lastTrajectory],lastTrajectory);
        editor->playTrajectory((*(editor->motion_plan_map_))[lastMotionPlan],(*(editor->trajectory_map_))[lastTrajectory]);
        lastTrajectory = "";
      }
    }
    else
    {
      if(lastMotionPlan != "")
      {
        ROS_INFO("\t\tOperation %d:\tRandom Start Pos", i);
        editor->randomlyPerturb((*(editor->motion_plan_map_))[lastMotionPlan], StartPosition);
      }
    }
  }
}



void spinThread()
{
  while(!inited)
  {
    usleep(10000);
  }
  while(!killThread && ros::ok())
  {
    ros::spinOnce();
    editor->sendMarkers();
    usleep(10000);
  }
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  ros::init(argc, argv, "planning_scene_warehouse_viewer", ros::init_options::NoSigintHandler);


  param<string>("set_planning_scene_diff_name", params.set_planning_scene_diff_name_, SET_PLANNING_SCENE_DIFF_NAME);
  param<string>("left_ik_name", params.left_ik_name_, LEFT_IK_NAME);
  param<string>("left_interpolate_service_name", params.left_interpolate_service_name_, LEFT_INTERPOLATE_SERVICE_NAME);
  param<string>("non_coll_left_ik_name", params.non_coll_left_ik_name_, NON_COLL_LEFT_IK_NAME);
  param<string>("non_coll_right_ik_name", params.non_coll_right_ik_name_, NON_COLL_RIGHT_IK_NAME);
  param<string>("planner_service_name", params.planner_service_name_, PLANNER_SERVICE_NAME);
  param<string>("proximity_space_planner_name", params.proximity_space_planner_name_, PROXIMITY_SPACE_PLANNER_NAME);
  param<string>("proximity_space_service_name",  params.proximity_space_service_name_, PROXIMITY_SPACE_SERVICE_NAME);
  param<string>("proximity_space_validity_name",  params.proximity_space_validity_name_,  PROXIMITY_SPACE_VALIDITY_NAME);
  param<string>("right_ik_name", params.right_ik_name_, RIGHT_IK_NAME);
  param<string>("right_interpolate_service_name", params.right_interpolate_service_name_, RIGHT_INTERPOLATE_SERVICE_NAME);
  param<string>("trajectory_filter_service_name", params.trajectory_filter_service_name_, TRAJECTORY_FILTER_SERVICE_NAME);
  param<string>("vis_topic_name", params.vis_topic_name_ , VIS_TOPIC_NAME);
  param<string>("right_ik_link", params.right_ik_link_ , RIGHT_IK_LINK);
  param<string>("left_ik_link", params.left_ik_link_ , LEFT_IK_LINK);
  param<string>("right_arm_group", params.right_arm_group_ , RIGHT_ARM_GROUP);
  param<string>("left_arm_group", params.left_arm_group_ , LEFT_ARM_GROUP);
  param<string>("right_redundancy", params.right_redundancy_ , RIGHT_ARM_REDUNDANCY);
  param<string>("left_redundancy", params.left_redundancy_ , LEFT_ARM_REDUNDANCY);
  param<string>("execute_left_trajectory", params.execute_left_trajectory_ , EXECUTE_LEFT_TRAJECTORY);
  param<string>("execute_right_trajectory", params.execute_right_trajectory_ , EXECUTE_RIGHT_TRAJECTORY);


  boost::thread spin_thread(boost::bind(&spinThread));

  int ret = RUN_ALL_TESTS();

  killThread = true;

  spin_thread.join();


  return ret;
}
