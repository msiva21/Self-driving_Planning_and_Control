#include <mpc_traj_follower/perception.h>

namespace mpc_traj_follower {

Perception::Perception(ros::NodeHandle& nh) : nh_(nh)
{
    // Preprocessing - read roadmap waypoints and obstacles
    readRoadmapFromCSV();

    /* ROS */
    sub_cur_state_ = nh_.subscribe("/vehicle_states", 10, &Perception::vehicleStateCallback, this);
    pub_road_cond_ = nh_.advertise<hkj_msgs::RoadConditionVector>("/perception", 10);

    ROS_INFO("Initialized perception_node!");
}

Perception::~Perception() {}

void Perception::vehicleStateCallback(const hkj_msgs::VehicleState::ConstPtr& msg)
{
    // clear stored vehicle state data
    clearVehicleState();
    // set receiving new veh state flag true
    new_veh_state_ = true;

    received_veh_state_.push_back(msg->pos_x);
    received_veh_state_.push_back(msg->pos_y);
    received_veh_state_.push_back(msg->vel_x);
    received_veh_state_.push_back(msg->vel_y);
    received_veh_state_.push_back(msg->yaw_angle);
    received_veh_state_.push_back(msg->yaw_rate);
}

bool Perception::readRoadmapFromCSV()
{
    /** TODO List: 
     *    1. read roadmap data (waypoints + obstacle) from a CSV file
     *    2. store the read information above into class member variables
     *    3. return true/flase (maybe set a flag indicating if perception prepared correctly)
     * */
    std::string roadmap_file;
    if (nh_.getParam("roadmap_file", roadmap_file))
    {
        ROS_INFO("Roadmap file:\n%s", roadmap_file.c_str());
        // Roadmap format: bl_x, bl_y, br_x, br_y, bc_x, bc_y, theta
        parseRoadMap(roadmap_file);
    }
    else
        ROS_ERROR("Not able to find roadmap file.");

}

void Perception::parseRoadMap(const std::string& roadmap)
{
    // Parse the roadmap file and add waypoints
    std::istringstream rm_stream(roadmap);
    std::string line;
    while (std::getline(rm_stream, line))
        parseRoadMapLine(line);
    ROS_INFO("Map parsed. We have %i waypoints", (int)waypoints_.size());
    assert (waypoints_.size());
}

void Perception::parseRoadMapLine(const std::string& line)
{
    // Parse each line in roadmap file and add waypoint vector
    std::istringstream line_stream(line);
    std::string number;
    std::vector<float> wp;
    while (std::getline(line_stream, number, ','))
        wp.push_back(std::stof(number));
    waypoints_.push_back(wp);
}

float Perception::distance(float x1, float y1, float x2, float y2)
{
    return sqrtf32((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
}

float Perception::distanceToVehicle(int index)
{
    // Compute the distance between vehicle and #index waypoint
    // We have three points (left, right and center) and we use the minimal distance among the three
    assert (index >= 0 && index < waypoints_.size());
    float dist1 = distance(state_pos_x_, state_pos_y_, waypoints_[index][0], waypoints_[index][1]);
    float dist2 = distance(state_pos_x_, state_pos_y_, waypoints_[index][2], waypoints_[index][3]);
    float dist3 = distance(state_pos_x_, state_pos_y_, waypoints_[index][4], waypoints_[index][5]);
    return std::min({dist1, dist2, dist3});
}

int Perception::getNextWaypointIndex()
{
    int index = 0, min_dist = INT_MAX;
    for (size_t i=0; i<waypoints_.size(); ++i)
    {
        int dist = distanceToVehicle(index);
        if (dist < min_dist)
        {
            min_dist = dist;
            index = i;
        }
    }

    // We return the waypoint index that is 'ahead' of vehicle
    float diff_x = waypoints_[index][4] - state_pos_x_;
    float diff_y = waypoints_[index][5] - state_pos_y_;
    float cos_x = std::cos(waypoints_[index][6]), sin_x = std::sin(waypoints_[index][6]);
    return diff_x*cos_x + diff_y*sin_x > 0 ? index : index+1;
}

std::vector<std::vector<float>> Perception::getWaypoints(int index)
{
    double distance_can_see;
    nh_.getParam("distance_can_see", distance_can_see);
    double dist = 0.0;
    std::vector<std::vector<float>> wp;
    wp.push_back(waypoints_[index]);
    for (int i=index+1; i<waypoints_.size(); ++i)
    {
        auto wp0 = waypoints_[index-1];
        auto wp1 = waypoints_[index];
        dist += distance(wp0[4], wp0[5], wp1[4], wp1[5]);
        
        // Return all waypoints here if distance exceeds the maximum distance can see
        if (dist > distance_can_see)
            return wp;
        
        wp.push_back(wp1);
    }
    return wp;
}

bool Perception::preparePerceptionMsg(double pos_x, double pos_y, double yaw_ang)
{
    /** TODO List: 
     *    1. find an apropriate range of road condition waypoints & obstacle sets
     *    2. store the found information above into class member variables
     *    3. return true/flase (maybe set a flag indicating if perception prepared correctly)
     * */ 
}

void Perception::publishPerceptionMsg()
{   
    /** TODO List: 
     *    1. Check if the perception info is prepared
     *    2. pack the perception info to be sent using a temp variable
     * */ 


    // Publish a dummy RoadConditionVector to test the pipeline
    hkj_msgs::ObstacleRect dummy_obstacle_1;
    dummy_obstacle_1.obstacle_rectangle_pos_x = {0.0, 1.0, 1.0, 0.0};
    dummy_obstacle_1.obstacle_rectangle_pos_y = {0.0, 0.0, 1.0, 1.0};
    hkj_msgs::ObstacleRect dummy_obstacle_2;
    dummy_obstacle_2.obstacle_rectangle_pos_x = {0.0, 1.0, 1.0, 0.0};
    dummy_obstacle_2.obstacle_rectangle_pos_y = {2.0, 2.0, 3.0, 3.0};

    hkj_msgs::RoadCondition dummy_road_condition_1;
    dummy_road_condition_1.left_wp  = {0.0, 0.0};
    dummy_road_condition_1.mid_wp   = {1.0, 0.0};
    dummy_road_condition_1.right_wp = {2.0, 0.0};
    hkj_msgs::RoadCondition dummy_road_condition_2;
    dummy_road_condition_2.left_wp  = {0.0, 4.0};
    dummy_road_condition_2.mid_wp   = {1.0, 4.0};
    dummy_road_condition_2.right_wp = {2.0, 4.0};
    hkj_msgs::RoadCondition dummy_road_condition_3;
    dummy_road_condition_3.left_wp  = {0.0, 8.0};
    dummy_road_condition_3.mid_wp   = {1.0, 8.0};
    dummy_road_condition_3.right_wp = {2.0, 8.0};

    hkj_msgs::RoadConditionVector dummy_perception_msg;
    dummy_perception_msg.obstacle_rectangles.push_back(dummy_obstacle_1);
    dummy_perception_msg.obstacle_rectangles.push_back(dummy_obstacle_2);
    dummy_perception_msg.road_condition_vt.push_back(dummy_road_condition_1);
    dummy_perception_msg.road_condition_vt.push_back(dummy_road_condition_2);
    dummy_perception_msg.road_condition_vt.push_back(dummy_road_condition_3);

    pub_road_cond_.publish(dummy_perception_msg);
    ROS_INFO("Perception msg has been published!");
}

// clear previous stored perception data
void Perception::clearPerception()
{
    left_bound_wps_.clear();
    right_bound_wps_.clear();
    mid_line_wps_.clear();
}

// clear previous stored vehicle state data
void Perception::clearVehicleState()
{
    received_veh_state_.clear();
}

} /* end of namespace mpc_traj_follower */