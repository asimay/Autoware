/*
 *  Copyright (c) 2015, Nagoya University
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Autoware nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ros/ros.h>
#include <visualization_msgs/MarkerArray.h>
#include <std_msgs/Int32.h>
#include <iostream>

#include "libvelocity_set.h"
#include "velocity_set_path.h"
#include "velocity_set_info.h"

namespace
{
constexpr int LOOP_RATE = 10;
constexpr double DECELERATION_SEARCH_DISTANCE = 30;
constexpr double STOP_SEARCH_DISTANCE = 60;


// Display a detected obstacle
void displayObstacle(const EControl &kind, const ObstaclePoints& obstacle_points, const ros::Publisher& obstacle_pub)
{
  visualization_msgs::Marker marker;
  marker.header.frame_id = "/map";
  marker.header.stamp = ros::Time();
  marker.ns = "my_namespace";
  marker.id = 0;
  marker.type = visualization_msgs::Marker::CUBE;
  marker.action = visualization_msgs::Marker::ADD;

  static geometry_msgs::Point prev_obstacle_point;
  if (kind == STOP || kind == DECELERATE)
  {
    marker.pose.position = obstacle_points.getObstaclePoint(kind);
    prev_obstacle_point = marker.pose.position;
  }
  else // kind == OTHERS
  {
    marker.pose.position = prev_obstacle_point;
  }
  geometry_msgs::Quaternion quat;
  marker.pose.orientation = quat;//g_localizer_pose.pose.orientation;

  marker.scale.x = 1.0;
  marker.scale.y = 1.0;
  marker.scale.z = 2.0;
  marker.color.a = 0.7;
  if (kind == STOP)
  {
    marker.color.r = 1.0;
    marker.color.g = 0.0;
    marker.color.b = 0.0;
  }
  else
  {
    marker.color.r = 1.0;
    marker.color.g = 1.0;
    marker.color.b = 0.0;
  }
  marker.lifetime = ros::Duration(0.1);
  marker.frame_locked = true;

  obstacle_pub.publish(marker);
}

void displayDetectionRange(const waypoint_follower::lane& lane, const CrossWalk& crosswalk, const int closest_waypoint, const EControl &kind, const int obstacle_waypoint, const double stop_range, const double deceleration_range, const ros::Publisher& detection_range_pub)
{
  // set up for marker array
  visualization_msgs::MarkerArray marker_array;
  visualization_msgs::Marker crosswalk_marker;
  visualization_msgs::Marker waypoint_marker_stop;
  visualization_msgs::Marker waypoint_marker_decelerate;
  visualization_msgs::Marker stop_line;
  crosswalk_marker.header.frame_id = "/map";
  crosswalk_marker.header.stamp = ros::Time();
  crosswalk_marker.id = 0;
  crosswalk_marker.type = visualization_msgs::Marker::SPHERE_LIST;
  crosswalk_marker.action = visualization_msgs::Marker::ADD;
  waypoint_marker_stop = crosswalk_marker;
  waypoint_marker_decelerate = crosswalk_marker;
  stop_line = crosswalk_marker;
  stop_line.type = visualization_msgs::Marker::CUBE;

  // set each namespace
  crosswalk_marker.ns = "Crosswalk Detection";
  waypoint_marker_stop.ns = "Stop Detection";
  waypoint_marker_decelerate.ns = "Decelerate Detection";
  stop_line.ns = "Stop Line";

  // set scale and color
  double scale = 2 * stop_range;
  waypoint_marker_stop.scale.x = scale;
  waypoint_marker_stop.scale.y = scale;
  waypoint_marker_stop.scale.z = scale;
  waypoint_marker_stop.color.a = 0.2;
  waypoint_marker_stop.color.r = 0.0;
  waypoint_marker_stop.color.g = 1.0;
  waypoint_marker_stop.color.b = 0.0;
  waypoint_marker_stop.frame_locked = true;

  scale = 2 * (stop_range + deceleration_range);
  waypoint_marker_decelerate.scale.x = scale;
  waypoint_marker_decelerate.scale.y = scale;
  waypoint_marker_decelerate.scale.z = scale;
  waypoint_marker_decelerate.color.a = 0.15;
  waypoint_marker_decelerate.color.r = 1.0;
  waypoint_marker_decelerate.color.g = 1.0;
  waypoint_marker_decelerate.color.b = 0.0;
  waypoint_marker_decelerate.frame_locked = true;

  if (obstacle_waypoint > -1)
  {
    stop_line.pose.position = lane.waypoints[obstacle_waypoint].pose.pose.position;
    stop_line.pose.orientation = lane.waypoints[obstacle_waypoint].pose.pose.orientation;
  }
  stop_line.pose.position.z += 1.0;
  stop_line.scale.x = 0.1;
  stop_line.scale.y = 15.0;
  stop_line.scale.z = 2.0;
  stop_line.color.a = 0.3;
  stop_line.color.r = 1.0;
  stop_line.color.g = 0.0;
  stop_line.color.b = 0.0;
  stop_line.lifetime = ros::Duration(0.1);
  stop_line.frame_locked = true;

  int crosswalk_id = crosswalk.getDetectionCrossWalkID();
  if (crosswalk_id > 0)
    scale = crosswalk.getDetectionPoints(crosswalk_id).width;
  crosswalk_marker.scale.x = scale;
  crosswalk_marker.scale.y = scale;
  crosswalk_marker.scale.z = scale;
  crosswalk_marker.color.a = 0.5;
  crosswalk_marker.color.r = 0.0;
  crosswalk_marker.color.g = 1.0;
  crosswalk_marker.color.b = 0.0;
  crosswalk_marker.frame_locked = true;

  // set marker points coordinate
  for (int i = 0; i < STOP_SEARCH_DISTANCE; i++)
  {
    if (closest_waypoint < 0 || i + closest_waypoint > static_cast<int>(lane.waypoints.size()) - 1)
      break;

    geometry_msgs::Point point;
    point = lane.waypoints[closest_waypoint + i].pose.pose.position;

    waypoint_marker_stop.points.push_back(point);

    if (i > DECELERATION_SEARCH_DISTANCE)
      continue;
    waypoint_marker_decelerate.points.push_back(point);
  }

  if (crosswalk_id > 0)
  {
    for (const auto &p : crosswalk.getDetectionPoints(crosswalk_id).points)
      crosswalk_marker.points.push_back(p);
  }

  // publish marker
  marker_array.markers.push_back(crosswalk_marker);
  marker_array.markers.push_back(waypoint_marker_stop);
  marker_array.markers.push_back(waypoint_marker_decelerate);
  if (kind == STOP)
    marker_array.markers.push_back(stop_line);
  detection_range_pub.publish(marker_array);
  marker_array.markers.clear();
}

// obstacle detection for crosswalk
EControl crossWalkDetection(const pcl::PointCloud<pcl::PointXYZ>& points, const CrossWalk& crosswalk, const geometry_msgs::PoseStamped& localizer_pose, const int points_threshold, ObstaclePoints* obstacle_points)
{
  int crosswalk_id = crosswalk.getDetectionCrossWalkID();
  double search_radius = crosswalk.getDetectionPoints(crosswalk_id).width / 2;

  // Search each calculated points in the crosswalk
  for (const auto &p : crosswalk.getDetectionPoints(crosswalk_id).points)
  {
    geometry_msgs::Point detection_point = calcRelativeCoordinate(p, localizer_pose.pose);
    tf::Vector3 detection_vector = point2vector(detection_point);
    detection_vector.setZ(0.0);

    int stop_count = 0;  // the number of points in the detection area
    for (const auto &p : points)
    {
      tf::Vector3 point_vector(p.x, p.y, 0.0);
      double distance = tf::tfDistance(point_vector, detection_vector);
      if (distance < search_radius)
      {
        stop_count++;
        geometry_msgs::Point point_temp;
        point_temp.x = p.x;
        point_temp.y = p.y;
        point_temp.z = p.z;
	obstacle_points->setStopPoint(calcAbsoluteCoordinate(point_temp, localizer_pose.pose));
      }
      if (stop_count > points_threshold)
        return STOP;
    }

    obstacle_points->clearStopPoints();
  }

  return KEEP;  // find no obstacles
}

int detectStopObstacle(const pcl::PointCloud<pcl::PointXYZ>& points, const int closest_waypoint, const waypoint_follower::lane& lane, const CrossWalk& crosswalk, double stop_range, double points_threshold, const geometry_msgs::PoseStamped& localizer_pose, ObstaclePoints* obstacle_points)
{
  int stop_obstacle_waypoint = -1;
  // start search from the closest waypoint
  for (int i = closest_waypoint; i < closest_waypoint + STOP_SEARCH_DISTANCE; i++)
  {
    // reach the end of waypoints
    if (i >= static_cast<int>(lane.waypoints.size()))
      break;

    // Detection for cross walk
    if (i == crosswalk.getDetectionWaypoint())
    {
      // found an obstacle in the cross walk
      if (crossWalkDetection(points, crosswalk, localizer_pose, points_threshold, obstacle_points) == STOP)
      {
        stop_obstacle_waypoint = i;
        break;
      }
    }

    // waypoint seen by localizer
    geometry_msgs::Point waypoint = calcRelativeCoordinate(lane.waypoints[i].pose.pose.position, localizer_pose.pose);
    tf::Vector3 tf_waypoint = point2vector(waypoint);
    tf_waypoint.setZ(0);

    int stop_point_count = 0;
    for (const auto& p : points)
    {
      tf::Vector3 point_vector(p.x, p.y, 0);

      // 2D distance between waypoint and points (obstacle)
      double dt = tf::tfDistance(point_vector, tf_waypoint);
      if (dt < stop_range)
      {
        stop_point_count++;
        geometry_msgs::Point point_temp;
        point_temp.x = p.x;
        point_temp.y = p.y;
        point_temp.z = p.z;
	obstacle_points->setStopPoint(calcAbsoluteCoordinate(point_temp, localizer_pose.pose));
      }
    }

    // there is an obstacle if the number of points exceeded the threshold
    if (stop_point_count > points_threshold)
    {
      stop_obstacle_waypoint = i;
      break;
    }

    obstacle_points->clearStopPoints();

    // check next waypoint...
  }

  return stop_obstacle_waypoint;
}

int detectDecelerateObstacle(const pcl::PointCloud<pcl::PointXYZ>& points, const int closest_waypoint, const waypoint_follower::lane& lane, const double stop_range, const double deceleration_range, const double points_threshold, const geometry_msgs::PoseStamped& localizer_pose, ObstaclePoints* obstacle_points)
{
  int decelerate_obstacle_waypoint = -1;
  // start search from the closest waypoint
  for (int i = closest_waypoint; i < closest_waypoint + DECELERATION_SEARCH_DISTANCE; i++)
  {
    // reach the end of waypoints
    if (i >= static_cast<int>(lane.waypoints.size()))
      break;

    // waypoint seen by localizer
    geometry_msgs::Point waypoint = calcRelativeCoordinate(lane.waypoints[i].pose.pose.position, localizer_pose.pose);
    tf::Vector3 tf_waypoint = point2vector(waypoint);
    tf_waypoint.setZ(0);

    int decelerate_point_count = 0;
    for (const auto& p : points)
    {
      tf::Vector3 point_vector(p.x, p.y, 0);

      // 2D distance between waypoint and points (obstacle)
      double dt = tf::tfDistance(point_vector, tf_waypoint);
      if (dt > stop_range && dt < stop_range + deceleration_range)
      {
        decelerate_point_count++;
        geometry_msgs::Point point_temp;
        point_temp.x = p.x;
        point_temp.y = p.y;
        point_temp.z = p.z;
	obstacle_points->setDeceleratePoint(calcAbsoluteCoordinate(point_temp, localizer_pose.pose));
      }
    }

    // there is an obstacle if the number of points exceeded the threshold
    if (decelerate_point_count > points_threshold)
    {
      decelerate_obstacle_waypoint = i;
      break;
    }

    obstacle_points->clearDeceleratePoints();

    // check next waypoint...
  }

  return decelerate_obstacle_waypoint;
}


// Detect an obstacle by using pointcloud
EControl pointsDetection(const pcl::PointCloud<pcl::PointXYZ>& points, const int closest_waypoint, const waypoint_follower::lane& lane, const CrossWalk& crosswalk, const VelocitySetInfo& vs_info, int* obstacle_waypoint, ObstaclePoints* obstacle_points)
{
  if (points.empty() == true || closest_waypoint < 0)
    return KEEP;

  int stop_obstacle_waypoint = detectStopObstacle(points, closest_waypoint, lane, crosswalk, vs_info.getStopRange(), vs_info.getPointsThreshold(), vs_info.getLocalizerPose(), obstacle_points);

  // skip searching deceleration range
  if (vs_info.getDecelerationRange() < 0.01)
  {
    *obstacle_waypoint = stop_obstacle_waypoint;
    return stop_obstacle_waypoint < 0 ? KEEP : STOP;
  }

  int decelerate_obstacle_waypoint = detectDecelerateObstacle(points, closest_waypoint, lane, vs_info.getStopRange(), vs_info.getDecelerationRange(), vs_info.getPointsThreshold(), vs_info.getLocalizerPose(), obstacle_points);

  // stop obstacle was not found
  if (stop_obstacle_waypoint < 0)
  {
    *obstacle_waypoint  = decelerate_obstacle_waypoint;
    return decelerate_obstacle_waypoint < 0 ? KEEP : DECELERATE;
  }

  // stop obstacle was found but decelerate obstacle was not found
  if (decelerate_obstacle_waypoint < 0)
  {
    *obstacle_waypoint = stop_obstacle_waypoint;
    return STOP;
  }

  // about 5.0 meter
  double waypoint_interval = getPlaneDistance(lane.waypoints[0].pose.pose.position, lane.waypoints[1].pose.pose.position);
  int stop_decelerate_threshold = 5 / waypoint_interval;

  // both were found
  if (stop_obstacle_waypoint - decelerate_obstacle_waypoint > stop_decelerate_threshold)
  {
    *obstacle_waypoint = decelerate_obstacle_waypoint;
    return DECELERATE;
  }
  else
  {
    *obstacle_waypoint = stop_obstacle_waypoint;
    return STOP;
  }

}

EControl obstacleDetection(int closest_waypoint, const waypoint_follower::lane& lane, const CrossWalk& crosswalk, const VelocitySetInfo vs_info, const ros::Publisher& detection_range_pub, const ros::Publisher& obstacle_pub, int* obstacle_waypoint)
{
  ObstaclePoints obstacle_points;
  EControl detection_result = pointsDetection(vs_info.getPoints(), closest_waypoint, lane, crosswalk, vs_info, obstacle_waypoint, &obstacle_points);
  displayDetectionRange(lane, crosswalk, closest_waypoint, detection_result, *obstacle_waypoint, vs_info.getStopRange(), vs_info.getDecelerationRange(), detection_range_pub);

  static int false_count = 0;
  static EControl prev_detection = KEEP;
  static int prev_obstacle_waypoint = -1;

  // stop or decelerate because we found obstacles
  if (detection_result == STOP || detection_result == DECELERATE)
  {
    displayObstacle(detection_result, obstacle_points, obstacle_pub);
      prev_detection = detection_result;
      false_count = 0;
      prev_obstacle_waypoint = *obstacle_waypoint;
      return detection_result;
  }

  // there are no obstacles, but wait a little for safety
  if (prev_detection == STOP || prev_detection == DECELERATE)
  {
    false_count++;

    if (false_count < LOOP_RATE / 2)
    {
      *obstacle_waypoint = prev_obstacle_waypoint;
      displayObstacle(OTHERS, obstacle_points, obstacle_pub);
      return prev_detection;
    }
  }

  // there are no obstacles, so we move forward
  *obstacle_waypoint = -1;
  false_count = 0;
  prev_detection = KEEP;
  return detection_result;
}

void changeWaypoints(const VelocitySetInfo& vs_info, const EControl& detection_result, int closest_waypoint, int obstacle_waypoint, const ros::Publisher& temporal_waypoints_pub, VelocitySetPath* vs_path)
{
  if (detection_result == STOP)
  {  // STOP for obstacle
    // stop_waypoint is about g_stop_distance meter away from obstacles
    int stop_waypoint = obstacle_waypoint - vs_info.getStopDistance() / vs_path->calcInterval(0, 1);

    // change waypoints to stop by the stop_waypoint
    vs_path->changeWaypoints(stop_waypoint, closest_waypoint, vs_info.getDeceleration());
    vs_path->avoidSuddenBraking(vs_info.getVelocityChangeLimit(), vs_info.getDeceleration(), closest_waypoint);
    vs_path->setTemporalWaypoints(vs_info.getTemporalWaypointsSize(), closest_waypoint, vs_info.getControlPose());
    temporal_waypoints_pub.publish(vs_path->getTemporalWaypoints());
  }
  else if (detection_result == DECELERATE)
  {  // DECELERATE for obstacles
    vs_path->initializeNewWaypoints();
    vs_path->setDeceleration(vs_info.getDeceleration(), closest_waypoint);
    vs_path->setTemporalWaypoints(vs_info.getTemporalWaypointsSize(), closest_waypoint, vs_info.getControlPose());
    temporal_waypoints_pub.publish(vs_path->getTemporalWaypoints());
  }
  else
  {  // ACELERATE or KEEP
    vs_path->initializeNewWaypoints();
    vs_path->avoidSuddenAceleration(vs_info.getDeceleration(), closest_waypoint);
    vs_path->avoidSuddenBraking(vs_info.getVelocityChangeLimit(), vs_info.getDeceleration(), closest_waypoint);
    vs_path->setTemporalWaypoints(vs_info.getTemporalWaypointsSize(), closest_waypoint, vs_info.getControlPose());
    temporal_waypoints_pub.publish(vs_path->getTemporalWaypoints());
  }
}

} // end namespace


int main(int argc, char **argv)
{
  ros::init(argc, argv, "velocity_set");

  ros::NodeHandle nh;
  ros::NodeHandle private_nh("~");

  bool use_crosswalk_detection;
  std::string points_topic;
  private_nh.param<bool>("use_crosswalk_detection", use_crosswalk_detection, true);
  private_nh.param<std::string>("points_topic", points_topic, "points_lanes");

  // class
  CrossWalk crosswalk;
  VelocitySetPath vs_path;
  VelocitySetInfo vs_info;

  // velocity set subscriber
  ros::Subscriber waypoints_sub = nh.subscribe("base_waypoints", 1, &VelocitySetPath::waypointsCallback, &vs_path);
  ros::Subscriber current_vel_sub = nh.subscribe("current_velocity", 1, &VelocitySetPath::currentVelocityCallback, &vs_path);

  // velocity set info subscriber
  ros::Subscriber config_sub = nh.subscribe("config/velocity_set", 1, &VelocitySetInfo::configCallback, &vs_info);
  ros::Subscriber points_sub = nh.subscribe(points_topic, 1, &VelocitySetInfo::pointsCallback, &vs_info);
  ros::Subscriber localizer_sub = nh.subscribe("localizer_pose", 1, &VelocitySetInfo::localizerPoseCallback, &vs_info);
  ros::Subscriber control_pose_sub = nh.subscribe("current_pose", 1, &VelocitySetInfo::controlPoseCallback, &vs_info);

  // vector map subscriber
  ros::Subscriber sub_dtlane = nh.subscribe("vector_map_info/cross_walk", 1, &CrossWalk::crossWalkCallback, &crosswalk);
  ros::Subscriber sub_area = nh.subscribe("vector_map_info/area", 1, &CrossWalk::areaCallback, &crosswalk);
  ros::Subscriber sub_line = nh.subscribe("vector_map_info/line", 1, &CrossWalk::lineCallback, &crosswalk);
  ros::Subscriber sub_point = nh.subscribe("vector_map_info/point", 1, &CrossWalk::pointCallback, &crosswalk);

  // publisher
  ros::Publisher detection_range_pub = nh.advertise<visualization_msgs::MarkerArray>("detection_range", 0);
  ros::Publisher temporal_waypoints_pub = nh.advertise<waypoint_follower::lane>("temporal_waypoints", 1000, true);
  ros::Publisher closest_waypoint_pub = nh.advertise<std_msgs::Int32>("closest_waypoint", 1000);
  ros::Publisher obstacle_pub = nh.advertise<visualization_msgs::Marker>("obstacle", 0);

  ros::Rate loop_rate(LOOP_RATE);
  while (ros::ok())
  {
    ros::spinOnce();

    if (crosswalk.loaded_all && !crosswalk.set_points)
      crosswalk.setCrossWalkPoints();

    if (!vs_info.getSetPose() || !vs_path.getSetPath())
    {
      loop_rate.sleep();
      continue;
    }

    int closest_waypoint = getClosestWaypoint(vs_path.getPrevWaypoints(), vs_info.getControlPose().pose);

    std_msgs::Int32 closest_waypoint_msg;
    closest_waypoint_msg.data = closest_waypoint;
    closest_waypoint_pub.publish(closest_waypoint_msg);

    if (use_crosswalk_detection)
      crosswalk.setDetectionWaypoint(crosswalk.findClosestCrosswalk(closest_waypoint, vs_path.getPrevWaypoints(), STOP_SEARCH_DISTANCE));

    int obstacle_waypoint = -1;
    EControl detection_result = obstacleDetection(closest_waypoint, vs_path.getPrevWaypoints(), crosswalk, vs_info, detection_range_pub, obstacle_pub, &obstacle_waypoint);

    changeWaypoints(vs_info, detection_result, closest_waypoint, obstacle_waypoint, temporal_waypoints_pub, &vs_path);

    vs_info.clearPoints();

    loop_rate.sleep();
  }

  return 0;
}
