#include <cmath>
using std::isnan;
using std::isinf;

#include <fstream>
using std::ofstream;
using std::ifstream;
using std::ios;

#include <iomanip>
using std::defaultfloat;
using std::hexfloat;
using std::setw;
using std::setprecision;

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
using std::ostream;
using std::istream;

#include <random>
using std::minstd_rand0;
using std::normal_distribution;

#include <thread>

#include <sstream>
using std::ostringstream;

#include <string>
using std::string;

#include <vector>
using std::vector;

#include "image_tools/image_set.hxx"
#include "cnn_edge.hxx"
#include "cnn_node.hxx"

#include "stdint.h"

CNN_Edge::CNN_Edge() {
    edge_id = -1;
    exact_id = -1;
    genome_id = -1;

    innovation_number = -1;

    input_node_innovation_number = -1;
    output_node_innovation_number = -1;

    input_node = NULL;
    output_node = NULL;

    needs_initialization = true;
}

CNN_Edge::CNN_Edge(CNN_Node *_input_node, CNN_Node *_output_node, bool _fixed, int _innovation_number) {
    edge_id = -1;
    exact_id = -1;
    genome_id = -1;

    fixed = _fixed;
    innovation_number = _innovation_number;
    disabled = false;
    forward_visited = false;
    reverse_visited = false;

    reverse_filter_x = false;
    reverse_filter_y = false;
    needs_initialization = true;

    input_node = _input_node;
    output_node = _output_node;

    input_node_innovation_number = input_node->get_innovation_number();
    output_node_innovation_number = output_node->get_innovation_number();

    batch_size = _input_node->get_batch_size();

    if (output_node->get_size_x() <= input_node->get_size_x()) {
        filter_x = (input_node->get_size_x() - output_node->get_size_x()) + 1;
    } else {
        reverse_filter_x = true;
        filter_x = (output_node->get_size_x() - input_node->get_size_x()) + 1;
    }

    if (output_node->get_size_y() <= input_node->get_size_y()) {
        filter_y = (input_node->get_size_y() - output_node->get_size_y()) + 1;
    } else {
        reverse_filter_y = true;
        filter_y = (output_node->get_size_y() - input_node->get_size_y()) + 1;
    }

    //cout << "\t\tcreated edge " << innovation_number << " (node " << input_node_innovation_number << " to " << output_node_innovation_number << ") with filter_x: " << filter_x << " (input: " << input_node->get_size_x() << ", output: " << output_node->get_size_x() << ") and filter_y: " << filter_y << " (input: " << input_node->get_size_y() << ", output: " << output_node->get_size_y() << "), reverse filter: " << reverse_filter_x << ", reverse_filter_y: " << reverse_filter_y << endl;

    weights = vector< vector<float> >(filter_y, vector<float>(filter_x, 0.0));
    weight_updates = vector< vector<float> >(filter_y, vector<float>(filter_x, 0.0));
    best_weights = vector< vector<float> >(filter_y, vector<float>(filter_x, 0.0));

    previous_velocity = vector< vector<float> >(filter_y, vector<float>(filter_x, 0.0));
    best_velocity = vector< vector<float> >(filter_y, vector<float>(filter_x, 0.0));
}

CNN_Edge::~CNN_Edge() {
    input_node = NULL;
    output_node = NULL;
}

void parse_vector_2d(vector< vector<float> > &output, istringstream &iss, int filter_x, int filter_y) {
    output.clear();
    output = vector< vector<float> >(filter_y, vector<float>(filter_x));

    int current_x = 0, current_y = 0;

    float val;
    while(iss >> val || !iss.eof()) {
        if (iss.fail()) {
            iss.clear();
            string dummy;
            iss >> dummy;
            continue;
        }

        //cout << "output[" << current_x << "][" << current_y << "]: " << val << endl;
        output.at(current_y).at(current_x) = val;

        current_x++;

        if (current_x >= filter_x) {
            current_x = 0;
            current_y++;
        }
    }
}



#ifdef _MYSQL_
CNN_Edge::CNN_Edge(int _edge_id) {
    edge_id = _edge_id;

    ostringstream query;
    query << "SELECT * FROM cnn_edge WHERE id = " << edge_id;

    mysql_exact_query(query.str());

    MYSQL_RES *result = mysql_store_result(exact_db_conn);

    if (result != NULL) {
        MYSQL_ROW row = mysql_fetch_row(result);

        int column = 0;

        exact_id = atoi(row[++column]);
        genome_id = atoi(row[++column]);
        innovation_number = atoi(row[++column]);

        input_node_innovation_number = atoi(row[++column]);
        output_node_innovation_number = atoi(row[++column]);

        batch_size = atoi(row[++column]);
        filter_x = atoi(row[++column]);
        filter_y = atoi(row[++column]);

        istringstream weights_iss(row[++column]);
        parse_vector_2d(weights, weights_iss, filter_x, filter_y);

        istringstream best_weights_iss(row[++column]);
        parse_vector_2d(best_weights, best_weights_iss, filter_x, filter_y);

        fixed = atoi(row[++column]);
        disabled = atoi(row[++column]);
        forward_visited = atoi(row[++column]);
        reverse_visited = atoi(row[++column]);
        reverse_filter_x = atoi(row[++column]);
        reverse_filter_y = atoi(row[++column]);
        needs_initialization = atoi(row[++column]);

        mysql_free_result(result);
    } else {
        cerr << "ERROR! Could not find cnn_edge in database with edge id: " << edge_id << endl;
        exit(1);
    }

    weight_updates = vector< vector<float> >(filter_y, vector<float>(filter_x, 0.0));
    previous_velocity = vector< vector<float> >(filter_y, vector<float>(filter_x, 0.0));
    best_velocity = vector< vector<float> >(filter_y, vector<float>(filter_x, 0.0));

    //cout << "read edge!" << endl;
    //cout << this << endl;
}

void CNN_Edge::export_to_database(int _exact_id, int _genome_id) {
    ostringstream query;

    genome_id = _genome_id;
    exact_id = _exact_id;

    //cout << "inserting edge with exact_id: " << exact_id << " and genome id: " << genome_id << endl;

    if (edge_id >= 0) {
        query << "REPLACE INTO cnn_edge SET id = " << edge_id << ",";
    } else {
        query << "INSERT INTO cnn_edge SET";
    }

    query << " exact_id = " << exact_id
        << ", genome_id = " << genome_id
        << ", innovation_number = " << innovation_number
        << ", input_node_innovation_number = " << input_node_innovation_number
        << ", output_node_innovation_number = " << output_node_innovation_number
        << ", batch_size = " << batch_size
        << ", filter_x = " << filter_x
        << ", filter_y = " << filter_y
        << ", fixed = " << fixed
        << ", disabled = " << disabled
        << ", forward_visited = " << forward_visited
        << ", reverse_visited = " << reverse_visited
        << ", reverse_filter_x = " << reverse_filter_x
        << ", reverse_filter_y = " << reverse_filter_y
        << ", needs_initialization = " << needs_initialization
        << ", weights = '";

    for (int32_t y = 0; y < filter_y; y++) {
        for (int32_t x = 0; x < filter_x; x++) {
            if (x != 0) query << " ";
            query << setprecision(15) << weights[y][x];
        }
        if (y != filter_y - 1) query << "\n";
    }

    query << "', best_weights = '";
    for (int32_t y = 0; y < filter_y; y++) {
        for (int32_t x = 0; x < filter_x; x++) {
            if (x != 0) query << " ";
            query << setprecision(15) << best_weights[y][x];
        }
        if (y != filter_y - 1) query << "\n";
    }
    query << "'";

    /*
    query << "', previous_velocity = '";
    for (int32_t y = 0; y < filter_y; y++) {
        for (int32_t x = 0; x < filter_x; x++) {
            if (x != 0) query << " ";
            query << setprecision(15) << previous_velocity[y][x];
        }
        if (y != filter_y - 1) query << "\n";
    }

    query << "', best_velocity = '";
    for (int32_t y = 0; y < filter_y; y++) {
        for (int32_t x = 0; x < filter_x; x++) {
            if (x != 0) query << " ";
            query << setprecision(15) << best_velocity[y][x];
        }
        if (y != filter_y - 1) query << "\n";
    }
    query << "'";
    */
    //query << "', previous_velocity = '', best_velocity = ''";

    mysql_exact_query(query.str());

    if (edge_id < 0) {
        edge_id = mysql_exact_last_insert_id();
        //cout << "set edge id to " << edge_id << endl;
    }
}

int CNN_Edge::get_edge_id() const {
    return edge_id;
}
#endif

bool CNN_Edge::equals(CNN_Edge *other) const {
    return filter_x == other->filter_x && filter_y == other->filter_y && disabled == other->disabled && reverse_filter_x == other->reverse_filter_x && reverse_filter_y == other->reverse_filter_y;
}

bool CNN_Edge::needs_init() const {
    return needs_initialization;
}

void CNN_Edge::set_needs_init() {
    needs_initialization = true;
}


void CNN_Edge::reset_times() {
    propagate_forward_time = 0.0;
    propagate_backward_time = 0.0;
    weight_update_time = 0.0;
}

void CNN_Edge::accumulate_times(float &total_forward_time, float &total_backward_time, float &total_weight_update_time) {
    total_forward_time += propagate_forward_time;
    total_backward_time += propagate_backward_time;
    total_weight_update_time += weight_update_time;
}

int CNN_Edge::get_filter_x() const {
    return filter_x;
}

int CNN_Edge::get_filter_y() const {
    return filter_y;
}

bool CNN_Edge::is_reverse_filter_x() const {
    return reverse_filter_x;
}

bool CNN_Edge::is_reverse_filter_y() const {
    return reverse_filter_y;
}

void CNN_Edge::propagate_weight_count() {
    output_node->add_weight_count(filter_x * filter_y);
}

void CNN_Edge::initialize_weights(minstd_rand0 &generator, NormalDistribution &normal_distribution) {
    int edge_size = output_node->get_weight_count();
    if (edge_size == 0) {
        cerr << "ERROR! Initializing weights on an edge when node weight counts have not yet been set!" << endl;
        exit(1);
    }

    float mu = 0.0;
    float sigma = sqrt(2.0 / edge_size);

    for (uint32_t i = 0; i < weights.size(); i++) {
        for (uint32_t j = 0; j < weights[i].size(); j++) {
            weights[i][j] = normal_distribution.random(generator, mu, sigma);
            best_weights[i][j] = 0.0;
            previous_velocity[i][j] = 0.0;
        }
    }
    //cout << "initialized weights for edge " << innovation_number << ", weights[0][0]: " << weights[0][0] << endl;

    needs_initialization = false;
}

void CNN_Edge::reset_velocities() {
    for (uint32_t y = 0; y < weights.size(); y++) {
        for (uint32_t x = 0; x < weights[y].size(); x++) {
            previous_velocity[y][x] = 0.0;
        }
    }
}

void CNN_Edge::resize() {
    //this may have changed from a regular to reverse filter
    if (output_node->get_size_x() <= input_node->get_size_x()) {
        reverse_filter_x = false;
        filter_x = (input_node->get_size_x() - output_node->get_size_x()) + 1;
    } else {
        reverse_filter_x = true;
        filter_x = (output_node->get_size_x() - input_node->get_size_x()) + 1;
    }

    if (output_node->get_size_y() <= input_node->get_size_y()) {
        reverse_filter_y = false;
        filter_y = (input_node->get_size_y() - output_node->get_size_y()) + 1;
    } else {
        reverse_filter_y = true;
        filter_y = (output_node->get_size_y() - input_node->get_size_y()) + 1;
    }

    weights = vector< vector<float> >(filter_y, vector<float>(filter_x, 0.0));
    weight_updates = vector< vector<float> >(filter_y, vector<float>(filter_x, 0.0));
    best_weights = vector< vector<float> >(filter_y, vector<float>(filter_x, 0.0));

    previous_velocity = vector< vector<float> >(filter_y, vector<float>(filter_x, 0.0));
    best_velocity = vector< vector<float> >(filter_y, vector<float>(filter_x, 0.0));

    needs_initialization = true;
}

void CNN_Edge::save_best_weights() {
    for (int32_t y = 0; y < filter_y; y++) {
        for (int32_t x = 0; x < filter_x; x++) {
            best_weights[y][x] = weights[y][x];
            best_velocity[y][x] = previous_velocity[y][x];
        }
    }
}

void CNN_Edge::set_weights_to_best() {
    /*
    if (filter_y != weights.size()) {
        cerr << "ERROR! edge filter_x != weights.size(): " << filter_x << " vs " << weights.size() << endl;
        exit(1);
    }

    if (filter_x != weights.size()) {
        cerr << "ERROR! edge filter_x != weights.size(): " << filter_x << " vs " << weights.size() << endl;
        exit(1);
    }
    */

    for (int32_t y = 0; y < filter_y; y++) {
        for (int32_t x = 0; x < filter_x; x++) {
            weights[y][x] = best_weights[y][x];
            //previous_velocity[y][x] = best_velocity[y][x];
            previous_velocity[y][x] = 0;
        }
    }
}


CNN_Edge* CNN_Edge::copy() const {
    CNN_Edge* copy = new CNN_Edge();

    copy->edge_id = -1;
    copy->genome_id = genome_id;

    copy->fixed = fixed;
    copy->innovation_number = innovation_number;
    copy->disabled = disabled;
    copy->forward_visited = forward_visited;
    copy->reverse_visited = reverse_visited;

    copy->input_node = input_node;
    copy->output_node = output_node;

    copy->input_node_innovation_number = input_node->get_innovation_number();
    copy->output_node_innovation_number = output_node->get_innovation_number();

    copy->batch_size = batch_size;
    copy->filter_x = filter_x;
    copy->filter_y = filter_y;

    copy->reverse_filter_x = reverse_filter_x;
    copy->reverse_filter_y = reverse_filter_y;
    copy->needs_initialization = needs_initialization;

    copy->weights = weights;
    copy->weight_updates = weight_updates;
    copy->best_weights = best_weights;
    copy->previous_velocity = previous_velocity;
    copy->best_velocity = best_velocity;

    return copy;
}

bool CNN_Edge::set_nodes(const vector<CNN_Node*> nodes) {
    //cout << "nodes.size(): " << nodes.size() << endl;
    //cout << "setting input node: " << input_node_innovation_number << endl;
    //cout << "setting output node: " << output_node_innovation_number << endl;

    input_node = NULL;
    output_node = NULL;

    for (uint32_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->get_innovation_number() == input_node_innovation_number) {
            input_node = nodes[i];
        }

        if (nodes[i]->get_innovation_number() == output_node_innovation_number) {
            output_node = nodes[i];
        }
    }

    if (input_node == NULL) {
        cerr << "ERROR! Could not find node with input node innovation number " << input_node_innovation_number << endl;
        cerr << "This should never happen!" << endl;
        cerr << "nodes innovation numbers:" << endl;
        for (uint32_t i = 0; i < nodes.size(); i++) {
            cerr << "\t" << nodes[i]->get_innovation_number() << endl;
        }
        exit(1);
    }

    if (output_node == NULL) {
        cerr << "ERROR! Could not find node with output node innovation number " << output_node_innovation_number << endl;
        cerr << "This should never happen!" << endl;
        cerr << "nodes innovation numbers:" << endl;
        for (uint32_t i = 0; i < nodes.size(); i++) {
            cerr << "\t" << nodes[i]->get_innovation_number() << endl;
        }
        exit(1);
    }

    if (output_node == input_node) {
        cerr << "ERROR! Setting nodes and output_node == input_node!" << endl;
        cerr << "input node innovation number: " << input_node_innovation_number << endl;
        cerr << "output node innovation number: " << output_node_innovation_number << endl;
        cerr << "This should never happen!" << endl;
        exit(1);
    }

    if (!is_filter_correct()) {
        return false;
    }

    return true;
}

bool CNN_Edge::is_filter_correct() const {
    //cout << "\t\tchecking filter correctness on edge: " << innovation_number << endl;
    //cout << "\t\t\tdisabled? " << disabled << endl;
    //cout << "\t\t\treverse_filter_x? " << reverse_filter_x << ", reverse_filter_y: " << reverse_filter_y << endl;
    //cout << "\t\t\tbetween node " << input_node_innovation_number << " and " << output_node_innovation_number << endl;

    bool is_correct = true;
    if (reverse_filter_x) {
        //cout << "\t\t\tfilter_x: " << filter_x << ", should be: " << (output_node->get_size_x() - input_node->get_size_x()) + 1 << " (output_x: " << output_node->get_size_x() << " - input_x: " << input_node->get_size_x() << " + 1) " << endl;

        is_correct = is_correct && (filter_x == (output_node->get_size_x() - input_node->get_size_x()) + 1);
    } else {
        //cout << "\t\t\tfilter_x: " << filter_x << ", should be: " << (input_node->get_size_x() - output_node->get_size_x()) + 1 << " (input_x: " << input_node->get_size_x() << " - output_x: " << output_node->get_size_x() << " + 1) " << endl;

        is_correct = is_correct && (filter_x == (input_node->get_size_x() - output_node->get_size_x()) + 1);
    }

    if (reverse_filter_y) {
        //cout << "\t\t\tfilter_y: " << filter_y << ", should be: " << (output_node->get_size_y() - input_node->get_size_y()) + 1 << " (output_y: " << output_node->get_size_y() << " - input_y: " << input_node->get_size_y() << " + 1) " << endl;

        is_correct = is_correct && (filter_y == (output_node->get_size_y() - input_node->get_size_y()) + 1);
    } else {
        //cout << "\t\t\tfilter_y: " << filter_y << ", should be: " << (input_node->get_size_y() - output_node->get_size_y()) + 1 << " (input_y: " << input_node->get_size_y() << " - output_y: " << output_node->get_size_y() << " + 1) " << endl;

        is_correct = is_correct && (filter_y == (input_node->get_size_y() - output_node->get_size_y()) + 1);
    }

    return is_correct;
}

void CNN_Edge::update_batch_size(int new_batch_size) {
    batch_size = new_batch_size;
}

void CNN_Edge::enable() {
    if (is_reachable()) {
        disabled = false;
    }
}

void CNN_Edge::disable() {
    if (!is_reachable()) {
        disabled = true;
    }
}

bool CNN_Edge::is_disabled() const {
    return disabled;
}

bool CNN_Edge::is_reachable() const {
    return !disabled && forward_visited && reverse_visited;
}

bool CNN_Edge::is_forward_visited() const {
    return forward_visited;
}

bool CNN_Edge::is_reverse_visited() const {
    return reverse_visited;
}

void CNN_Edge::forward_visit() {
    forward_visited = true;
}

void CNN_Edge::reverse_visit() {
    reverse_visited = true;
}

void CNN_Edge::set_unvisited() {
    forward_visited = false;
    reverse_visited = false;
}


int CNN_Edge::get_number_weights() const {
    return filter_x * filter_y;
}

int CNN_Edge::get_batch_size() const {
    return batch_size;
}


int CNN_Edge::get_innovation_number() const {
    return innovation_number;
}

int CNN_Edge::get_input_innovation_number() const {
    return input_node_innovation_number;
}

int CNN_Edge::get_output_innovation_number() const {
    return output_node_innovation_number;
}


CNN_Node* CNN_Edge::get_input_node() {
    return input_node;
}

CNN_Node* CNN_Edge::get_output_node() {
    return output_node;
}

bool CNN_Edge::connects(int n1, int n2) const {
    return (input_node_innovation_number == n1) && (output_node_innovation_number == n2);
}

bool CNN_Edge::has_zero_weight() const {
    if (is_reachable()) return false;

    float filter_sum = 0.0;
    for (int32_t fy = 0; fy < filter_y; fy++) {
        for (int32_t fx = 0; fx < filter_x; fx++) {
            filter_sum += (weights[fy][fx] * weights[fy][fx]);
        }
    }

    return filter_sum == 0;
}

bool CNN_Edge::has_zero_best_weight() const {
    if (is_reachable()) return false;

    float filter_sum = 0.0;
    for (int32_t fy = 0; fy < filter_y; fy++) {
        for (int32_t fx = 0; fx < filter_x; fx++) {
            filter_sum += (best_weights[fy][fx] * best_weights[fy][fx]);
        }
    }

    return filter_sum == 0;
}



void CNN_Edge::print(ostream &out) {
    out << "CNN_Edge " << innovation_number << " of from node " << input_node->get_innovation_number() << " to node " << output_node->get_innovation_number() << " with filter x: " << filter_x << ", y: " << filter_y << endl;

    for (uint32_t i = 0; i < weights.size(); i++) {
        out << "    ";
        for (uint32_t j = 0; j < weights[i].size(); j++) {
            out << setw(9) << fixed << setprecision(3) << weights[i][j];
        }
        out << endl;
    }

    for (uint32_t i = 0; i < previous_velocity.size(); i++) {
        out << "    ";
        for (uint32_t j = 0; j < previous_velocity[i].size(); j++) {
            out << setw(9) << fixed << setprecision(3) << previous_velocity[i][j];
        }
        out << endl;
    }
}

void CNN_Edge::check_output_update(const vector< vector< vector<float> > > &output, const vector< vector< vector<float> > > &input, float value, float weight, float previous_output, int batch_number, int in_y, int in_x, int out_y, int out_x) {
    if (isnan(output[batch_number][out_y][out_x]) || isinf(output[batch_number][out_y][out_x])) {
        cerr << "ERROR in edge " << innovation_number << " propagate forward!" << endl;
        cerr << "input node innovation number: " << input_node->get_innovation_number() << " at depth: " << input_node->get_depth() << endl;
        cerr << "input node inputs fired: " << input_node->get_inputs_fired() << ", total_inputs: " << input_node->get_number_inputs() << endl;
        cerr << "output node innovation number: " << output_node->get_innovation_number() << " at depth: " << output_node->get_depth() << endl;
        cerr << "output became: " << output[batch_number][out_y][out_x] << "!" << endl;
        cerr << "output[" << batch_number << "][" << out_y << "][" << out_x << "] = " << output[batch_number][out_y][out_x] << endl;
        cerr << "input[" << batch_number << "][" << in_y << "][" << in_x << "] = " << input[batch_number][in_y][in_x] << endl;
        cerr << "weight: " << weight << endl;
        cerr << "previous output: " << previous_output << endl;
        cerr << "value added: " << value << endl;

        input_node->print(cerr);
        output_node->print(cerr);

        exit(1);
    }
}

void CNN_Edge::propagate_forward(bool training, bool accumulate_test_statistics, float epsilon, float alpha, bool perform_dropout, float hidden_dropout_probability, minstd_rand0 &generator) {
    if (!is_reachable()) return;

    float propagate_forward_start_time = time(NULL);

    vector< vector< vector<float> > > &input = input_node->get_values_out();
    vector< vector< vector<float> > > &output = output_node->get_values_in();

#ifdef NAN_CHECKS
    if (!is_filter_correct()) {
        cerr << "ERROR: filter_x != input_node->get_size_x: " << input_node->get_size_x() << " - output_node->get_size_x: " << output_node->get_size_x() << " + 1" << endl;
        exit(1);
    }

    float previous_output;
#endif

    int output_size_x = output_node->get_size_x();
    int output_size_y = output_node->get_size_y();
    int input_size_x = input_node->get_size_x();
    int input_size_y = input_node->get_size_y();

    if (reverse_filter_x && reverse_filter_y) {
        for (int32_t batch_number = 0; batch_number < batch_size; batch_number++) {
            for (int32_t fy = 0; fy < filter_y; fy++) {
                for (int32_t fx = 0; fx < filter_x; fx++) {
                    float weight = weights[fy][fx];

                    for (int32_t y = 0; y < input_size_y; y++) {
                        for (int32_t x = 0; x < input_size_x; x++) {
                            float value = weight * input[batch_number][y][x];
#ifdef NAN_CHECKS
                            previous_output = output[batch_number][y + fy][x + fx];
#endif
                            output[batch_number][y + fy][x + fx] += value;
#ifdef NAN_CHECKS
                            check_output_update(output, input, value, weight, previous_output, batch_number, y, x, y + fy, x + fx);
#endif
                        }
                    }
                }
            }
        }

    } else if (reverse_filter_x) {
        for (int32_t batch_number = 0; batch_number < batch_size; batch_number++) {
            for (int32_t fy = 0; fy < filter_y; fy++) {
                for (int32_t fx = 0; fx < filter_x; fx++) {
                    float weight = weights[fy][fx];

                    for (int32_t y = 0; y < output_size_y; y++) {
                        for (int32_t x = 0; x < input_size_x; x++) {
                            float value = weight * input[batch_number][y + fy][x];
#ifdef NAN_CHECKS
                            previous_output = output[batch_number][y][x + fx];
#endif
                            output[batch_number][y][x + fx] += value;
#ifdef NAN_CHECKS
                            check_output_update(output, input, value, weight, previous_output, batch_number, y + fy, x, y, x + fx);
#endif
                        }
                    }
                }
            }
        }

    } else if (reverse_filter_y) {
        for (int32_t batch_number = 0; batch_number < batch_size; batch_number++) {
            for (int32_t fy = 0; fy < filter_y; fy++) {
                for (int32_t fx = 0; fx < filter_x; fx++) {
                    float weight = weights[fy][fx];

                    for (int32_t y = 0; y < input_size_y; y++) {
                        for (int32_t x = 0; x < output_size_x; x++) {
                            float value = weight * input[batch_number][y][x + fx];
#ifdef NAN_CHECKS
                            previous_output = output[batch_number][y + fy][x];
#endif
                            output[batch_number][y + fy][x] += value;
#ifdef NAN_CHECKS
                            check_output_update(output, input, value, weight, previous_output, batch_number, y, x + fx, y + fy, x);
#endif
                        }
                    }
                }
            }
        }

    } else {
        for (int32_t batch_number = 0; batch_number < batch_size; batch_number++) {
            for (int32_t fy = 0; fy < filter_y; fy++) {
                for (int32_t fx = 0; fx < filter_x; fx++) {
                    float weight = weights[fy][fx];

                    for (int32_t y = 0; y < output_size_y; y++) {
                        for (int32_t x = 0; x < output_size_x; x++) {
                            float value = weight * input[batch_number][y + fy][x + fx];
#ifdef NAN_CHECKS
                            previous_output = output[batch_number][y][x];
#endif
                            output[batch_number][y][x] += value;
#ifdef NAN_CHECKS
                            check_output_update(output, input, value, weight, previous_output, batch_number, y + fy, x + fx, y, x);
#endif
                        }
                    }
                }
            }
        }
    }

    propagate_forward_time += time(NULL) - propagate_forward_start_time;
	output_node->input_fired(training, accumulate_test_statistics, epsilon, alpha, perform_dropout, hidden_dropout_probability, generator);
}

void CNN_Edge::update_weights(float mu, float learning_rate, float weight_decay) {
    if (!is_reachable()) return;

    float weight_update_start_time = time(NULL);

    float dx, pv, velocity, weight;
#ifdef NAN_CHECKS
    float previous_weight;
#endif

	for (int32_t fy = 0; fy < filter_y; fy++) {
        for (int32_t fx = 0; fx < filter_x; fx++) {
            dx = weight_updates[fy][fx];

            //try clipping the weight
            //dx = dx * (0.5 / fmax(0.5, fabs(dx)));

            pv = previous_velocity[fy][fx];

            velocity = (mu * pv) - learning_rate * dx;

            weight = weights[fy][fx];
#ifdef NAN_CHECKS
            previous_weight = weight;
#endif
            weight += velocity + mu * (velocity - pv);
            //weight += (-mu * pv + (1 + mu) * velocity);
            //weight += velocity;

            weight -= (weight * weight_decay);

            weights[fy][fx] = weight;

            previous_velocity[fy][fx] = velocity;

#ifdef NAN_CHECKS
            if (isnan(weights[fy][fx]) || isinf(weights[fy][fx])) {
                cerr << "ERROR! weight became " << weights[fy][fx] << " in edge: " << innovation_number << " (" << input_node_innovation_number << " to " << output_node_innovation_number << ")" << endl;
                cerr << "\tdx: " << dx << endl;
                cerr << "\tpv: " << pv << endl;
                cerr << "\tvelocity: " << velocity << endl;
                cerr << "\tprevious_weight: " << previous_weight << endl;
                exit(1);
            }
#endif

            if (weights[fy][fx] > 50.0) {
                /*
                cout << "weight > 2!" << endl;
                cout << "updating weight from " << input_node_innovation_number << " to " << output_node_innovation_number
                    << ": fy: " << fy << ", fx: " << fx 
                    << ", weight: " << weights[fy][fx] 
                    << ", weight_update: " << weight_updates[fy][fx] 
                    << ", learning_rate * dx: " << (learning_rate * dx) << endl;

                this->print(cout);
                input_node->print(cout);
                output_node->print(cout);

                exit(1);
                */

                weights[fy][fx] = 50.0;
                previous_velocity[fy][fx] = 0.0;
            } else if (weights[fy][fx] < -50.0) {
                /*
                cout << "weight < -2!" << endl;
                cout << "updating weight from " << input_node_innovation_number << " to " << output_node_innovation_number
                    << ": fy: " << fy << ", fx: " << fx 
                    << ", weight: " << weights[fy][fx] 
                    << ", weight_update: " << weight_updates[fy][fx] 
                    << ", learning_rate * dx: " << (learning_rate * dx) << endl;
                this->print(cout);
                input_node->print(cout);
                output_node->print(cout);

                exit(1);
                */

                weights[fy][fx] = -50.0;
                previous_velocity[fy][fx] = 0.0;
            }
        }
    }

    weight_update_time += time(NULL) - weight_update_start_time;
}

void CNN_Edge::check_weight_update(const vector< vector< vector<float> > > &input, const vector< vector< vector<float> > > &input_errors, float error, float previous_error, float weight_update, float previous_weight_update, int batch_number, int out_y, int out_x, int in_y, int in_x) {
    if (isnan(weight_update) || isinf(weight_update)) {
        cerr << "ERROR weight_update became " << weight_update << " in edge " << innovation_number << " (" << input_node_innovation_number << " to " << output_node_innovation_number << ")!" << endl;
        cerr << "\tprevious_weight_udpate: " << previous_weight_update << endl;
        cerr << "\terror: " << error << endl;
        cerr << "\tinput: " << input[batch_number][in_y][in_x] << endl;

        cerr << endl << "input_node: " << endl;
        input_node->print(cerr);

        cerr << endl << "output_node: " << endl;
        output_node->print(cerr);

        exit(1);
    }

    if (isnan(input_errors[batch_number][in_y][in_x]) || isinf(input_errors[batch_number][in_y][in_x])) {
        cerr << "ERROR input_error became " << input_errors[batch_number][in_y][in_x] << " in edge " << innovation_number << " (" << input_node_innovation_number << " to " << output_node_innovation_number << ")!" << endl;
        cerr << "\tprevious_error: " << previous_error << endl;
        cerr << "\terror: " << error << endl;
        cerr << "\tinput: " << input[batch_number][in_y][in_x] << endl;

        cerr << endl << "input_node: " << endl;
        input_node->print(cerr);

        cerr << endl << "output_node: " << endl;
        output_node->print(cerr);

        exit(1);
    }
}

void CNN_Edge::propagate_backward(float mu, float learning_rate, float epsilon) {
    if (!is_reachable()) return;

    float propagate_backward_start_time = time(NULL);

    vector< vector< vector<float> > > &output_errors = output_node->get_errors_in();

    vector< vector< vector<float> > > &input = input_node->get_values_out();
    vector< vector< vector<float> > > &input_errors = input_node->get_errors_out();

    float weight, weight_update, delta;

    int in_x = input_node->get_size_x();
    int in_y = input_node->get_size_y();

    int out_x = output_node->get_size_x();
    int out_y = output_node->get_size_y();

    for (int32_t fy = 0; fy < filter_y; fy++) {
        for (int32_t fx = 0; fx < filter_x; fx++) {
            weight_updates[fy][fx] = 0;
        }
    }

    if (reverse_filter_x && reverse_filter_y) {
        //cout << "reverse filter x and y!" << endl;

        for (int32_t batch_number = 0; batch_number < batch_size; batch_number++) {
            for (int32_t fy = 0; fy < filter_y; fy++) {
                for (int32_t fx = 0; fx < filter_x; fx++) {
                    weight_update = 0.0;
                    weight = weights[fy][fx];

                    for (int32_t y = 0; y < in_y; y++) {
                        for (int32_t x = 0; x < in_x; x++) {
                            delta = output_errors[batch_number][y + fy][x + fx];
#ifdef NAN_CHECKS                        
                            float previous_weight_update = weight_update;
                            float previous_error = input_errors[batch_number][y][x];
#endif
                            weight_update += input[batch_number][y][x] * delta;
                            input_errors[batch_number][y][x] += delta * weight;
#ifdef NAN_CHECKS
                            check_weight_update(input, input_errors, delta, previous_error, weight_update, previous_weight_update, batch_number, y + fy, x + fx, y, x);
#endif
                        }
                    }
                    weight_updates[fy][fx] += weight_update;
                }
            }
        }

    } else if (reverse_filter_x) {
        //cout << "reverse filter x!" << endl;

        for (int32_t batch_number = 0; batch_number < batch_size; batch_number++) {
            for (int32_t fy = 0; fy < filter_y; fy++) {
                for (int32_t fx = 0; fx < filter_x; fx++) {
                    weight_update = 0;
                    weight = weights[fy][fx];

                    for (int32_t y = 0; y < out_y; y++) {
                        for (int32_t x = 0; x < in_x; x++) {
                            delta = output_errors[batch_number][y][x + fx];
#ifdef NAN_CHECKS                        
                            float previous_weight_update = weight_update;
                            float previous_error = input_errors[batch_number][y][x];
#endif
                            weight_update += input[batch_number][y + fy][x] * delta;
                            input_errors[batch_number][y + fy][x] += delta * weight;
#ifdef NAN_CHECKS
                            check_weight_update(input, input_errors, delta, previous_error, weight_update, previous_weight_update, batch_number, y, x + fx, y + fy, x);
#endif
                        }
                    }
                    weight_updates[fy][fx] = weight_update;
                }
            }
        }

    } else if (reverse_filter_y) {
        /*
        cout << "reverse filter y!" << endl;

        std::thread::id this_id = std::this_thread::get_id();
        //cout << "thread '" << this_id << "' -- INN: " << input_node_innovation_number << "ONN: " << output_node_innovation_number << ", in null? " << in_null << ", out null? " << out_null << endl;
        cout << "thread '" << this_id << "' -- in_y: " << in_y << ", in_x: " << in_x << endl;
        cout << "thread '" << this_id << "' -- out_y: " << out_y << ", out_x: " << out_x << endl;
        cout << "thread '" << this_id << "' -- filter_y: " << filter_y << ", filter_x: " << filter_x << endl;
        cout << "thread '" << this_id << "' -- weights.size(): " << weights.size() << " (should be filter_y), weights[0].size(): " << weights[0].size() << " (should be filter_x)" << endl;
        */


        for (int32_t batch_number = 0; batch_number < batch_size; batch_number++) {
            for (int32_t fy = 0; fy < filter_y; fy++) {
                for (int32_t fx = 0; fx < filter_x; fx++) {
                    weight_update = 0;
                    weight = weights[fy][fx];

                    //cout << "thread '" << this_id << "' -- looping fy: " << fy << ", fx: "<< fx << "!" << endl;

                    for (int32_t y = 0; y < in_y; y++) {
                        for (int32_t x = 0; x < out_x; x++) {
                            //cout << "thread '" << this_id << "' -- setting output[" << y + fy << "][" << x << "]" << endl;

                            delta = output_errors[batch_number][y + fy][x];
#ifdef NAN_CHECKS                        
                            float previous_weight_update = weight_update;
                            float previous_error = input_errors[batch_number][y][x];
#endif
                            //cout << "thread '" << this_id << "' -- setting input[" << y << "][" << x + fx << "]" << endl;

                            weight_update += input[batch_number][y][x + fx] * delta;
                            input_errors[batch_number][y][x + fx] += delta * weight;

#ifdef NAN_CHECKS
                            check_weight_update(input, input_errors, delta, previous_error, weight_update, previous_weight_update, batch_number, y + fy, x, y, x + fx);
#endif
                        }
                    }
                    weight_updates[fy][fx] += weight_update;
                }
            }
        }

    } else {
        //cout << "no reverse filter!" << endl;


        for (int32_t batch_number = 0; batch_number < batch_size; batch_number++) {
            for (int32_t fy = 0; fy < filter_y; fy++) {
                for (int32_t fx = 0; fx < filter_x; fx++) {
                    weight_update = 0;
                    weight = weights[fy][fx];

                    for (int32_t y = 0; y < out_y; y++) {
                        for (int32_t x = 0; x < out_x; x++) {
                            delta = output_errors[batch_number][y][x];
#ifdef NAN_CHECKS                        
                            float previous_weight_update = weight_update;
                            float previous_error = input_errors[batch_number][y][x];
#endif

                            weight_update += input[batch_number][y + fy][x + fx] * delta;
                            input_errors[batch_number][y + fy][x + fx] += delta * weight;

#ifdef NAN_CHECKS
                            check_weight_update(input, input_errors, delta, previous_error, weight_update, previous_weight_update, batch_number, y, x, y + fy, x + fx);
#endif
                        }
                    }
                    weight_updates[fy][fx] += weight_update;
                }
            }
        }
    }

    propagate_backward_time += time(NULL) - propagate_backward_start_time;

    input_node->output_fired(mu, learning_rate, epsilon);
}

bool CNN_Edge::has_nan() const {
    for (uint32_t y = 0; y < filter_y; y++) {
        for (uint32_t x = 0; x < filter_x; x++) {
            if (isnan(weights[y][x]) || isinf(weights[y][x])) return true;
            if (isnan(weight_updates[y][x]) || isinf(weight_updates[y][x])) return true;
            if (isnan(previous_velocity[y][x]) || isinf(previous_velocity[y][x])) return true;
        }
    }
    return false;
}

void CNN_Edge::print_statistics() {
    float weight_min = std::numeric_limits<float>::max(), weight_max = -std::numeric_limits<float>::max(), weight_avg = 0.0;
    float weight_update_min = std::numeric_limits<float>::max(), weight_update_max = -std::numeric_limits<float>::max(), weight_update_avg = 0.0;
    float velocity_min = std::numeric_limits<float>::max(), velocity_max = -std::numeric_limits<float>::max(), velocity_avg = 0.0;

    for (int fy = 0; fy < filter_y; fy++) {
        for (int fx = 0; fx < filter_x; fx++) {
            if (weights[fy][fx] < weight_min) weight_min = weights[fy][fx];
            if (weights[fy][fx] > weight_max) weight_max = weights[fy][fx];
            weight_avg += weights[fy][fx];

            if (weight_updates[fy][fx] < weight_update_min) weight_update_min = weight_updates[fy][fx];
            if (weight_updates[fy][fx] > weight_update_max) weight_update_max = weight_updates[fy][fx];
            weight_update_avg += weight_updates[fy][fx];

            if (previous_velocity[fy][fx] < velocity_min) velocity_min = previous_velocity[fy][fx];
            if (previous_velocity[fy][fx] > velocity_max) velocity_max = previous_velocity[fy][fx];
            velocity_avg += previous_velocity[fy][fx];
        }
    }

    velocity_avg /= filter_y * filter_x;
    weight_avg /= filter_y * filter_x;

    cerr << "edge " << setw(4) << innovation_number << " (in: " << setw(4) << input_node_innovation_number << ", out: " << setw(4) << output_node_innovation_number << ") w_min: " << weight_min << ", w_avg: " << weight_avg << ", w_max: " << weight_max << ", wu_min: " << weight_update_min << ", wu_avg: " << weight_update_avg << ", wu_max: " << weight_update_max << ", v_min: " << velocity_min << ", v_avg: " << velocity_avg << ", v_max: " << velocity_max << endl;

}

ostream &operator<<(ostream &os, const CNN_Edge* edge) {
    os << edge->edge_id << " ";
    os << edge->exact_id << " ";
    os << edge->genome_id << " ";
    os << edge->innovation_number << " ";
    os << edge->input_node_innovation_number << " ";
    os << edge->output_node_innovation_number << " ";
    os << edge->filter_x << " ";
    os << edge->filter_y << " ";
    os << edge->fixed << " ";
    os << edge->reverse_filter_x << " ";
    os << edge->reverse_filter_y << " ";
    os << edge->disabled << " ";
    os << edge->forward_visited << " ";
    os << edge->reverse_visited << " ";
    os << edge->needs_initialization << " ";
    os << edge->batch_size << endl;

    os << "WEIGHTS" << endl;

    for (int32_t y = 0; y < edge->filter_y; y++) {
        for (int32_t x = 0; x < edge->filter_x; x++) {
            if (y > 0 || x > 0) os << " ";
            write_hexfloat(os, edge->weights[y][x]);
        }
    }
    os << endl;

    os << "BEST_WEIGHTS" << endl;
    for (int32_t y = 0; y < edge->filter_y; y++) {
        for (int32_t x = 0; x < edge->filter_x; x++) {
            if (y > 0 || x > 0) os << " ";
            write_hexfloat(os, edge->best_weights[y][x]);
        }
    }
    os << endl;

    os << "PREVIOUS_VELOCITY" << endl;
    for (int32_t y = 0; y < edge->filter_y; y++) {
        for (int32_t x = 0; x < edge->filter_x; x++) {
            if (y > 0 || x > 0) os << " ";
            write_hexfloat(os, edge->previous_velocity[y][x]);
        }
    }
    os << endl;

    os << "BEST_VELOCITY" << endl;
    for (int32_t y = 0; y < edge->filter_y; y++) {
        for (int32_t x = 0; x < edge->filter_x; x++) {
            if (y > 0 || x > 0) os << " ";
            write_hexfloat(os, edge->best_velocity[y][x]);
        }
    }

    return os;
}

istream &operator>>(istream &is, CNN_Edge* edge) {
    is >> edge->edge_id;
    is >> edge->exact_id;
    is >> edge->genome_id;
    is >> edge->innovation_number;
    is >> edge->input_node_innovation_number;
    is >> edge->output_node_innovation_number;
    is >> edge->filter_x;
    is >> edge->filter_y;
    is >> edge->fixed;
    is >> edge->reverse_filter_x;
    is >> edge->reverse_filter_y;
    is >> edge->disabled;
    is >> edge->forward_visited;
    is >> edge->reverse_visited;
    is >> edge->needs_initialization;
    is >> edge->batch_size;

    edge->weights = vector< vector<float> >(edge->filter_y, vector<float>(edge->filter_x, 0.0));
    edge->weight_updates = vector< vector<float> >(edge->filter_y, vector<float>(edge->filter_x, 0.0));
    edge->best_weights = vector< vector<float> >(edge->filter_y, vector<float>(edge->filter_x, 0.0));

    edge->previous_velocity = vector< vector<float> >(edge->filter_y, vector<float>(edge->filter_x, 0.0));
    edge->best_velocity = vector< vector<float> >(edge->filter_y, vector<float>(edge->filter_x, 0.0));

    string line;
    getline(is, line);
    getline(is, line);
    if (line.compare("WEIGHTS") != 0) {
        cerr << "ERROR: invalid input file, expected line to be 'WEIGHTS' but line was '" << line << "'" << endl;
        exit(1);
    }

    for (int32_t y = 0; y < edge->filter_y; y++) {
        for (int32_t x = 0; x < edge->filter_x; x++) {
            edge->weights[y][x] = read_hexfloat(is);
        }
    }

    getline(is, line);
    getline(is, line);
    if (line.compare("BEST_WEIGHTS") != 0) {
        cerr << "ERROR: invalid input file, expected line to be 'BEST_WEIGHTS' but line was '" << line << "'" << endl;
        exit(1);
    }

    for (int32_t y = 0; y < edge->filter_y; y++) {
        for (int32_t x = 0; x < edge->filter_x; x++) {
            edge->best_weights[y][x] = read_hexfloat(is);
        }
    }

    getline(is, line);
    getline(is, line);
    if (line.compare("PREVIOUS_VELOCITY") != 0) {
        cerr << "ERROR: invalid input file, expected line to be 'PREVIOUS_VELOCITY' but line was '" << line << "'" << endl;
        exit(1);
    }

    for (int32_t y = 0; y < edge->filter_y; y++) {
        for (int32_t x = 0; x < edge->filter_x; x++) {
            edge->previous_velocity[y][x] = read_hexfloat(is);
        }
    }

    getline(is, line);
    getline(is, line);
    if (line.compare("BEST_VELOCITY") != 0) {
        cerr << "ERROR: invalid input file, expected line to be 'BEST_VELOCITY' but line was '" << line << "'" << endl;
        exit(1);
    }

    for (int32_t y = 0; y < edge->filter_y; y++) {
        for (int32_t x = 0; x < edge->filter_x; x++) {
            edge->best_velocity[y][x] = read_hexfloat(is);
        }
    }

    return is;
}
