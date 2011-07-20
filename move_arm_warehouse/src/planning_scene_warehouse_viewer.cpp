/*
 * Copyright (c) 2011, Willow Garage, Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <ORGANIZATION> nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// Author: E. Gil Jones

#include <move_arm_warehouse/planning_scene_warehouse_viewer.h>
#include <qt4/QtGui/qapplication.h>

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
PlanningSceneVisualizer* psv = NULL;
bool inited = false;

PlanningSceneVisualizer::PlanningSceneVisualizer(QWidget* parent, planning_scene_utils::PlanningSceneParameters& params) :
  QWidget(parent), PlanningSceneEditor(params)
{
  quit_threads_ = false;
  initQtWidgets();
  setCurrentPlanningScene("", false, false);
  selected_trajectory_ID_ = "";
}

PlanningSceneVisualizer::~PlanningSceneVisualizer()
{
  //TODO: Implement
}

void PlanningSceneVisualizer::initQtWidgets()
{
  menu_bar_ = new QMenuBar(this);

  QGroupBox* motionPlanBox = new QGroupBox(this);
  motionPlanBox->setTitle("Motion Plan Requests");

  selected_request_label_ = new QLabel(motionPlanBox);
  selected_request_label_->setText("Selected Request: None");

  QGroupBox* trajectoryBox = new QGroupBox(this);
  trajectoryBox->setTitle("Trajectories");


  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->addWidget(motionPlanBox);
  layout->addWidget(trajectoryBox);
  QVBoxLayout* motionBoxLayout = new QVBoxLayout(motionPlanBox);
  motion_plan_tree_ = new QTreeWidget(motionPlanBox);
  motion_plan_tree_->setColumnCount(5);

  QVBoxLayout* trajectoryBoxLayout = new QVBoxLayout(trajectoryBox);
  file_menu_ = menu_bar_->addMenu("File");
  new_planning_scene_action_ = file_menu_->addAction("New Planning Scene ...");
  load_planning_scene_action_ = file_menu_->addAction("Load Planning Scene ...");
  save_planning_scene_action_ = file_menu_->addAction("Save Planning Scene ...");
  new_motion_plan_action_ = file_menu_->addAction("New Motion Plan Request ...");
  quit_action_ = file_menu_->addAction("Quit");

  collision_object_menu_ = menu_bar_->addMenu("Collision Objects");
  new_object_action_ = collision_object_menu_->addAction("New Collision Object ...");

  trajectory_tree_ = new QTreeWidget(trajectoryBox);
  trajectory_tree_->setColumnCount(8);


  QLabel* trajectory_label = new QLabel(trajectoryBox);
  trajectory_label->setText("Trajectory Position");

  QGroupBox* buttonsBox = new QGroupBox(trajectoryBox);
  QHBoxLayout* buttonLayout = new QHBoxLayout(buttonsBox);

  trajectory_slider_ = new QSlider(Qt::Horizontal,trajectoryBox);

  QLabel* modeLabel = new QLabel(trajectoryBox);
  modeLabel->setText("Selected Trajectory: ");

  play_button_ = new QPushButton(this);
  play_button_->setText("Play Trajectory");
  play_button_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  filter_button_ = new QPushButton(this);
  filter_button_->setText("Filter Trajectory");
  filter_button_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  if(params_.trajectory_filter_service_name_ == "none")
  {
    filter_button_->setEnabled(false);
  }

  replan_button_ = new QPushButton(this);
  replan_button_->setText("Plan New Trajectory");
  replan_button_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  if(params_.planner_service_name_ == "none")
  {
    replan_button_->setEnabled(false);
  }

  execute_button_ = new QPushButton(this);
  execute_button_->setText("Execute On Robot");
  execute_button_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  execute_button_->setEnabled(params_.use_robot_data_);

  trajectory_point_edit_ = new QSpinBox(this);
  trajectory_point_edit_->setRange(0, 100);
  trajectory_point_edit_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  selected_trajectory_label_ = new QLabel(this);
  selected_trajectory_label_->setText("None");

  QPushButton* deleteMPRButton = new QPushButton(motionPlanBox);
  deleteMPRButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  deleteMPRButton->setText("Delete Motion Plan Request");

  QPushButton* deleteTrajectoryButton = new QPushButton(trajectoryBox);
  deleteTrajectoryButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  deleteTrajectoryButton->setText("Delete Trajectory");

  motionBoxLayout->addWidget(menu_bar_);
  motionBoxLayout->addWidget(motion_plan_tree_);
  motionBoxLayout->addWidget(selected_request_label_);
  motionBoxLayout->addWidget(deleteMPRButton);
  trajectoryBoxLayout->addWidget(trajectory_tree_);
  trajectoryBoxLayout->addWidget(modeLabel);
  trajectoryBoxLayout->addWidget(selected_trajectory_label_);
  trajectoryBoxLayout->addWidget(trajectory_label);
  trajectoryBoxLayout->addWidget(trajectory_point_edit_);
  trajectoryBoxLayout->addWidget(trajectory_slider_);
  trajectoryBoxLayout->addWidget(deleteTrajectoryButton);
  trajectoryBoxLayout->addWidget(buttonsBox);
  buttonLayout->addWidget(play_button_);
  buttonLayout->addWidget(replan_button_);
  buttonLayout->addWidget(filter_button_);
  buttonLayout->addWidget(execute_button_);
  buttonsBox->setTitle("Trajectory Controls");

  connect(deleteMPRButton, SIGNAL(clicked()), this, SLOT(deleteSelectedMotionPlan()));
  connect(deleteTrajectoryButton, SIGNAL(clicked()), this, SLOT(deleteSelectedTrajectory()));
  connect(new_planning_scene_action_, SIGNAL(triggered()), this, SLOT(createNewPlanningScenePressed()));
  connect(new_motion_plan_action_, SIGNAL(triggered()), this, SLOT(createNewMotionPlanPressed()));
  connect(new_object_action_, SIGNAL(triggered()), this, SLOT(createNewObjectPressed()));
  connect(save_planning_scene_action_, SIGNAL(triggered()), this, SLOT(saveCurrentPlanningScene()));
  connect(load_planning_scene_action_, SIGNAL(triggered()), this, SLOT(popupLoadPlanningScenes()));
  connect(quit_action_, SIGNAL(triggered()), this, SLOT(quit()));
  connect(play_button_, SIGNAL(clicked()), this, SLOT(playButtonPressed()));
  connect(filter_button_, SIGNAL(clicked()), this, SLOT(filterButtonPressed()));
  connect(trajectory_slider_, SIGNAL(sliderMoved(int)), this, SLOT(sliderDragged()));
  connect(trajectory_tree_, SIGNAL(itemSelectionChanged()), this, SLOT(trajectoryTableSelection()));
  connect(replan_button_, SIGNAL(clicked()), this, SLOT(replanButtonPressed()));
  connect(trajectory_point_edit_, SIGNAL(valueChanged(int)), this, SLOT(trajectoryEditChanged()));
  connect(motion_plan_tree_, SIGNAL(itemSelectionChanged()), this, SLOT(motionPlanTableSelection()));
  connect(this, SIGNAL(updateTables()), this, SLOT(updateStateTriggered()));
  connect(execute_button_, SIGNAL(clicked()), this, SLOT(executeButtonPressed()));
  load_planning_scene_dialog_ = new QDialog(this);

  setupPlanningSceneDialog();
  connect(this, SIGNAL(changeProgress(int)), load_scene_progress_, SLOT(setValue(int)));
  menu_bar_->setMinimumWidth(500);
  buttonsBox->setLayout(buttonLayout);
  trajectoryBox->setLayout(trajectoryBoxLayout);
  motionPlanBox->setLayout(motionBoxLayout);
  this->setLayout(layout);

  createNewObjectDialog();
  createRequestDialog();

  setCurrentPlanningScene(createNewPlanningScene());
}

void PlanningSceneVisualizer::executeButtonPressed()
{
  if(selected_trajectory_ID_ != "")
  {
    executeTrajectory(selected_trajectory_ID_);
  }
  else
  {
    QMessageBox msg(QMessageBox::Warning, "Execute Trajectory", "No Trajectory Selected!");
    msg.addButton("Ok", QMessageBox::AcceptRole);
    msg.exec();
  }
}

void PlanningSceneVisualizer::deleteSelectedMotionPlan()
{
  if(selected_motion_plan_ID_ != "")
  {
    lockScene();
    deleteMotionPlanRequest(selected_motion_plan_ID_);
    selected_motion_plan_ID_ = "";
    emit updateStateTriggered();
    unlockScene();
  }
  else
  {
    QMessageBox msg(QMessageBox::Warning, "Delete Motion Plan Request", "No Motion Plan Request Selected!");
    msg.addButton("Ok", QMessageBox::AcceptRole);
    msg.exec();
  }
}

void PlanningSceneVisualizer::deleteSelectedTrajectory()
{
  if(selected_trajectory_ID_ != "")
  {
    lockScene();
    deleteTrajectory(selected_trajectory_ID_);
    selected_trajectory_ID_ = "";
    emit updateStateTriggered();
    unlockScene();
  }
  else
  {
    QMessageBox msg(QMessageBox::Warning, "Delete Trajectory", "No Trajectory Selected!");
    msg.addButton("Ok", QMessageBox::AcceptRole);
    msg.exec();
  }
}

void PlanningSceneVisualizer::createNewMotionPlanPressed()
{
  new_request_dialog_->open();
}

void PlanningSceneVisualizer::trajectoryEditChanged()
{
  if(selected_trajectory_ID_ != "")
  {
    TrajectoryData& trajectory = (*trajectory_map_)[selected_trajectory_ID_];
    trajectory.setCurrentPoint(trajectory_point_edit_->value());
  }
}

void PlanningSceneVisualizer::setupPlanningSceneDialog()
{
  planning_scene_table_ = new QTableWidget(load_planning_scene_dialog_);
  QVBoxLayout* layout = new QVBoxLayout(load_planning_scene_dialog_);
  layout->addWidget(planning_scene_table_);
  load_scene_progress_ = new QProgressBar(load_planning_scene_dialog_);
  layout->addWidget(load_scene_progress_);
  load_planning_scene_button_ = new QPushButton(load_planning_scene_dialog_);
  load_planning_scene_button_->setText("Load...");
  load_planning_scene_button_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  refresh_planning_scene_button_ = new QPushButton(load_planning_scene_dialog_);
  refresh_planning_scene_button_->setText("Refresh...");
  refresh_planning_scene_button_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  connect(load_planning_scene_button_, SIGNAL(clicked()), this, SLOT(loadButtonPressed()));
  connect(refresh_planning_scene_button_, SIGNAL(clicked()), this, SLOT(refreshButtonPressed()));


  load_motion_plan_requests_box_ = new QCheckBox(load_planning_scene_dialog_);
  load_motion_plan_requests_box_->setChecked(true);
  load_motion_plan_requests_box_->setText("Load Motion Plan Requests");
  load_trajectories_box_ = new QCheckBox(load_planning_scene_dialog_);
  load_trajectories_box_->setChecked(true);
  load_trajectories_box_->setText("Load Trajectories");

  layout->addWidget(load_motion_plan_requests_box_);
  layout->addWidget(load_trajectories_box_);
  layout->addWidget(load_planning_scene_button_);
  layout->addWidget(refresh_planning_scene_button_);

  load_planning_scene_dialog_->setLayout(layout);
  table_load_thread_ = new TableLoadThread(this);
  table_load_thread_->start();
}

void PlanningSceneVisualizer::quit()
{
  QMessageBox msgBox(QMessageBox::Warning, "Quit?", "Are you sure you want to quit? Unsaved changes will be lost.");
  QPushButton *quitButton = msgBox.addButton("Quit", QMessageBox::ActionRole);
  QPushButton *saveQuitButton = msgBox.addButton("Save And Quit", QMessageBox::ActionRole);
  msgBox.addButton(QMessageBox::Cancel);

  msgBox.exec();

  if (msgBox.clickedButton() == quitButton)
  {
    quit_threads_ = true;
    delete this;
    return;
  }
  else if (msgBox.clickedButton() == saveQuitButton)
  {
    saveCurrentPlanningScene();
    quit_threads_ = true;
    delete this;
    return;
  }
  else
  {
    return;
  }
}

void PlanningSceneVisualizer::motionPlanTableSelection()
{
  QList<QTreeWidgetItem*> selected = motion_plan_tree_->selectedItems();

  if(selected.size() > 0)
  {
    const QString& name = selected[0]->toolTip(0);
    selectMotionPlan(name.toStdString());
  }
}

void PlanningSceneVisualizer::selectMotionPlan(std::string ID)
{
  selected_motion_plan_ID_ = ID;
  selected_trajectory_ID_ = "";
  createTrajectoryTable();
  selected_request_label_->setText(QString::fromStdString("Selected Request: "+selected_motion_plan_ID_ ));


  if((*motion_plan_map_)[selected_motion_plan_ID_].getTrajectories().size() > 0)
  {
    selectTrajectory((*motion_plan_map_)[selected_motion_plan_ID_].getTrajectories()[0]);
  }
}

void PlanningSceneVisualizer::createMotionPlanTable()
{
  if(current_planning_scene_ID_ != "")
  {
    PlanningSceneData& planningSceneData = (*planning_scene_map_)[current_planning_scene_ID_];

    motion_plan_tree_->clear();
    motion_plan_tree_->setColumnCount(4);
    motion_plan_tree_->setColumnWidth(0, 150);

    for(size_t i = 0; i < planningSceneData.getRequests().size(); i++)
    {
      std::string request = planningSceneData.getRequests()[i];
      MotionPlanRequestData& requestData = (*motion_plan_map_)[request];


      QTreeWidgetItem* nameItem = new QTreeWidgetItem(QStringList(QString::fromStdString(requestData.getID())));
      nameItem->setToolTip(0,nameItem->text(0));
      nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      motion_plan_tree_->insertTopLevelItem(i, nameItem);

      QStringList sourceList;
      sourceList.append("Source");
      sourceList.append(QString::fromStdString(requestData.getSource()));
      QTreeWidgetItem* sourceItem = new QTreeWidgetItem(sourceList);
      sourceItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      sourceItem->setToolTip(0, nameItem->text(0));
      sourceItem->setToolTip(1, nameItem->text(0));
      nameItem->insertChild(0, sourceItem);

      QStringList collisionList;
      collisionList.append("Collisions");
      QTreeWidgetItem* collisionItem = new QTreeWidgetItem(collisionList);
      collisionItem->setToolTip(0, nameItem->text(0));
      QCheckBox* collisionsVisible = new QCheckBox(motion_plan_tree_);
      collisionsVisible->setText("Show Collisions");
      collisionsVisible->setToolTip(nameItem->text(0));
      nameItem->insertChild(1, collisionItem);
      motion_plan_tree_->setItemWidget(collisionItem, 1, collisionsVisible);
      collisionsVisible->setChecked(requestData.areCollisionsVisible());

      connect(collisionsVisible, SIGNAL(clicked(bool)), this, SLOT(motionPlanCollisionVisibleButtonClicked(bool)));

      QStringList startList;
      startList.append("Start Position");
      startList.append("");
      startList.append("Color");
      startList.append("");
      QTreeWidgetItem* startItem = new QTreeWidgetItem(startList);
      startItem->setToolTip(0, nameItem->text(0));
      QCheckBox* startVisible = new QCheckBox(motion_plan_tree_);
      startVisible->setText("Visible");
      startVisible->setToolTip(nameItem->text(0));
      nameItem->insertChild(2, startItem);
      motion_plan_tree_->setItemWidget(startItem, 1, startVisible);
      startVisible->setChecked(requestData.isStartVisible());

      connect(startVisible, SIGNAL(clicked(bool)), this, SLOT(motionPlanStartVisibleButtonClicked(bool)));

      QPushButton* startColorButton = new QPushButton(motion_plan_tree_);
      std::stringstream startColorStream;
      startColorStream<< "(" << (int)(requestData.getStartColor().r*255) <<" , ";
      startColorStream << (int)(requestData.getStartColor().g*255) << " , ";
      startColorStream << (int)(requestData.getStartColor().b*255) << ")";
      startColorButton->setText(QString::fromStdString(startColorStream.str()));
      startColorButton->setToolTip(nameItem->text(0));
      startColorButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
      connect(startColorButton, SIGNAL(clicked()), this, SLOT(motionPlanStartColorButtonClicked()));
      motion_plan_tree_->setItemWidget(startItem, 3, startColorButton);


      QStringList endList;
      endList.append("End Position");
      endList.append("");
      endList.append("Color");
      endList.append("");
      QTreeWidgetItem* endItem = new QTreeWidgetItem(endList);
      endItem->setToolTip(0, nameItem->text(0));
      QCheckBox* endVisible = new QCheckBox(motion_plan_tree_);
      endVisible->setText("Visible");
      endVisible->setToolTip(nameItem->text(0));
      nameItem->insertChild(3, endItem);
      motion_plan_tree_->setItemWidget(endItem, 1, endVisible);
      endVisible->setChecked(requestData.isEndVisible());

      connect(endVisible, SIGNAL(clicked(bool)), this, SLOT(motionPlanEndVisibleButtonClicked(bool)));

      QPushButton* endColorButton = new QPushButton(motion_plan_tree_);
      std::stringstream endColorStream;
      endColorStream<< "(" << (int)(requestData.getEndColor().r*255) <<" , ";
      endColorStream << (int)(requestData.getEndColor().g*255) << " , ";
      endColorStream << (int)(requestData.getEndColor().b*255) << ")";
      endColorButton->setText(QString::fromStdString(endColorStream.str()));
      endColorButton->setToolTip(nameItem->text(0));
      endColorButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
      connect(endColorButton, SIGNAL(clicked()), this, SLOT(motionPlanEndColorButtonClicked()));
      motion_plan_tree_->setItemWidget(endItem, 3, endColorButton);

      QStringList controlList;
      controlList.append("Joint Control");
      QTreeWidgetItem* controlItem = new QTreeWidgetItem(controlList);
      controlItem->setToolTip(0, nameItem->text(0));
      nameItem->insertChild(4, controlItem);
      QCheckBox* controlsVisible = new QCheckBox(motion_plan_tree_);
      controlsVisible->setText("Joint Control");
      controlsVisible->setToolTip(nameItem->text(0));
      connect(controlsVisible, SIGNAL(clicked(bool)), this, SLOT(motionPlanJointControlsActiveButtonClicked(bool)));
      motion_plan_tree_->setItemWidget(controlItem,1, controlsVisible);

    }
  }
}

void PlanningSceneVisualizer::motionPlanCollisionVisibleButtonClicked(bool checked)
{
  QObject* sender = QObject::sender();
  QCheckBox* button = qobject_cast<QCheckBox*> (sender);

  if(button != NULL)
  {
    std::string mprID = button->toolTip().toStdString();
    MotionPlanRequestData& data = (*motion_plan_map_)[mprID];
    data.setCollisionsVisible(checked);
  }
}

void PlanningSceneVisualizer::motionPlanStartColorButtonClicked()
{
  QObject* sender = QObject::sender();
  QPushButton* button = qobject_cast<QPushButton*>(sender);

  if(button != NULL)
  {
    std::string trajectoryID = button->toolTip().toStdString();
    MotionPlanRequestData& data = (*motion_plan_map_)[trajectoryID];
    QColor col(data.getStartColor().r*255, data.getStartColor().g*255, data.getStartColor().b*255);
    QColor colorSelected = QColorDialog::getColor(col, this);
    std::stringstream colorStream;
    colorStream<< "(" << colorSelected.red()<<" , ";
    colorStream << colorSelected.green()<< " , ";
    colorStream << colorSelected.blue() << ")";

    button->setText(QString::fromStdString(colorStream.str()));

    std_msgs::ColorRGBA trajColor;
    trajColor.r = colorSelected.redF();
    trajColor.g = colorSelected.greenF();
    trajColor.b = colorSelected.blueF();
    trajColor.a = 0.5f;
    data.setStartColor(trajColor);
    data.refreshColors();
  }
}

void PlanningSceneVisualizer::motionPlanEndColorButtonClicked()
{
  QObject* sender = QObject::sender();
  QPushButton* button = qobject_cast<QPushButton*>(sender);

  if(button != NULL)
  {
    std::string trajectoryID = button->toolTip().toStdString();
    MotionPlanRequestData& data = (*motion_plan_map_)[trajectoryID];
    QColor col(data.getEndColor().r*255, data.getEndColor().g*255, data.getEndColor().b*255);
    QColor colorSelected = QColorDialog::getColor(col, this);
    std::stringstream colorStream;
    colorStream<< "(" << colorSelected.red()<<" , ";
    colorStream << colorSelected.green()<< " , ";
    colorStream << colorSelected.blue() << ")";

    button->setText(QString::fromStdString(colorStream.str()));

    std_msgs::ColorRGBA trajColor;
    trajColor.r = colorSelected.redF();
    trajColor.g = colorSelected.greenF();
    trajColor.b = colorSelected.blueF();
    trajColor.a = 0.5f;
    data.setEndColor(trajColor);
    data.refreshColors();

  }
}

void PlanningSceneVisualizer::motionPlanStartVisibleButtonClicked(bool checked)
{
  QObject* sender = QObject::sender();
  QCheckBox* button = qobject_cast<QCheckBox*> (sender);

  if(button != NULL)
  {
    std::string mprID = button->toolTip().toStdString();
    MotionPlanRequestData& data = (*motion_plan_map_)[mprID];
    data.setStartVisible(checked);
    data.setJointControlsVisible(data.areJointControlsVisible(), this);
    setIKControlsVisible(mprID, StartPosition, button->isChecked());
  }


}

void PlanningSceneVisualizer::motionPlanEndVisibleButtonClicked(bool checked)
{
  QObject* sender = QObject::sender();
  QCheckBox* button = qobject_cast<QCheckBox*> (sender);

  if(button != NULL)
  {
    std::string mprID = button->toolTip().toStdString();
    MotionPlanRequestData& data = (*motion_plan_map_)[mprID];
    data.setEndVisible(checked);
    data.setJointControlsVisible(data.areJointControlsVisible(), this);
    setIKControlsVisible(mprID, EndPosition, button->isChecked());
  }
}

void PlanningSceneVisualizer::createNewMotionPlanRequest(std::string group_name, std::string end_effector_name)
{

  if(current_planning_scene_ID_ != "")
  {
    std::string motionPlanID;
    createMotionPlanRequest(*robot_state_, *robot_state_, group_name, end_effector_name, false,
                            current_planning_scene_ID_, motionPlanID, create_request_from_robot_box_->isChecked());
  }
  else
  {
    QMessageBox msg(QMessageBox::Warning, "Create Request", "No Planning Scene Loaded!");
    msg.addButton("Ok", QMessageBox::AcceptRole);
    msg.exec();
  }
}

void PlanningSceneVisualizer::saveCurrentPlanningScene()
{
  savePlanningScene((*planning_scene_map_)[current_planning_scene_ID_]);
  QMessageBox msgBox(QMessageBox::Information, "Saved", "Saved planning scene successfully.");
  msgBox.addButton(QMessageBox::Ok);
  msgBox.exec();
}

void PlanningSceneVisualizer::createNewPlanningScenePressed()
{
  QMessageBox msgBox(QMessageBox::Warning, "Create New Planning Scene", "Are you sure you want to create a new planning scene? Unsaved changes to the current planning scene will be lost.");
  QPushButton *createButton= msgBox.addButton("Create New Without Saving Changes", QMessageBox::ActionRole);
  QPushButton *saveCreateButton = msgBox.addButton("Save Changes Before Creating New", QMessageBox::ActionRole);
  msgBox.addButton(QMessageBox::Cancel);

  msgBox.exec();

  if (msgBox.clickedButton() == saveCreateButton)
  {
    saveCurrentPlanningScene();
    setCurrentPlanningScene(createNewPlanningScene(), true, true);
    motion_plan_tree_->clear();
    trajectory_tree_->clear();
    ROS_INFO("Created a new planning scene: %s", current_planning_scene_ID_.c_str());
  }
  else if (msgBox.clickedButton() == createButton)
  {
    setCurrentPlanningScene(createNewPlanningScene(), true, true);
    motion_plan_tree_->clear();
    trajectory_tree_->clear();
    ROS_INFO("Created a new planning scene: %s", current_planning_scene_ID_.c_str());
  }
  else
  {
    return;
  }


}

void PlanningSceneVisualizer::collisionDisplayChanged(const QString& mode)
{
  if(selected_trajectory_ID_ != "")
  {
    TrajectoryData& trajectory = (*trajectory_map_)[selected_trajectory_ID_];
    std::string collision_state_display_string = mode.toStdString();
    std::string disp = "none";
    if(collision_state_display_string == "Start State")
    {
      disp = "scene";

    }
    else if(collision_state_display_string ==  "Goal State")
    {
      disp = "end_effector";
    }
    else if(collision_state_display_string == "Planner trajectory state")
    {
      disp = "planner";
    }
    else if(collision_state_display_string ==  "Filter trajectory state")
    {
      disp = "filter";
    }
    else if(collision_state_display_string ==  "Monitor trajectory state")
    {
      disp = "monitor";
    }
    else if(collision_state_display_string ==  "Paused collision state")
    {
      disp = "paused";
    }

    else
    {
      disp = "none";
    }

    std::string arm_group_name = trajectory.getGroupName();
    MotionPlanRequestData& motionPlan = (*motion_plan_map_)[trajectory.getMotionPlanRequestID()];
    setShowCollisions(true, disp, arm_group_name, motionPlan);
  }
}

void PlanningSceneVisualizer::updateState()
{
  emit updateTables();
}

void PlanningSceneVisualizer::updateStateTriggered()
{
  lockScene();
  createMotionPlanTable();
  createTrajectoryTable();
  unlockScene();
}

void PlanningSceneVisualizer::popupLoadPlanningScenes()
{
  load_planning_scene_dialog_->show();
}

void PlanningSceneVisualizer::refreshButtonPressed()
{
  if(table_load_thread_ != NULL)
  {
    table_load_thread_->wait();
    delete table_load_thread_;
    table_load_thread_ = NULL;
  }
  table_load_thread_= new TableLoadThread(this);
  table_load_thread_->start();
}

void PlanningSceneVisualizer::loadButtonPressed()
{

  QMessageBox msgBox(QMessageBox::Information, "Load New Planning Scene", "Are you sure you want to load a new planning scene? Unsaved changes to the current planning scene will be lost.");
  QPushButton *createButton= msgBox.addButton("Load New Without Saving Changes", QMessageBox::ActionRole);
  QPushButton *saveCreateButton = msgBox.addButton("Save Changes Before Loading New", QMessageBox::ActionRole);
  msgBox.addButton(QMessageBox::Cancel);

  msgBox.exec();

  if (msgBox.clickedButton() == saveCreateButton)
  {
    saveCurrentPlanningScene();
  }
  else if (msgBox.clickedButton() != createButton)
  {
    return;
  }

  QList<QTableWidgetItem*> items = planning_scene_table_->selectedItems();

  if(items.size() > 0)
  {
    QTableWidgetItem* item = items[0];
    QTableWidgetItem* nameItem = planning_scene_table_->item(item->row(),0);
    PlanningSceneData& data = (*planning_scene_map_)[nameItem->text().toStdString()];
    setCurrentPlanningScene(nameItem->text().toStdString(), load_motion_plan_requests_box_->isChecked(), load_trajectories_box_->isChecked());
    sendPlanningScene(data);
    trajectory_tree_->clear();
    updateJointStates();
    createMotionPlanTable();
    data.getRobotState(robot_state_);
  }

  load_planning_scene_dialog_->close();
}

void PlanningSceneVisualizer::replanButtonPressed()
{
  if(selected_motion_plan_ID_ != "")
  {
    std::string newTraj;
    planToRequest((*motion_plan_map_)[selected_motion_plan_ID_], newTraj);
    createTrajectoryTable();
    selectTrajectory(newTraj);
    playTrajectory((*motion_plan_map_)[selected_motion_plan_ID_], (*trajectory_map_)[newTraj]);
  }
  else
  {
    QMessageBox msg(QMessageBox::Warning, "Plan Trajectory", "No Motion Plan Request Selected!");
    msg.addButton("Ok", QMessageBox::AcceptRole);
    msg.exec();
  }
}

void PlanningSceneVisualizer::trajectoryTableSelection()
{
  QList<QTreeWidgetItem*> selected = trajectory_tree_->selectedItems();

  if(selected.size() > 0)
  {
    QString name = selected[0]->toolTip(0);
    selectTrajectory(name.toStdString());
  }
}

void PlanningSceneVisualizer::selectTrajectory(std::string ID)
{
  selected_trajectory_ID_ = ID;
  TrajectoryData& trajectory = (*trajectory_map_)[selected_trajectory_ID_];
  trajectory_slider_->setMaximum((int)trajectory.getTrajectory().points.size() - 1);
  trajectory_slider_->setMinimum(0);

  trajectory_point_edit_->setMaximum((int)trajectory.getTrajectory().points.size() - 1);
  trajectory_point_edit_->setMinimum(0);

  std::stringstream ss;
  ss << trajectory.trajectory_error_code_.val;
  selected_trajectory_label_->setText(QString::fromStdString(ID + " Error Code : " + armNavigationErrorCodeToString(trajectory.trajectory_error_code_) + " (" + ss.str().c_str()+ ")"));
}

void PlanningSceneVisualizer::playButtonPressed()
{
  if(selected_trajectory_ID_ != "")
  {
    TrajectoryData& trajectory = (*trajectory_map_)[selected_trajectory_ID_];
    playTrajectory((*motion_plan_map_)[trajectory.getMotionPlanRequestID()], trajectory);
    std::stringstream ss;
    ss << trajectory.trajectory_error_code_.val;
    selected_trajectory_label_->setText(QString::fromStdString(selected_trajectory_ID_ + " Error Code : " + armNavigationErrorCodeToString(trajectory.trajectory_error_code_) + " (" + ss.str().c_str()+ ")"));
  }
  else
  {
    QMessageBox msg(QMessageBox::Warning, "Play Trajectory", "No Trajectory Selected!");
    msg.addButton("Ok", QMessageBox::AcceptRole);
    msg.exec();
  }
}

void PlanningSceneVisualizer::filterButtonPressed()
{
  if(selected_trajectory_ID_ != "")
  {
    TrajectoryData& trajectory = (*trajectory_map_)[selected_trajectory_ID_];
    string filterID;
    MotionPlanRequestData& data = (*motion_plan_map_)[trajectory.getMotionPlanRequestID()];
    filterTrajectory(data,trajectory, filterID);
    std::stringstream ss;
    ss << trajectory.trajectory_error_code_.val;
    selected_trajectory_label_->setText(QString::fromStdString(selected_trajectory_ID_ + " Error Code : " + armNavigationErrorCodeToString(trajectory.trajectory_error_code_) + " (" + ss.str().c_str()+ ")"));
    createTrajectoryTable();
  }
  else
  {
    QMessageBox msg(QMessageBox::Warning, "Filter Trajectory", "No Trajectory Selected!");
    msg.addButton("Ok", QMessageBox::AcceptRole);
    msg.exec();
  }
}

void PlanningSceneVisualizer::sliderDragged()
{
  if(selected_trajectory_ID_ != "")
  {
    trajectory_point_edit_->setValue(trajectory_slider_->value());
  }
  else
  {
    QMessageBox msg(QMessageBox::Warning, "Control Trajectory", "No Trajectory Selected!");
    msg.addButton("Ok", QMessageBox::AcceptRole);
    msg.exec();
  }
}

void PlanningSceneVisualizer::createTrajectoryTable()
{
  if(selected_motion_plan_ID_ == "")
  {
    return;
  }

  MotionPlanRequestData& data = (*motion_plan_map_)[selected_motion_plan_ID_];
  trajectory_tree_->clear();
  trajectory_tree_->setColumnCount(2);
  trajectory_tree_->setColumnWidth(0, 200);

  for(size_t i = 0; i < data.getTrajectories().size(); i++)
  {
    TrajectoryData& trajectory = (*trajectory_map_)[data.getTrajectories()[i]];

    QTreeWidgetItem* nameItem = new QTreeWidgetItem(QStringList(QString::fromStdString(trajectory.getID())));
    nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    nameItem->setToolTip(0, nameItem->text(0));
    trajectory_tree_->insertTopLevelItem(i, nameItem);

    QStringList sourceList;
    sourceList.append("Source");
    sourceList.append(QStringList(QString::fromStdString(trajectory.getSource())));
    QTreeWidgetItem* sourceItem = new QTreeWidgetItem(sourceList);
    sourceItem->setToolTip(0, nameItem->text(0));
    sourceItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    nameItem->insertChild(0, sourceItem);

    std::stringstream durationStream;
    durationStream << (float)trajectory.getDuration().toSec() << " seconds";

    QStringList durationList;
    if(trajectory.getSource() == "Planner" || trajectory.getSource() == "planner")
    {
      durationList.append("Planning Time");
    }
    else if (trajectory.getSource() == "Trajectory Filterer" || trajectory.getSource() == "filter")
    {
      durationList.append("Filter Time");
    }
    else if(trajectory.getSource() == "Robot Monitor" || trajectory.getSource() == "monitor")
    {
      durationList.append("Execution Time");
    }
    else
    {
      durationList.append("Duration");
    }
    durationList.append(QString::fromStdString(durationStream.str()));
    QTreeWidgetItem* durationItem = new QTreeWidgetItem(durationList);
    durationItem->setToolTip(0, nameItem->text(0));
    durationItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    nameItem->insertChild(1, durationItem);

    QStringList collisionList;
    collisionList.append("");
    QTreeWidgetItem* collisionItem = new QTreeWidgetItem(collisionList);
    collisionItem->setToolTip(0, nameItem->text(0));
    QCheckBox* collisionsVisibleBox = new QCheckBox(trajectory_tree_);
    collisionsVisibleBox->setText("Show Collisions");
    collisionsVisibleBox->setChecked(trajectory.areCollisionsVisible());
    collisionsVisibleBox->setToolTip(nameItem->text(0));
    nameItem->insertChild(2, collisionItem);
    trajectory_tree_->setItemWidget(collisionItem, 0, collisionsVisibleBox);
    connect(collisionsVisibleBox, SIGNAL(clicked(bool)), this, SLOT(trajectoryCollisionsVisibleButtonClicked(bool)));

    QStringList visibleList;
    visibleList.append("");
    QTreeWidgetItem* visibleItem = new QTreeWidgetItem(visibleList);
    visibleItem->setToolTip(0, nameItem->text(0));
    QCheckBox* visibleBox = new QCheckBox(trajectory_tree_);
    visibleBox->setText("Visible");
    visibleBox->setChecked(trajectory.isVisible());
    nameItem->insertChild(3, visibleItem);
    trajectory_tree_->setItemWidget(visibleItem, 0, visibleBox);
    visibleBox->setToolTip(nameItem->text(0));
    connect(visibleBox, SIGNAL(clicked(bool)), this, SLOT(trajectoryVisibleButtonClicked(bool)));

    QStringList colorList;
    colorList.append("Color");
    colorList.append("");
    QTreeWidgetItem* colorItem = new QTreeWidgetItem(colorList);
    colorItem->setToolTip(0, nameItem->text(0));
    QPushButton* colorButton = new QPushButton(trajectory_tree_);

    std::stringstream colorStream;
    colorStream<< "(" << (int)(trajectory.getColor().r*255) <<" , ";
    colorStream << (int)(trajectory.getColor().g*255) << " , ";
    colorStream << (int)(trajectory.getColor().b*255) << ")";

    colorButton->setText(QString::fromStdString(colorStream.str()));
    colorButton->setToolTip(nameItem->text(0));
    colorButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(colorButton, SIGNAL(clicked()), this, SLOT(trajectoryColorButtonClicked()));

    nameItem->insertChild(4, colorItem);
    trajectory_tree_->setItemWidget(colorItem, 1, colorButton);
  }

}

void PlanningSceneVisualizer::trajectoryCollisionsVisibleButtonClicked(bool checked)
{
  QObject* sender = QObject::sender();
  QCheckBox* button = qobject_cast<QCheckBox*> (sender);

  if(button != NULL)
  {
    std::string trajectoryID = button->toolTip().toStdString();
    TrajectoryData& data = (*trajectory_map_)[trajectoryID];
    data.setCollisionsVisible(checked);
  }
}

void PlanningSceneVisualizer::trajectoryVisibleButtonClicked(bool checked)
{
  QObject* sender = QObject::sender();
  QCheckBox* button = qobject_cast<QCheckBox*> (sender);

  if(button != NULL)
  {
    std::string trajectoryID = button->toolTip().toStdString();
    TrajectoryData& data = (*trajectory_map_)[trajectoryID];
    data.setVisible(checked);
  }
}

void PlanningSceneVisualizer::trajectoryColorButtonClicked()
{
  QObject* sender = QObject::sender();
  QPushButton* button = qobject_cast<QPushButton*>(sender);

  if(button != NULL)
  {
    std::string trajectoryID = button->toolTip().toStdString();
    TrajectoryData& data = (*trajectory_map_)[trajectoryID];
    QColor col(data.getColor().r*255, data.getColor().g*255, data.getColor().b*255);
    QColor colorSelected = QColorDialog::getColor(col, this);
    std::stringstream colorStream;
    colorStream<< "(" << colorSelected.red()<<" , ";
    colorStream << colorSelected.green()<< " , ";
    colorStream << colorSelected.blue() << ")";

    button->setText(QString::fromStdString(colorStream.str()));

    std_msgs::ColorRGBA trajColor;
    trajColor.r = colorSelected.redF();
    trajColor.g = colorSelected.greenF();
    trajColor.b = colorSelected.blueF();
    trajColor.a = 0.5f;
    data.setColor(trajColor);
    data.refreshColors();
  }
}

void PlanningSceneVisualizer::onPlanningSceneLoaded(int scene, int numScenes)
{
  emit changeProgress((int)((100.0f)*((float)(scene + 1)/ (float)numScenes)));
}

void PlanningSceneVisualizer::createPlanningSceneTable()
{
  loadAllWarehouseData();
  assert(planning_scene_map_ != NULL);
  planning_scene_table_->clear();
  int count = 0;
  for(map<string, PlanningSceneData>::iterator it = planning_scene_map_->begin(); it != planning_scene_map_->end(); it++)
  {
    count ++;
  }
  planning_scene_table_->setRowCount(count);
  planning_scene_table_->setColumnCount(3);
  QStringList labels;
  labels.append("Name");
  labels.append("Timestamp");
  labels.append("Notes");
  planning_scene_table_->setHorizontalHeaderLabels(labels);

  ROS_INFO("Num Planning Scenes: %d", planning_scene_table_->rowCount());

  int r = 0;
  for(map<string, PlanningSceneData>::iterator it = planning_scene_map_->begin(); it != planning_scene_map_->end(); it++)
  {
    PlanningSceneData& data = it->second;

    QTableWidgetItem* nameItem = new QTableWidgetItem(QString::fromStdString(data.getName()));
    nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    planning_scene_table_->setItem(r, 0, nameItem);

    QDateTime time;
    time.setTime_t((unsigned int)(data.getTimeStamp().toSec()));
    stringstream timestampStream;
    timestampStream << time.toString().toStdString();

    QTableWidgetItem* timeItem = new QTableWidgetItem(QString::fromStdString(timestampStream.str()));
    timeItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    planning_scene_table_->setItem(r, 1, timeItem);

    stringstream notesStream;

    vector<ArmNavigationErrorCodes>& errorCodes = data.getErrorCodes();


    if(errorCodes.size() == 0)
    {
      notesStream << "No Outcomes";
    }
    else
    {
      notesStream << errorCodes.size() << " Outcomes recorded. "<<data.getPipelineStages().size() << " trajectories";
    }

    QTableWidgetItem* notesItem = new QTableWidgetItem(QString::fromStdString(notesStream.str()));
    notesItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    planning_scene_table_->setItem(r, 2, notesItem);
    r++;
  }

  planning_scene_table_->setColumnWidth(0, 150);
  planning_scene_table_->setColumnWidth(1, 300);
  planning_scene_table_->setColumnWidth(2, 400);
  planning_scene_table_->setMinimumWidth(1000);

}

void PlanningSceneVisualizer::motionPlanJointControlsActiveButtonClicked(bool checked)
{
  QObject* sender = QObject::sender();
  QCheckBox* box = dynamic_cast<QCheckBox*>(sender);

  if(box == NULL)
  {
    return;
  }

  std::string ID = box->toolTip().toStdString();

  if(motion_plan_map_->find(ID) == motion_plan_map_->end())
  {
    return;
  }

  MotionPlanRequestData& data = (*motion_plan_map_)[ID];
  data.setJointControlsVisible(checked, this);
  interactive_marker_server_->applyChanges();
}

void PlanningSceneVisualizer::createNewObjectPressed()
{
  new_object_dialog_->show();
}

void PlanningSceneVisualizer::createNewObjectDialog()
{
  new_object_dialog_ = new QDialog(this);
  QVBoxLayout* layout = new QVBoxLayout(new_object_dialog_);
  QGroupBox* panel = new QGroupBox(new_object_dialog_);
  panel->setTitle("New Collision Object");

  QVBoxLayout* panelLayout = new QVBoxLayout(panel);
  collision_object_type_box_ = new QComboBox(new_object_dialog_);
  collision_object_type_box_->addItem("Box");
  collision_object_type_box_->addItem("Cylinder");
  collision_object_type_box_->addItem("Sphere");
  collision_object_type_box_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  panelLayout->addWidget(collision_object_type_box_);

  QGroupBox* scaleBox = new QGroupBox(panel);
  scaleBox->setTitle("Scale (x,y,z) cm");
  QHBoxLayout* scaleLayout = new QHBoxLayout(scaleBox);

  collision_object_scale_x_box_ = new QSpinBox(scaleBox);
  collision_object_scale_y_box_ = new QSpinBox(scaleBox);
  collision_object_scale_z_box_ = new QSpinBox(scaleBox);
  collision_object_scale_x_box_->setRange(1, 10000);
  collision_object_scale_y_box_->setRange(1, 10000);
  collision_object_scale_z_box_->setRange(1, 10000);
  collision_object_scale_x_box_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  collision_object_scale_y_box_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  collision_object_scale_z_box_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  scaleLayout->addWidget(collision_object_scale_x_box_);
  scaleLayout->addWidget(collision_object_scale_y_box_);
  scaleLayout->addWidget(collision_object_scale_z_box_);

  scaleBox->setLayout(scaleLayout);
  panelLayout->addWidget(scaleBox);

  QGroupBox* posBox = new QGroupBox(panel);
  posBox->setTitle("Position (x,y,z) cm");
  QHBoxLayout* posLayout = new QHBoxLayout(posBox);


  collision_object_pos_x_box_ = new QSpinBox(posBox);
  collision_object_pos_y_box_ = new QSpinBox(posBox);
  collision_object_pos_z_box_ = new QSpinBox(posBox);
  collision_object_pos_x_box_->setRange(-10000, 10000);
  collision_object_pos_y_box_->setRange(-10000, 10000);
  collision_object_pos_z_box_->setRange(-10000, 10000);
  collision_object_pos_x_box_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  collision_object_pos_y_box_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  collision_object_pos_z_box_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  posLayout->addWidget(collision_object_pos_x_box_);
  posLayout->addWidget(collision_object_pos_y_box_);
  posLayout->addWidget(collision_object_pos_z_box_);

  posBox->setLayout(posLayout);
  panelLayout->addWidget(posBox);

  panel->setLayout(panelLayout);
  layout->addWidget(panel);

  make_object_button_ = new QPushButton(new_object_dialog_);
  make_object_button_->setText("Create...");
  make_object_button_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  connect(make_object_button_, SIGNAL(clicked()), this, SLOT(createObjectConfirmedPressed()));

  layout->addWidget(make_object_button_);
  new_object_dialog_->setLayout(layout);

}

void PlanningSceneVisualizer::createObjectConfirmedPressed()
{
  geometry_msgs::Pose pose;
  pose.position.x = (float)collision_object_pos_x_box_->value() / 100.0f;
  pose.position.y = (float)collision_object_pos_y_box_->value() / 100.0f;
  pose.position.z = (float)collision_object_pos_z_box_->value() / 100.0f;
  pose.orientation.x = 0;
  pose.orientation.y = 0;
  pose.orientation.z = 0;
  pose.orientation.w = 1;

  std::string name = collision_object_type_box_->currentText().toStdString();
  PlanningSceneEditor::GeneratedShape shape;
  if(name == "Box")
  {
    shape = PlanningSceneEditor::Box;
  }
  if(name == "Cylinder")
  {
    shape = PlanningSceneEditor::Cylinder;
  }
  if(name == "Sphere")
  {
    shape = PlanningSceneEditor::Sphere;
  }

  createCollisionObject(pose, shape, (float)collision_object_scale_x_box_->value() / 100.0f,
                        (float)collision_object_scale_y_box_->value() / 100.0f,
                        (float)collision_object_scale_z_box_->value() / 100.0f);
}

void PlanningSceneVisualizer::createRequestDialog()
{
  new_request_dialog_ = new QDialog(this);
  QVBoxLayout* layout = new QVBoxLayout(new_request_dialog_);
  QGroupBox* box = new QGroupBox(new_request_dialog_);
  QVBoxLayout* boxLayout = new QVBoxLayout(box);
  box->setTitle("New Motion Plan Request");
  QLabel* boxLabel = new QLabel(box);
  boxLabel->setText("Planning Group:");
  request_group_name_box_ = new QComboBox(box);

  if(params_.right_arm_group_ != "none")
  {
    request_group_name_box_->addItem(QString::fromStdString(params_.right_arm_group_));
  }

  if(params_.left_arm_group_ != "none")
  {
    request_group_name_box_->addItem(QString::fromStdString(params_.left_arm_group_));
  }

  create_request_from_robot_box_ = new QCheckBox(box);
  create_request_from_robot_box_->setChecked(false);
  create_request_from_robot_box_->setText("Start From Current Robot State");

  boxLayout->addWidget(boxLabel);
  boxLayout->addWidget(request_group_name_box_);
  boxLayout->addWidget(create_request_from_robot_box_);

  QPushButton* createRequestButton = new QPushButton(box);
  createRequestButton->setText("Create ...");
  createRequestButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  boxLayout->addWidget(createRequestButton);

  connect(createRequestButton, SIGNAL(clicked()), this, SLOT(createRequestPressed()));
  layout->addWidget(box);
  new_request_dialog_->setLayout(layout);
}

void PlanningSceneVisualizer::createRequestPressed()
{
  std::string group_name = request_group_name_box_->currentText().toStdString();
  std::string end_effector_name = "";

  if(group_name == params_.right_arm_group_)
  {
    end_effector_name = params_.right_ik_link_;
  }
  else if(group_name == params_.left_arm_group_)
  {
    end_effector_name = params_.left_ik_link_;
  }
  else
  {
    return;
  }
  createNewMotionPlanRequest(group_name, end_effector_name);
  createMotionPlanTable();
  new_request_dialog_->close();
}

void marker_function()
{
  unsigned int counter = 0;
  while(ros::ok())
  {
    if(inited)
    {
      if(counter % CONTROL_SPEED == 0)
      {
        counter = 1;
        psv->sendMarkers();
      }
      else
      {
        counter++;
      }

      if(psv->quit_threads_)
      {
        break;
      }
    }
    usleep(5000);
  }

}

void spin_function()
{
  ros::WallRate r(100.0);
  while(ros::ok() && !inited)
  {
    r.sleep();
    ros::spinOnce();
  }
  while(ros::ok() && !psv->quit_threads_)
  {
    r.sleep();
    ros::spinOnce();
  }
}

void quit(int sig)
{
  if(psv != NULL)
  {
    delete psv;
  }
}

int main(int argc, char** argv)
{
  ros::init(argc, argv, "planning_scene_warehouse_viewer", ros::init_options::NoSigintHandler);

  boost::thread spin_thread(boost::bind(&spin_function));
  boost::thread marker_thread(boost::bind(&marker_function));

  QApplication* app = new QApplication(argc, argv);
  planning_scene_utils::PlanningSceneParameters params;
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

  ParameterDialog* dialog = new ParameterDialog(params);
  dialog->exec();
  dialog->updateParams();
  psv = new PlanningSceneVisualizer(NULL, dialog->params_);
  app->setActiveWindow(psv);
  psv->show();
  inited = true;

  int ret = app->exec();
  psv->quit_threads_ = true;
  spin_thread.join();
  marker_thread.join();
  return ret;

}
