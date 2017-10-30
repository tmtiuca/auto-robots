//  ///////////////////////////////////////////////////////////
//
// Lab2 Code for MTE 544
// This file contains example code for use with ME 597 lab 2
// It outlines the basic setup of a ros node and the various
// inputs and outputs needed for this lab
//
// Author: Rishab Sareen & Pavel Shering
//
// //////////////////////////////////////////////////////////


#include <ros/ros.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/Twist.h>
#include <tf/transform_datatypes.h>
#include <gazebo_msgs/ModelStates.h>
#include <visualization_msgs/Marker.h>
#include <nav_msgs/OccupancyGrid.h>
#include <sensor_msgs/LaserScan.h>
#include <fstream>
#include <cmath>
#include <sstream>
#include <tf/transform_broadcaster.h>
#include <string>

double ips_x;
double ips_y;
double ips_yaw;
const int8_t map_resolution = 20;
nav_msgs::OccupancyGrid map;

// typedef char int8;

short sgn(int x) { return x >= 0 ? 1 : -1; }

//Bresenham line algorithm (pass empty vectors)
// Usage: (x0, y0) is the first point and (x1, y1) is the second point. The calculated
//        points (x, y) are stored in the x and y vector. x and y should be empty
//	  vectors of integers and shold be defined where this function is called from.
void bresenham(int x0, int y0, int x1, int y1, std::vector<int>& x, std::vector<int>& y) {

    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int dx2 = x1 - x0;
    int dy2 = y1 - y0;

    const bool s = abs(dy) > abs(dx);

    if (s) {
        int dx2 = dx;
        dx = dy;
        dy = dx2;
    }

    int inc1 = 2 * dy;
    int d = inc1 - dx;
    int inc2 = d - dx;

    x.push_back(x0);
    y.push_back(y0);

    while (x0 != x1 || y0 != y1) {
        if (s) y0+=sgn(dy2); else x0+=sgn(dx2);
        if (d < 0) d += inc1;
        else {
            d += inc2;
            if (s) x0+=sgn(dx2); else y0+=sgn(dy2);
        }

        //Add point to vector
        x.push_back(x0);
        y.push_back(y0);
    }
}

//Callback function for the Position topic (SIMULATION)
void pose_callback(const gazebo_msgs::ModelStates& msg)
{

    int i;
    for(i = 0; i < msg.name.size(); i++) if(msg.name[i] == "mobile_base") break;

    ips_x = msg.pose[i].position.x ;
    ips_y = msg.pose[i].position.y ;
    ips_yaw = tf::getYaw(msg.pose[i].orientation);

}

//Callback function for the Position topic (LIVE)
/*
void pose_callback(const geometry_msgs::PoseWithCovarianceStamped& msg)
{

	ips_x X = msg.pose.pose.position.x; // Robot X psotition
	ips_y Y = msg.pose.pose.position.y; // Robot Y psotition
	ips_yaw = tf::getYaw(msg.pose.pose.orientation); // Robot Yaw
	ROS_DEBUG("pose_callback X: %f Y: %f Yaw: %f", X, Y, Yaw);
}*/

//Callback function for the Laser Scan data topic
void laser_callback(const sensor_msgs::LaserScan& msg)
{

}

void map_build()
{
    // map.header.frame_id = ros::this_node::getName() + "/local_map";
    map.info.map_load_time = ros::Time::now();
    map.info.resolution = 5/map_resolution;
    map.info.width = map_resolution;
    map.info.height = map_resolution;

    map.info.origin.position.x = -static_cast<double>(map.info.width) / 2 * map.info.resolution;
    map.info.origin.position.x = -static_cast<double>(map.info.height) / 2 * map.info.resolution;
    map.info.origin.orientation.w = 1.0;
    map.data.assign(map.info.width * map.info.height, -1); // fill the map with "unknown" occupancy of -1
    ROS_INFO("%d", map.data.size());
    // log_odds = log(occupancy / (1 - occupancy)); // prefill with equiqual probablity between being occupied and free
    // occupancy = 0.5;
    // log_odds.assign(map.info.width*map.info.height, 0);

}

bool save_map(const std::string& name)
{
    std::string filename;
    if (name.empty()) {
        const ros::Time time = ros::Time::now();
        const int sec = time.sec;
        const int nsec = time.nsec;

        std::stringstream sname;
        sname << "map_";
        sname << std::setw(5) << std::setfill('0') << sec;
        sname << std::setw(0) << "_";
        sname << std::setw(9) << std::setfill('0') << nsec;
        sname << std::setw(0) << ".txt";
        filename = sname.str();
    } else {
     filename = name;
    }

    std::ofstream ofs;
    ofs.open(filename.c_str());
    if (!ofs.is_open()) {
        ROS_ERROR("Cannot open %s", filename.c_str());
        return false;
    }
    for (size_t i = 0; i < map.data.size(); i++) {
        ofs << static_cast<int>(map.data[i]);
        if ((i % map.info.width) == (map.info.width - 1)) {
            ofs << "\n";
        } else {
            ofs << ",";
        }
    }
    ofs.close();
    return true;
}

int main(int argc, char **argv)
{
	  //Initialize the ROS framework
    ros::init(argc,argv,"main_control");
    ros::NodeHandle n;

    //Create tf broadcaster
    tf::TransformBroadcaster broadcaster;

    //Subscribe to the desired topics and assign callbacks
    ros::Subscriber pose_sub = n.subscribe("/gazebo/model_states", 1, pose_callback);
    ros::Subscriber laser_sub = n.subscribe("/scan", 1, laser_callback);

    //Setup topics that this node will Publish to
    ros::Publisher map_publisher = n.advertise<nav_msgs::OccupancyGrid>("/map",1);

    //Initialize our empty map grid
    map_build();
    bool res = save_map("1");
    ROS_INFO("%d\n", res);

    //Set the loop rate
    ros::Rate loop_rate(20);    //20Hz update rate

    while (ros::ok())
    {
        loop_rate.sleep(); //Maintain the loop rate
        ros::spinOnce();   //Check for new messages
      //Main loop code goes here:
        map_publisher.publish(map);
        broadcaster.sendTransform(
          tf::StampedTransform(
            tf::Transform(
              tf::Quaternion(0, 0, 0, 1), tf::Vector3(0.0, 0.0, 0.0)),
              ros::Time::now(),"base_link", "map"));
    }

    return 0;
}
