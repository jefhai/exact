#include <cmath>

#include <fstream>
using std::ofstream;

#include <iomanip>
using std::setw;

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <random>
using std::minstd_rand0;
using std::uniform_real_distribution;

#include <thread>
using std::thread;

#include <random>
using std::minstd_rand0;
using std::uniform_real_distribution;

#include <sstream>
using std::ostringstream;

#include <string>
using std::string;

#include <vector>
using std::vector;

#include "common/random.hxx"

#include "rnn.hxx"
#include "rnn_node.hxx"
#include "lstm_node.hxx"
#include "rnn_genome.hxx"

void RNN_Genome::sort_nodes_by_depth() {
    sort(nodes.begin(), nodes.end(), sort_RNN_Nodes_by_depth());
}

void RNN_Genome::sort_edges_by_depth() {
    sort(edges.begin(), edges.end(), sort_RNN_Edges_by_depth());
}

RNN_Genome::RNN_Genome(vector<RNN_Node_Interface*> &_nodes, vector<RNN_Edge*> &_edges) {
    generation_id = -1;
    best_validation_error = EXALT_MAX_DOUBLE;

    nodes = _nodes;
    edges = _edges;

    sort_nodes_by_depth();
    sort_edges_by_depth();

    //set default values
    bp_iterations = 20000;
    learning_rate = 0.001;
    adapt_learning_rate = false;
    use_nesterov_momentum = true;
    use_reset_weights = false;

    use_high_norm = true;
    high_threshold = 1.0;
    use_low_norm = true;
    low_threshold = 0.05;

    use_dropout = false;
    dropout_probability = 0.5;

    log_filename = "";

    uint16_t seed = std::chrono::system_clock::now().time_since_epoch().count();
    generator = minstd_rand0(seed);
    rng_0_1 = uniform_real_distribution<double>(0.0, 1.0);

    assign_reachability();
}

RNN_Genome::RNN_Genome(vector<RNN_Node_Interface*> &_nodes, vector<RNN_Edge*> &_edges, vector<RNN_Recurrent_Edge*> &_recurrent_edges) : RNN_Genome(_nodes, _edges) {
    recurrent_edges = _recurrent_edges;
    assign_reachability();
}

RNN_Genome::RNN_Genome(vector<RNN_Node_Interface*> &_nodes, vector<RNN_Edge*> &_edges, vector<RNN_Recurrent_Edge*> &_recurrent_edges, uint16_t seed) : RNN_Genome(_nodes, _edges) {
    recurrent_edges = _recurrent_edges;

    generator = minstd_rand0(seed);
    assign_reachability();
}



RNN_Genome* RNN_Genome::copy() {
    vector<RNN_Node_Interface*> node_copies;
    vector<RNN_Edge*> edge_copies;
    vector<RNN_Recurrent_Edge*> recurrent_edge_copies;

    for (uint32_t i = 0; i < nodes.size(); i++) {
        node_copies.push_back( nodes[i]->copy() );
    }

    for (uint32_t i = 0; i < edges.size(); i++) {
        edge_copies.push_back( edges[i]->copy(node_copies) );
    }

    for (uint32_t i = 0; i < recurrent_edges.size(); i++) {
        recurrent_edge_copies.push_back( recurrent_edges[i]->copy(node_copies) );
    }

    RNN_Genome *other = new RNN_Genome(node_copies, edge_copies, recurrent_edge_copies);

    other->bp_iterations = bp_iterations;
    other->learning_rate = learning_rate;
    other->adapt_learning_rate = adapt_learning_rate;
    other->use_nesterov_momentum = use_nesterov_momentum;
    other->use_reset_weights = use_reset_weights;

    other->use_high_norm = use_high_norm;
    other->high_threshold = high_threshold;
    other->use_low_norm = use_low_norm;
    other->low_threshold = low_threshold;

    other->use_dropout = use_dropout;
    other->dropout_probability = dropout_probability;

    other->log_filename = log_filename;

    other->generated_by_map = generated_by_map;

    other->initial_parameters = initial_parameters;

    other->best_validation_error = best_validation_error;
    other->best_parameters = best_parameters;

    other->assign_reachability();

    return other;
}


RNN_Genome::~RNN_Genome() {
    RNN_Node_Interface *node;

    while (nodes.size() > 0) {
        node = nodes.back();
        nodes.pop_back();
        delete node;
    }

    RNN_Edge *edge;

    while (edges.size() > 0) {
        edge = edges.back();
        edges.pop_back();
        delete edge;
    }

    RNN_Recurrent_Edge *recurrent_edge;

    while (recurrent_edges.size() > 0) {
        recurrent_edge = recurrent_edges.back();
        recurrent_edges.pop_back();
        delete recurrent_edge;
    }
}


string RNN_Genome::generated_by_string() {
    ostringstream oss;
    oss << "[";
    bool first = true;
    for (auto i = generated_by_map.begin(); i != generated_by_map.end(); i++) {
        if (!first) oss << ", ";
        oss << i->first << ":" << generated_by_map[i->first];
        first = false;
    }
    oss << "]";

    return oss.str();
}

void RNN_Genome::set_bp_iterations(int32_t _bp_iterations) {
    bp_iterations = _bp_iterations;
}

void RNN_Genome::set_learning_rate(double _learning_rate) {
    learning_rate = _learning_rate;
}

void RNN_Genome::set_adapt_learning_rate(bool _adapt_learning_rate) {
    adapt_learning_rate = _adapt_learning_rate;
}

void RNN_Genome::set_nesterov_momentum(bool _use_nesterov_momentum) {
    use_nesterov_momentum = _use_nesterov_momentum;
}

void RNN_Genome::set_reset_weights(bool _use_reset_weights) {
    use_reset_weights = _use_reset_weights;
}


void RNN_Genome::disable_high_threshold() {
    use_high_norm = false;
}

void RNN_Genome::enable_high_threshold(double _high_threshold) {
    use_high_norm = true;
    high_threshold = _high_threshold;
}

void RNN_Genome::disable_low_threshold() {
    use_low_norm = false;
}

void RNN_Genome::enable_low_threshold(double _low_threshold) {
    use_low_norm = true;
    low_threshold = _low_threshold;
}

void RNN_Genome::disable_dropout() {
    use_dropout = false;
}

void RNN_Genome::enable_dropout(double _dropout_probability) {
    dropout_probability = _dropout_probability;
}

void RNN_Genome::set_log_filename(string _log_filename) {
    log_filename = _log_filename;
}




void RNN_Genome::get_weights(vector<double> &parameters) {
    parameters.resize(get_number_weights());

    uint32_t current = 0;

    for (uint32_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->is_reachable()) nodes[i]->get_weights(current, parameters);
    }

    for (uint32_t i = 0; i < edges.size(); i++) {
        if (edges[i]->is_reachable()) parameters[current++] = edges[i]->weight;
    }

    for (uint32_t i = 0; i < recurrent_edges.size(); i++) {
        if (recurrent_edges[i]->is_reachable()) parameters[current++] = recurrent_edges[i]->weight;
    }
}

void RNN_Genome::set_weights(const vector<double> &parameters) {
    if (parameters.size() != get_number_weights()) {
        cerr << "ERROR! Trying to set weights where the RNN has " << get_number_weights() << " weights, and the parameters vector has << " << parameters.size() << " weights!" << endl;
        exit(1);
    }

    uint32_t current = 0;

    for (uint32_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->is_reachable()) nodes[i]->set_weights(current, parameters);
    }

    for (uint32_t i = 0; i < edges.size(); i++) {
        if (edges[i]->is_reachable()) edges[i]->weight = parameters[current++];
    }

    for (uint32_t i = 0; i < recurrent_edges.size(); i++) {
        if (recurrent_edges[i]->is_reachable()) recurrent_edges[i]->weight = parameters[current++];
    }

}

uint32_t RNN_Genome::get_number_weights() {
    uint32_t number_weights = 0;

    for (uint32_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->is_reachable()) number_weights += nodes[i]->get_number_weights();
    }

    for (uint32_t i = 0; i < edges.size(); i++) {
        if (edges[i]->is_reachable()) number_weights++;
    }

    for (uint32_t i = 0; i < recurrent_edges.size(); i++) {
        if (recurrent_edges[i]->is_reachable()) number_weights++;
    }

    return number_weights;
}

void RNN_Genome::initialize_randomly() {
    cout << "initializing randomly!" << endl;
    int number_of_weights = get_number_weights();
    initial_parameters.assign(number_of_weights, 0.0);

    uniform_real_distribution<double> rng(-0.5, 0.5);
    for (uint32_t i = 0; i < initial_parameters.size(); i++) {
        initial_parameters[i] = rng(generator);
    }
}


RNN* RNN_Genome::get_rnn() {
    vector<RNN_Node_Interface*> node_copies;
    vector<RNN_Edge*> edge_copies;
    vector<RNN_Recurrent_Edge*> recurrent_edge_copies;

    for (uint32_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->type == RNN_INPUT_NODE || nodes[i]->type == RNN_OUTPUT_NODE || nodes[i]->is_reachable()) node_copies.push_back( nodes[i]->copy() );
    }

    for (uint32_t i = 0; i < edges.size(); i++) {
        if (edges[i]->is_reachable()) edge_copies.push_back( edges[i]->copy(node_copies) );
    }

    for (uint32_t i = 0; i < recurrent_edges.size(); i++) {
        if (recurrent_edges[i]->is_reachable()) recurrent_edge_copies.push_back( recurrent_edges[i]->copy(node_copies) );
    }

    return new RNN(node_copies, edge_copies, recurrent_edge_copies);
}

int32_t RNN_Genome::get_generation_id() const {
    return generation_id;
}

double RNN_Genome::get_validation_error() const {
    return best_validation_error;
}


void RNN_Genome::set_generated_by(string type) {
    generated_by_map[type]++;
}

int RNN_Genome::get_generated_by(string type) {
    return generated_by_map[type];
}

bool RNN_Genome::sanity_check() {
    return true;
}


void forward_pass_thread(RNN* rnn, const vector<double> &parameters, const vector< vector<double> > &inputs, const vector< vector<double> > &outputs, uint32_t i, double *mses, bool use_dropout, bool training, double dropout_probability) {
    rnn->set_weights(parameters);
    rnn->forward_pass(inputs, use_dropout, training, dropout_probability);
    mses[i] = rnn->calculate_error_mse(outputs);

    //mses[i] = rnn->calculate_error_mae(outputs);
    //cout << "mse[" << i << "]: " << mse_current << endl;
}

void RNN_Genome::get_analytic_gradient(vector<RNN*> &rnns, const vector<double> &parameters, const vector< vector< vector<double> > > &inputs, const vector< vector< vector<double> > > &outputs, double &mse, vector<double> &analytic_gradient, bool training) {

    double *mses = new double[rnns.size()];
    double mse_sum = 0.0;
    vector<thread> threads;
    for (uint32_t i = 0; i < rnns.size(); i++) {
        threads.push_back( thread(forward_pass_thread, rnns[i], parameters, inputs[i], outputs[i], i, mses, use_dropout, training, dropout_probability) );
    }

    for (uint32_t i = 0; i < rnns.size(); i++) {
        threads[i].join();
        mse_sum += mses[i];
    }
    delete [] mses;

    for (uint32_t i = 0; i < rnns.size(); i++) {
        double d_mse = mse_sum * (1.0 / outputs[i][0].size()) * 2.0;
        rnns[i]->backward_pass(d_mse, use_dropout, training, dropout_probability);

        //double d_mae = mse_sum * (1.0 / outputs[i][0].size());
        //rnns[i]->backward_pass(d_mae);

    }

    mse = mse_sum;

    vector<double> current_gradients;
    analytic_gradient.assign(parameters.size(), 0.0);
    for (uint32_t k = 0; k < rnns.size(); k++) {

        uint32_t current = 0;
        for (uint32_t i = 0; i < rnns[k]->get_number_nodes(); i++) {
            rnns[k]->get_node(i)->get_gradients(current_gradients);

            for (uint32_t j = 0; j < current_gradients.size(); j++) {
                analytic_gradient[current] += current_gradients[j];
                current++;
            }
        }

        for (uint32_t i = 0; i < rnns[k]->get_number_edges(); i++) {
            analytic_gradient[current] += rnns[k]->get_edge(i)->get_gradient();
            current++;
        }
    }
}


void RNN_Genome::backpropagate(const vector< vector< vector<double> > > &inputs, const vector< vector< vector<double> > > &outputs, const vector< vector< vector<double> > > &validation_inputs, const vector< vector< vector<double> > > &validation_outputs) {

    double learning_rate = this->learning_rate / inputs.size();
    double low_threshold = sqrt(this->low_threshold * inputs.size());
    double high_threshold = sqrt(this->high_threshold * inputs.size());

    int32_t n_series = inputs.size();
    vector<RNN*> rnns;
    for (int32_t i = 0; i < n_series; i++) {
        rnns.push_back( this->get_rnn() );
    }

    vector<double> parameters = initial_parameters;

    int n_parameters = this->get_number_weights();
    vector<double> prev_parameters(n_parameters, 0.0);

    vector<double> prev_velocity(n_parameters, 0.0);
    vector<double> prev_prev_velocity(n_parameters, 0.0);

    vector<double> analytic_gradient;
    vector<double> prev_gradient(n_parameters, 0.0);

    double mu = 0.9;
    double original_learning_rate = learning_rate;

    double prev_mu;
    double prev_norm;
    double prev_learning_rate;
    double prev_mse;
    double mse;

    double parameter_norm = 0.0;
    double velocity_norm = 0.0;
    double norm = 0.0;

    //initialize the initial previous values
    get_analytic_gradient(rnns, parameters, inputs, outputs, mse, analytic_gradient, true);
    double validation_error = get_mse(validation_inputs, validation_outputs);
    best_validation_error = validation_error;
    best_parameters = parameters;

    norm = 0.0;
    for (int32_t i = 0; i < parameters.size(); i++) {
        norm += analytic_gradient[i] * analytic_gradient[i];
    }
    norm = sqrt(norm);
    
    ofstream *output_log = NULL;
    if (log_filename != "") {
        output_log = new ofstream(log_filename);
    }

    bool was_reset = false;
    double reset_count = 0;
    for (uint32_t iteration = 0; iteration < bp_iterations; iteration++) {
        prev_mu = mu;
        prev_norm  = norm;
        prev_mse = mse;
        prev_learning_rate = learning_rate;


        prev_gradient = analytic_gradient;

        get_analytic_gradient(rnns, parameters, inputs, outputs, mse, analytic_gradient, true);

        this->set_weights(parameters);
        validation_error = get_mse(validation_inputs, validation_outputs);
        if (validation_error < best_validation_error) {
            best_validation_error = validation_error;
            best_parameters = parameters;
        }

        norm = 0.0;
        velocity_norm = 0.0;
        parameter_norm = 0.0;
        for (int32_t i = 0; i < parameters.size(); i++) {
            norm += analytic_gradient[i] * analytic_gradient[i];
            velocity_norm += prev_velocity[i] * prev_velocity[i];
            parameter_norm += parameters[i] * parameters[i];
        }
        norm = sqrt(norm);
        velocity_norm = sqrt(velocity_norm);

        if (output_log != NULL) {
            (*output_log) << iteration
                << " " << mse 
                << " " << validation_error
                << " " << best_validation_error << endl;
        }

        cout << "iteration " << setw(10) << iteration
             << ", mse: " << setw(10) << mse 
             << ", v_mse: " << setw(10) << validation_error 
             << ", bv_mse: " << setw(10) << best_validation_error 
             << ", lr: " << setw(10) << learning_rate 
             << ", norm: " << setw(10) << norm
             << ", p_norm: " << setw(10) << parameter_norm
             << ", v_norm: " << setw(10) << velocity_norm;

        if (use_reset_weights && prev_mse * 1.25 < mse) {
            cout << ", RESETTING WEIGHTS " << reset_count << endl;
            parameters = prev_parameters;
            //prev_velocity = prev_prev_velocity;
            prev_velocity.assign(parameters.size(), 0.0);
            mse = prev_mse;
            mu = prev_mu;
            learning_rate = prev_learning_rate;
            analytic_gradient = prev_gradient;


            //learning_rate *= 0.5;
            //if (learning_rate < 0.0000001) learning_rate = 0.0000001;

            reset_count++;
            if (reset_count > 20) break;

            was_reset = true;
            continue;
        }

        if (was_reset) {
            was_reset = false;
        } else {
            reset_count -= 0.1;
            if (reset_count < 0) reset_count = 0;
            if (adapt_learning_rate) learning_rate = original_learning_rate;
        }


        if (adapt_learning_rate) {
            if (prev_mse > mse) {
                learning_rate *= 1.10;
                if (learning_rate > 1.0) learning_rate = 1.0;

                cout << ", INCREASING LR";
            }
        }

        if (use_high_norm && norm > high_threshold) {
            double high_threshold_norm = high_threshold / norm;

            cout << ", OVER THRESHOLD, multiplier: " << high_threshold_norm;

            for (int32_t i = 0; i < parameters.size(); i++) {
                analytic_gradient[i] = high_threshold_norm * analytic_gradient[i];
            }

            if (adapt_learning_rate) {
                learning_rate *= 0.5;
                if (learning_rate < 0.0000001) learning_rate = 0.0000001;
            }

        } else if (use_low_norm && norm < low_threshold) {
            double low_threshold_norm = low_threshold / norm;
            cout << ", UNDER THRESHOLD, multiplier: " << low_threshold_norm;

            for (int32_t i = 0; i < parameters.size(); i++) {
                analytic_gradient[i] = low_threshold_norm * analytic_gradient[i];
            }

            if (adapt_learning_rate) {
                if (prev_mse * 1.05 < mse) {
                    cout << ", WORSE";
                    learning_rate *= 0.5;
                    if (learning_rate < 0.0000001) learning_rate = 0.0000001;
                }
            }
        }

        if (reset_count > 0) {
            double reset_penalty = pow(5.0, -reset_count);
            cout << ", RESET PENALTY (" << reset_count << "): " << reset_penalty;

            for (int32_t i = 0; i < parameters.size(); i++) {
                analytic_gradient[i] = reset_penalty * analytic_gradient[i];
            }

        }


        cout << endl;

        if (use_nesterov_momentum) {
            for (int32_t i = 0; i < parameters.size(); i++) {
                prev_parameters[i] = parameters[i];
                prev_prev_velocity[i] = prev_velocity[i];

                double mu_v = prev_velocity[i] * prev_mu;

                prev_velocity[i] = mu_v  - (prev_learning_rate * prev_gradient[i]);
                parameters[i] += mu_v + ((mu + 1) * prev_velocity[i]);
            }
        } else {
            for (int32_t i = 0; i < parameters.size(); i++) {
                prev_parameters[i] = parameters[i];
                prev_gradient[i] = analytic_gradient[i];
                parameters[i] -= learning_rate * analytic_gradient[i];
            }
        }
    }

    RNN *g;
    for (int32_t i = 0; i < n_series; i++) {
        g = rnns.back();
        rnns.pop_back();
        delete g;

    }

    this->set_weights(best_parameters);
}

void RNN_Genome::backpropagate_stochastic(const vector< vector< vector<double> > > &inputs, const vector< vector< vector<double> > > &outputs, const vector< vector< vector<double> > > &validation_inputs, const vector< vector< vector<double> > > &validation_outputs) {

    vector<double> parameters = initial_parameters;

    int n_parameters = this->get_number_weights();
    vector<double> prev_parameters(n_parameters, 0.0);

    vector<double> prev_velocity(n_parameters, 0.0);
    vector<double> prev_prev_velocity(n_parameters, 0.0);

    vector<double> analytic_gradient;
    vector<double> prev_gradient(n_parameters, 0.0);

    double mu = 0.9;
    double original_learning_rate = learning_rate;

    int n_series = inputs.size();
    double prev_mu[n_series];
    double prev_norm[n_series];
    double prev_learning_rate[n_series];
    double prev_mse[n_series];
    double mse;

    double norm = 0.0;

    RNN* rnn = get_rnn();
    rnn->set_weights(parameters);

    //initialize the initial previous values
    for (uint32_t i = 0; i < n_series; i++) {
        rnn->get_analytic_gradient(parameters, inputs[i], outputs[i], mse, analytic_gradient, use_dropout, true, dropout_probability);

        norm = 0.0;
        for (int32_t j = 0; j < parameters.size(); j++) {
            norm += analytic_gradient[j] * analytic_gradient[j];
        }
        norm = sqrt(norm);
        prev_mu[i] = mu;
        prev_norm[i] = norm;
        prev_mse[i] = mse;
        prev_learning_rate[i] = learning_rate;
    }

    double validation_error = get_mse(validation_inputs, validation_outputs);
    best_validation_error = validation_error;
    best_parameters = parameters;

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    minstd_rand0 generator(seed);
    uniform_real_distribution<double> rng(0, 1);

    int random_selection = rng(generator);
    mu = prev_mu[random_selection];
    norm = prev_norm[random_selection];
    mse = prev_mse[random_selection];
    learning_rate = prev_learning_rate[random_selection];

    ofstream *output_log = NULL;
    
    if (log_filename != "") output_log = new ofstream(log_filename);

    vector<int32_t> shuffle_order;
    for (int32_t i = 0; i < (int32_t)inputs.size(); i++) {
        shuffle_order.push_back(i);
    }

    bool was_reset = false;
    int reset_count = 0;

    for (uint32_t iteration = 0; iteration < bp_iterations; iteration++) {
        fisher_yates_shuffle(generator, shuffle_order);

        double avg_norm = 0.0;
        for (uint32_t k = 0; k < shuffle_order.size(); k++) {
            random_selection = shuffle_order[k];

            prev_mu[random_selection] = mu;
            prev_norm[random_selection] = norm;
            prev_mse[random_selection] = mse;
            prev_learning_rate[random_selection] = learning_rate;

            prev_gradient = analytic_gradient;

            rnn->get_analytic_gradient(parameters, inputs[random_selection], outputs[random_selection], mse, analytic_gradient, use_dropout, true, dropout_probability);

            norm = 0.0;
            for (int32_t i = 0; i < parameters.size(); i++) {
                norm += analytic_gradient[i] * analytic_gradient[i];
            }
            norm = sqrt(norm);
            avg_norm += norm;

            /*
            cout << "iteration " << iteration
                << ", series: " << random_selection
                << ", mse: " << mse 
                << ", lr: " << learning_rate 
                << ", norm: " << norm;
                */

            if (use_reset_weights && prev_mse[random_selection] * 2 < mse) {
                //cout << ", RESETTING WEIGHTS" << endl;
                parameters = prev_parameters;
                //prev_velocity = prev_prev_velocity;
                prev_velocity.assign(parameters.size(), 0.0);
                mse = prev_mse[random_selection];
                mu = prev_mu[random_selection];
                learning_rate = prev_learning_rate[random_selection];
                analytic_gradient = prev_gradient;

                random_selection = rng(generator) * inputs.size();

                learning_rate *= 0.5;
                if (learning_rate < 0.0000001) learning_rate = 0.0000001;

                reset_count++;
                if (reset_count > 20) break;

                was_reset = true;
                k--;
                continue;
            }

            if (was_reset) {
                was_reset = false;
            } else {
                reset_count = 0;
                learning_rate = original_learning_rate;
            }


            if (adapt_learning_rate) {
                if (prev_mse[random_selection] > mse) {
                    learning_rate *= 1.10;
                    if (learning_rate > 1.0) learning_rate = 1.0;

                    //cout << ", INCREASING LR";
                }
            }

            if (use_high_norm && norm > high_threshold) {
                double high_threshold_norm = high_threshold / norm;
                //cout << ", OVER THRESHOLD, multiplier: " << high_threshold_norm;

                for (int32_t i = 0; i < parameters.size(); i++) {
                    analytic_gradient[i] = high_threshold_norm * analytic_gradient[i];
                }

                if (adapt_learning_rate) {
                    learning_rate *= 0.5;
                    if (learning_rate < 0.0000001) learning_rate = 0.0000001;
                }

            } else if (use_low_norm && norm < low_threshold) {
                double low_threshold_norm = low_threshold / norm;
                //cout << ", UNDER THRESHOLD, multiplier: " << low_threshold_norm;

                for (int32_t i = 0; i < parameters.size(); i++) {
                    analytic_gradient[i] = low_threshold_norm * analytic_gradient[i];
                }

                if (adapt_learning_rate) {
                    if (prev_mse[random_selection] * 1.05 < mse) {
                        cout << ", WORSE";
                        learning_rate *= 0.5;
                        if (learning_rate < 0.0000001) learning_rate = 0.0000001;
                    }
                }
            }

            //cout << endl;

            if (use_nesterov_momentum) {
                for (int32_t i = 0; i < parameters.size(); i++) {
                    prev_parameters[i] = parameters[i];
                    prev_prev_velocity[i] = prev_velocity[i];

                    double mu_v = prev_velocity[i] * prev_mu[random_selection];

                    prev_velocity[i] = mu_v  - (prev_learning_rate[random_selection] * prev_gradient[i]);
                    parameters[i] += mu_v + ((mu + 1) * prev_velocity[i]);

                    if (parameters[i] < -10.0) parameters[i] = -10.0;
                    else if (parameters[i] > 10.0) parameters[i] = 10.0;
                }
            } else {
                for (int32_t i = 0; i < parameters.size(); i++) {
                    prev_parameters[i] = parameters[i];
                    prev_gradient[i] = analytic_gradient[i];
                    parameters[i] -= learning_rate * analytic_gradient[i];

                    if (parameters[i] < -10.0) parameters[i] = -10.0;
                    else if (parameters[i] > 10.0) parameters[i] = 10.0;
                }
            }
        }

        this->set_weights(parameters);

        double training_error = get_mse(inputs, outputs);
        validation_error = get_mse(validation_inputs, validation_outputs);
        if (validation_error < best_validation_error) {
            best_validation_error = validation_error;
            best_parameters = parameters;
        }

        if (output_log != NULL) {
            (*output_log) << iteration
                << " " << training_error
                << " " << validation_error
                << " " << best_validation_error
                << " " << avg_norm << endl;

        }


        cout << "iteration " << iteration << ", mse: " << training_error << ", v_mse: " << validation_error << ", bv_mse: " << best_validation_error << ", avg_norm: " << avg_norm << endl;

    }

    this->set_weights(best_parameters);
    //cout << "backpropagation completed, getting mu/sigma" << endl;
    //double _mu, _sigma;
    //get_mu_sigma(best_parameters, _mu, _sigma);
}

double RNN_Genome::get_mse(const vector< vector< vector<double> > > &inputs, const vector< vector< vector<double> > > &outputs, bool verbose) {
    RNN *rnn = get_rnn();

    double mse;
    double avg_mse = 0.0;

    int32_t width = ceil(log10(inputs.size()));
    for (uint32_t i = 0; i < inputs.size(); i++) {
        mse = rnn->prediction_mse(inputs[i], outputs[i], use_dropout, false, dropout_probability);

        avg_mse += mse;

        if (verbose) {
            cout << "series[" << setw(width) << i << "] MSE:  " << mse << endl;
        }
    }

    delete rnn;

    avg_mse /= inputs.size();
    if (verbose) {
        cout << "average MSE:   " << string(width, ' ') << avg_mse << endl;
    }
    return avg_mse;
}

double RNN_Genome::get_mae(const vector< vector< vector<double> > > &inputs, const vector< vector< vector<double> > > &outputs, bool verbose) {
    RNN *rnn = get_rnn();

    double mae;
    double avg_mae = 0.0;

    int32_t width = ceil(log10(inputs.size()));
    for (uint32_t i = 0; i < inputs.size(); i++) {
        mae = rnn->prediction_mae(inputs[i], outputs[i], use_dropout, false, dropout_probability);

        avg_mae += mae;

        if (verbose) {
            cout << "series[" << setw(width) << i << "] MAE:  " << mae << endl;
        }
    }

    delete rnn;

    avg_mae /= inputs.size();
    if (verbose) {
        cout << "average MAE:   " << string(width, ' ') << avg_mae << endl;
    }
    return avg_mae;
}

bool RNN_Genome::equals(RNN_Genome* other) {

    if (nodes.size() != other->nodes.size()) return false;
    if (edges.size() != other->edges.size()) return false;
    if (recurrent_edges.size() != other->recurrent_edges.size()) return false;

    for (int32_t i = 0; i < (int32_t)nodes.size(); i++) {
        if (!nodes[i]->equals(other->nodes[i])) return false;
    }

    for (int32_t i = 0; i < (int32_t)edges.size(); i++) {
        if (!edges[i]->equals(other->edges[i])) return false;
    }


    for (int32_t i = 0; i < (int32_t)recurrent_edges.size(); i++) {
        if (!recurrent_edges[i]->equals(other->recurrent_edges[i])) return false;
    }

    return true;
}

void RNN_Genome::assign_reachability() {
    cout << "assigning reachability!" << endl;
    cout << nodes.size() << " nodes, " << edges.size() << " edges, " << recurrent_edges.size() << " recurrent_edges" << endl;

    for (int32_t i = 0; i < (int32_t)nodes.size(); i++) {
        nodes[i]->forward_reachable = false;
        nodes[i]->backward_reachable = false;
        nodes[i]->total_inputs = 0;
        nodes[i]->total_outputs = 0;

        //set enabled input nodes as reachable
        if (nodes[i]->type == RNN_INPUT_NODE && nodes[i]->enabled) {
            nodes[i]->forward_reachable = true;
            nodes[i]->total_inputs = 1;
            
            //cout << "\tsetting input node[" << i << "] reachable" << endl;
        }

        if (nodes[i]->type == RNN_OUTPUT_NODE) {
            nodes[i]->backward_reachable = true;
            nodes[i]->total_outputs = 1;
        }
    }

    for (int32_t i = 0; i < (int32_t)edges.size(); i++) {
        edges[i]->forward_reachable = false;
        edges[i]->backward_reachable = false;
    }

    for (int32_t i = 0; i < (int32_t)recurrent_edges.size(); i++) {
        recurrent_edges[i]->forward_reachable = false;
        recurrent_edges[i]->backward_reachable = false;
    }

    //do forward reachability
    vector<RNN_Node_Interface*> nodes_to_visit;
    for (int32_t i = 0; i < (int32_t)nodes.size(); i++) {
        if (nodes[i]->type == RNN_INPUT_NODE && nodes[i]->enabled) {
            nodes_to_visit.push_back(nodes[i]);
        }
    }

    while (nodes_to_visit.size() > 0) {
        RNN_Node_Interface *current = nodes_to_visit.back();
        nodes_to_visit.pop_back();

        //if the node is not enabled, we don't need to do anything
        if (!current->enabled) continue;

        for (int32_t i = 0; i < (int32_t)edges.size(); i++) {
            if (edges[i]->input_innovation_number == current->innovation_number &&
                    edges[i]->enabled) {
                //this is an edge coming out of this node

                if (edges[i]->output_node->enabled) {
                    edges[i]->forward_reachable = true;

                    if (edges[i]->output_node->forward_reachable == false) {
                        if (edges[i]->output_node->innovation_number == edges[i]->input_node->innovation_number) {
                            cerr << "ERROR, forward edge was circular -- this should never happen" << endl;
                            exit(1);
                        }
                        edges[i]->output_node->forward_reachable = true;
                        nodes_to_visit.push_back(edges[i]->output_node);
                    }
                }
            }
        }

        for (int32_t i = 0; i < (int32_t)recurrent_edges.size(); i++) {
            if (recurrent_edges[i]->forward_reachable) continue;

            if (recurrent_edges[i]->input_innovation_number == current->innovation_number &&
                    recurrent_edges[i]->enabled) {
                //this is an recurrent_edge coming out of this node

                if (recurrent_edges[i]->output_node->enabled) {
                    recurrent_edges[i]->forward_reachable = true;

                    if (recurrent_edges[i]->output_node->forward_reachable == false) {
                        recurrent_edges[i]->output_node->forward_reachable = true;

                        //handle the edge case when a recurrent edge loops back on itself
                        nodes_to_visit.push_back(recurrent_edges[i]->output_node);
                    }
                }
            }
        }
    }

    //do backward reachability
    nodes_to_visit.clear();
    for (int32_t i = 0; i < (int32_t)nodes.size(); i++) {
        if (nodes[i]->type == RNN_OUTPUT_NODE && nodes[i]->enabled) {
            nodes_to_visit.push_back(nodes[i]);
        }
    }

    while (nodes_to_visit.size() > 0) {
        RNN_Node_Interface *current = nodes_to_visit.back();
        nodes_to_visit.pop_back();

        //if the node is not enabled, we don't need to do anything
        if (!current->enabled) continue;

        for (int32_t i = 0; i < (int32_t)edges.size(); i++) {
            if (edges[i]->output_innovation_number == current->innovation_number &&
                    edges[i]->enabled) {
                //this is an edge coming out of this node

                if (edges[i]->input_node->enabled) {
                    edges[i]->backward_reachable = true;
                    if (edges[i]->input_node->backward_reachable == false) {
                        edges[i]->input_node->backward_reachable = true;
                        nodes_to_visit.push_back(edges[i]->input_node);
                    }
                }
            }
        }

        for (int32_t i = 0; i < (int32_t)recurrent_edges.size(); i++) {
            if (recurrent_edges[i]->output_innovation_number == current->innovation_number &&
                    recurrent_edges[i]->enabled) {
                //this is an recurrent_edge coming out of this node

                if (recurrent_edges[i]->input_node->enabled) {
                    recurrent_edges[i]->backward_reachable = true;
                    if (recurrent_edges[i]->input_node->backward_reachable == false) {
                        recurrent_edges[i]->input_node->backward_reachable = true;
                        nodes_to_visit.push_back(recurrent_edges[i]->input_node);
                    }
                }
            }
        }
    }

    //set inputs/outputs
    for (int32_t i = 0; i < (int32_t)edges.size(); i++) {
        if (edges[i]->is_reachable()) {
            edges[i]->input_node->total_outputs++;
            edges[i]->output_node->total_inputs++;
        }
    }

    for (int32_t i = 0; i < (int32_t)recurrent_edges.size(); i++) {
        if (recurrent_edges[i]->is_reachable()) {
            recurrent_edges[i]->input_node->total_outputs++;
            recurrent_edges[i]->output_node->total_inputs++;
        }
    }

    /*
    cout << "node reachabiltity:" << endl;
    for (int32_t i = 0; i < nodes.size(); i++) {
        RNN_Node_Interface *n = nodes[i];
        cout << "node " << n->innovation_number << ", e: " << n->enabled << ", fr: " << n->forward_reachable << ", br: " << n->backward_reachable << ", ti: " << n->total_inputs << ", to: " << n->total_outputs << endl;
    }

    cout << "edge reachability:" << endl;
    for (int32_t i = 0; i < edges.size(); i++) {
        RNN_Edge *e = edges[i];
        cout << "edge " << e->innovation_number << ", e: " << e->enabled << ", fr: " << e->forward_reachable << ", br: " << e->backward_reachable << endl;
    }

    cout << "recurrent edge reachability:" << endl;
    for (int32_t i = 0; i < recurrent_edges.size(); i++) {
        RNN_Recurrent_Edge *e = recurrent_edges[i];
        cout << "recurrent edge " << e->innovation_number << ", e: " << e->enabled << ", fr: " << e->forward_reachable << ", br: " << e->backward_reachable << endl;
    }
    */
}


bool RNN_Genome::outputs_unreachable() {
    assign_reachability();

    for (int32_t i = 0; i < (int32_t)nodes.size(); i++) {
        if (nodes[i]->type == RNN_OUTPUT_NODE && !nodes[i]->is_reachable()) return true;
    }

    return false;
}

void RNN_Genome::get_mu_sigma(const vector<double> &p, double &mu, double &sigma) {
    if (p.size() == 0) {
        mu = 0.0;
        sigma = 0.25;
        cout << "\tmu: " << mu << ", sigma: " << sigma << ", paramters.size() == 0" << endl;
        return;
    }

    mu = 0.0;
    sigma = 0.0;
    
    for (int32_t i = 0; i < p.size(); i++) {
        mu += p[i];
    }
    mu /= p.size();

    double temp;
    for (int32_t i = 0; i < p.size(); i++) {
        temp = (mu - p[i]) * (mu - p[i]);
        sigma += temp;
    }

    sigma /= (p.size() - 1);
    sigma = sqrt(sigma);

    cout << "\tmu: " << mu << ", sigma: " << sigma << endl;
    if (isnan(mu) || isinf(mu) || isnan(sigma) || isinf(sigma)) {
        cerr << "mu or sigma was not a number, best parameters: " << endl;
        for (int32_t i = 0; i < (int32_t)p.size(); i++) { 
            cerr << "\t" << p[i] << endl;
        }

        exit(1);
    }
}


RNN_Node_Interface* RNN_Genome::create_node(double mu, double sigma, double lstm_node_rate, int32_t &node_innovation_count, double depth) {
    RNN_Node_Interface *n = NULL;

    if (rng_0_1(generator) < lstm_node_rate) {
        n = new LSTM_Node(++node_innovation_count, RNN_HIDDEN_NODE, depth);
    } else {
        n = new RNN_Node(++node_innovation_count, RNN_HIDDEN_NODE, depth);
    }

    n->initialize_randomly(generator, normal_distribution, mu, sigma);

    return n;
}

bool RNN_Genome::attempt_edge_insert(RNN_Node_Interface *n1, RNN_Node_Interface *n2, double mu, double sigma, int32_t &edge_innovation_count) {
    cout << "\tadding edge between nodes " << n1->innovation_number << " and " << n2->innovation_number << endl;

    if (n1->depth == n2->depth) {
        cout << "\tcannot add edge between nodes as their depths are the same: " << n1->depth << " and " << n2->depth << endl;
        return false;
    }

    if (n2->depth < n1->depth) {
        //swap the nodes so that the lower one is first
        RNN_Node_Interface *temp = n2;
        n2 = n1;
        n1 = temp;
        cout << "\tswaping nodes, because n2->depth < n1->depth" << endl;
    }


    //check to see if an edge between the two nodes already exists
    for (int32_t i = 0; i < (int32_t)edges.size(); i++) {
        if (edges[i]->input_innovation_number == n1->innovation_number &&
                edges[i]->output_innovation_number == n2->innovation_number) {
            if (!edges[i]->enabled) {
                //edge was disabled so we can enable it
                cout << "\tedge already exists but was disabled, enabling it." << endl;
                edges[i]->enabled = true;
                return true;
            } else {
                cout << "\tedge already exists, not adding." << endl;
                //edge was already enabled, so there will not be a change
                return false;
            }
        }
    }

    RNN_Edge *e = new RNN_Edge(++edge_innovation_count, n1, n2);
    e->weight = normal_distribution.random(generator, mu, sigma);
    cout << "\tadding edge between nodes " << e->input_innovation_number << " and " << e->output_innovation_number << ", new edge weight: " << e->weight << endl;

    edges.insert( upper_bound(edges.begin(), edges.end(), e, sort_RNN_Edges_by_depth()), e);
    return true;
}


bool RNN_Genome::add_edge(double mu, double sigma, int32_t &edge_innovation_count) {
    cout << "\tattempting to add edge!" << endl;
    vector<RNN_Node_Interface*> reachable_nodes;
    for (int32_t i = 0; i < (int32_t)nodes.size(); i++) {
        if (nodes[i]->is_reachable()) reachable_nodes.push_back(nodes[i]);
    }
    cout << "\treachable_nodes.size(): " << reachable_nodes.size() << endl;

    int position = rng_0_1(generator) * reachable_nodes.size();

    RNN_Node_Interface *n1 = reachable_nodes[position];
    cout << "\tselected first node " << n1->innovation_number << " with depth " << n1->depth << endl;

    for (auto i = reachable_nodes.begin(); i != reachable_nodes.end();) {
        if ((*i)->depth == n1->depth) {
            cout << "\t\terasing node " << (*i)->innovation_number << " with depth " << (*i)->depth << endl;
            reachable_nodes.erase(i);
        } else {
            cout << "\t\tkeeping node " << (*i)->innovation_number << " with depth " << (*i)->depth << endl;
            i++;
        }
    }
    cout << "\treachable_nodes.size(): " << reachable_nodes.size() << endl;


    position = rng_0_1(generator) * reachable_nodes.size();
    RNN_Node_Interface *n2 = reachable_nodes[position];
    cout << "\tselected second node " << n2->innovation_number << " with depth " << n2->depth << endl;

    return attempt_edge_insert(n1, n2, mu, sigma, edge_innovation_count);
}

bool RNN_Genome::add_recurrent_edge(double mu, double sigma, int32_t &edge_innovation_count) {
    cout << "\tattempting to add recurrent edge!" << endl;
    vector<RNN_Node_Interface*> reachable_nodes;
    for (int32_t i = 0; i < (int32_t)nodes.size(); i++) {
        if (nodes[i]->type != RNN_INPUT_NODE && nodes[i]->is_reachable()) reachable_nodes.push_back(nodes[i]);
    }
    cout << "\treachable_nodes.size(): " << reachable_nodes.size() << endl;
    if (reachable_nodes.size() == 0) return false;

    int position = rng_0_1(generator) * reachable_nodes.size();

    RNN_Node_Interface *n1 = reachable_nodes[position];
    cout << "\tselected first node " << n1->innovation_number << " with depth " << n1->depth << endl;

    position = rng_0_1(generator) * reachable_nodes.size();
    RNN_Node_Interface *n2 = reachable_nodes[position];
    cout << "\tselected second node " << n2->innovation_number << " with depth " << n2->depth << endl;

    //no need to swap the nodes as recurrent connections can go backwards

    //check to see if an edge between the two nodes already exists
    for (int32_t i = 0; i < (int32_t)recurrent_edges.size(); i++) {
        if (recurrent_edges[i]->input_innovation_number == n1->innovation_number &&
                recurrent_edges[i]->output_innovation_number == n2->innovation_number) {
            if (!recurrent_edges[i]->enabled) {
                recurrent_edges[i]->enabled = true;
                cout << "\tenabling recurrent edge between nodes " << n1->innovation_number << " and " << n2->innovation_number << endl;
                return true;
            } else {
                cout << "\tenabled edge already existed between selected nodes " << n1->innovation_number << " and " << n2->innovation_number << endl;

                return false;
            }
        }
    }

    cout << "\tadding recurrent edge between nodes " << n1->innovation_number << " and " << n2->innovation_number << endl;

    //edge with same input/output did not exist, now we can create it
    RNN_Recurrent_Edge *recurrent_edge = new RNN_Recurrent_Edge(++edge_innovation_count, n1, n2);
    recurrent_edge->weight = normal_distribution.random(generator, mu, sigma);

    recurrent_edges.insert( upper_bound(recurrent_edges.begin(), recurrent_edges.end(), recurrent_edge, sort_RNN_Recurrent_Edges_by_depth()), recurrent_edge);

    return true;
}


//TODO: should probably change these to enable/disable path
bool RNN_Genome::disable_edge() {
    //TODO: edge should be reachable
    vector<RNN_Edge*> enabled_edges;
    for (int32_t i = 0; i < edges.size(); i++) {
        if (edges[i]->enabled) enabled_edges.push_back(edges[i]);
    }

    vector<RNN_Recurrent_Edge*> enabled_recurrent_edges;
    for (int32_t i = 0; i < recurrent_edges.size(); i++) {
        if (recurrent_edges[i]->enabled) enabled_recurrent_edges.push_back(recurrent_edges[i]);
    }

    if ((enabled_edges.size() + enabled_recurrent_edges.size()) == 0) {
        return false;
    }


    int32_t position = (enabled_edges.size() + enabled_recurrent_edges.size()) * rng_0_1(generator);

    if (position < enabled_edges.size()) {
        enabled_edges[position]->enabled = false;
        return true;
    } else {
        position -= enabled_edges.size();
        enabled_recurrent_edges[position]->enabled = false;
        return true;
    }
}

bool RNN_Genome::enable_edge() {
    //TODO: edge should be reachable
    vector<RNN_Edge*> disabled_edges;
    for (int32_t i = 0; i < edges.size(); i++) {
        if (!edges[i]->enabled) disabled_edges.push_back(edges[i]);
    }

    vector<RNN_Recurrent_Edge*> disabled_recurrent_edges;
    for (int32_t i = 0; i < recurrent_edges.size(); i++) {
        if (!recurrent_edges[i]->enabled) disabled_recurrent_edges.push_back(recurrent_edges[i]);
    }

    if ((disabled_edges.size() + disabled_recurrent_edges.size()) == 0) {
        return false;
    }

    int32_t position = (disabled_edges.size() + disabled_recurrent_edges.size()) * rng_0_1(generator);

    if (position < disabled_edges.size()) {
        disabled_edges[position]->enabled = true;
        return true;
    } else {
        position -= disabled_edges.size();
        disabled_recurrent_edges[position]->enabled = true;
        return true;
    }
}


bool RNN_Genome::split_edge(double mu, double sigma, double lstm_node_rate, int32_t &edge_innovation_count, int32_t &node_innovation_count) {
    cout << "\tattempting to split an edge!" << endl;
    vector<RNN_Edge*> enabled_edges;
    for (int32_t i = 0; i < edges.size(); i++) {
        if (edges[i]->enabled) enabled_edges.push_back(edges[i]);
    }

    int32_t position = rng_0_1(generator) * enabled_edges.size();
    RNN_Edge *edge = enabled_edges[position];
    RNN_Node_Interface *n1 = edge->input_node;
    RNN_Node_Interface *n2 = edge->output_node;

    edge->enabled = false;


    double new_depth = (n1->get_depth() + n2->get_depth()) / 2.0;
    RNN_Node_Interface *new_node = create_node(mu, sigma, lstm_node_rate, node_innovation_count, new_depth);

    nodes.insert( upper_bound(nodes.begin(), nodes.end(), new_node, sort_RNN_Nodes_by_depth()), new_node);
   
    attempt_edge_insert(n1, new_node, mu, sigma, edge_innovation_count);
    attempt_edge_insert(new_node, n2, mu, sigma, edge_innovation_count);

    return true;
}

bool RNN_Genome::add_node(double mu, double sigma, double lstm_node_rate, int32_t &edge_innovation_count, int32_t &node_innovation_count) {
    double split_depth = rng_0_1(generator);

    vector<RNN_Node_Interface*> possible_inputs;
    vector<RNN_Node_Interface*> possible_outputs;

    for (int32_t i = 0; i < (int32_t)nodes.size(); i++) {
        if (nodes[i]->depth < split_depth) possible_inputs.push_back(nodes[i]);
        else possible_outputs.push_back(nodes[i]);
    }

    int32_t max_inputs = 5;
    int32_t max_outputs = 5;

    while (possible_inputs.size() > max_inputs) {
        int32_t position = rng_0_1(generator) * possible_inputs.size();
        possible_inputs.erase(possible_inputs.begin() + position);
    }

    while (possible_outputs.size() > max_outputs) {
        int32_t position = rng_0_1(generator) * possible_outputs.size();
        possible_outputs.erase(possible_outputs.begin() + position);
    }

    RNN_Node_Interface *new_node = create_node(mu, sigma, lstm_node_rate, node_innovation_count, split_depth);
    nodes.insert( upper_bound(nodes.begin(), nodes.end(), new_node, sort_RNN_Nodes_by_depth()), new_node);

    for (int32_t i = 0; i < possible_inputs.size(); i++) {
        attempt_edge_insert(possible_inputs[i], new_node, mu, sigma, edge_innovation_count);
    }

    for (int32_t i = 0; i < possible_outputs.size(); i++) {
        attempt_edge_insert(new_node, possible_outputs[i], mu, sigma, edge_innovation_count);
    }

    return true;
}

bool RNN_Genome::enable_node() {
    cout << "\tattempting to enable a node!" << endl;
    vector<RNN_Node_Interface*> possible_nodes;
    for (int32_t i = 0; i < (int32_t)nodes.size(); i++) {
        if (!nodes[i]->enabled) possible_nodes.push_back(nodes[i]);
    }

    if (possible_nodes.size() == 0) return false;

    int position = rng_0_1(generator) * possible_nodes.size();
    possible_nodes[position]->enabled = true;
    cout << "\tenabling node " << possible_nodes[position]->innovation_number << " at depth " << possible_nodes[position]->depth << endl;

    return true;
}

bool RNN_Genome::disable_node() {
    cout << "\tattempting to disable a node!" << endl;
    vector<RNN_Node_Interface*> possible_nodes;
    for (int32_t i = 0; i < (int32_t)nodes.size(); i++) {
        if (nodes[i]->type != RNN_OUTPUT_NODE && nodes[i]->enabled) possible_nodes.push_back(nodes[i]);
    }

    if (possible_nodes.size() == 0) return false;

    int position = rng_0_1(generator) * possible_nodes.size();
    possible_nodes[position]->enabled = false;
    cout << "\tdisabling node " << possible_nodes[position]->innovation_number << " at depth " << possible_nodes[position]->depth << endl;

    return true;
}

bool RNN_Genome::split_node(double mu, double sigma, double lstm_node_rate, int32_t &edge_innovation_count, int32_t &node_innovation_count) {
    cout << "\tattempting to split a node!" << endl;
    vector<RNN_Node_Interface*> possible_nodes;
    for (int32_t i = 0; i < (int32_t)nodes.size(); i++) {
        if (nodes[i]->type != RNN_INPUT_NODE && nodes[i]->type != RNN_OUTPUT_NODE) possible_nodes.push_back(nodes[i]);
    }

    if (possible_nodes.size() == 0) return false;

    int position = rng_0_1(generator) * possible_nodes.size();
    RNN_Node_Interface *selected_node = possible_nodes[position];
    cout << "\tselected node: " << selected_node->innovation_number << " at depth " << selected_node->depth << endl;

    vector<RNN_Edge*> input_edges;
    vector<RNN_Edge*> output_edges;
    for (int32_t i = 0; i < (int32_t)edges.size(); i++) {
        if (edges[i]->output_innovation_number == selected_node->innovation_number) {
            input_edges.push_back(edges[i]);
        }

        if (edges[i]->input_innovation_number == selected_node->innovation_number) {
            output_edges.push_back(edges[i]);
        }
    }

    vector<RNN_Edge*> input_edges_1;
    vector<RNN_Edge*> input_edges_2;

    for (int32_t i = 0; i < (int32_t)input_edges.size(); i++) {
        if (rng_0_1(generator) < 0.5) input_edges_1.push_back(input_edges[i]);
        if (rng_0_1(generator) < 0.5) input_edges_2.push_back(input_edges[i]);
    }

    //make sure there is at least one input edge
    if (input_edges_1.size() == 0) {
        int position = rng_0_1(generator) * input_edges.size();
        input_edges_1.push_back(input_edges[position]);
    }

    if (input_edges_2.size() == 0) {
        int position = rng_0_1(generator) * input_edges.size();
        input_edges_2.push_back(input_edges[position]);
    }

    vector<RNN_Edge*> output_edges_1;
    vector<RNN_Edge*> output_edges_2;

    for (int32_t i = 0; i < (int32_t)output_edges.size(); i++) {
        if (rng_0_1(generator) < 0.5) output_edges_1.push_back(output_edges[i]);
        if (rng_0_1(generator) < 0.5) output_edges_2.push_back(output_edges[i]);
    }

    //make sure there is at least one output edge
    if (output_edges_1.size() == 0) {
        int position = rng_0_1(generator) * output_edges.size();
        output_edges_1.push_back(output_edges[position]);
    }

    if (output_edges_2.size() == 0) {
        int position = rng_0_1(generator) * output_edges.size();
        output_edges_2.push_back(output_edges[position]);
    }

    //create the two new nodes
    double n1_avg_input = 0.0, n1_avg_output = 0.0;
    double n2_avg_input = 0.0, n2_avg_output = 0.0;

    for (int32_t i = 0; i < (int32_t)input_edges_1.size(); i++) {
        n1_avg_input += input_edges_1[i]->input_node->depth;
    }
    n1_avg_input /= input_edges_1.size();

    for (int32_t i = 0; i < (int32_t)output_edges_1.size(); i++) {
        n1_avg_output += output_edges_1[i]->output_node->depth;
    }
    n1_avg_output /= output_edges_1.size();

    for (int32_t i = 0; i < (int32_t)input_edges_2.size(); i++) {
        n2_avg_input += input_edges_2[i]->input_node->depth;
    }
    n2_avg_input /= input_edges_2.size();

    for (int32_t i = 0; i < (int32_t)output_edges_2.size(); i++) {
        n2_avg_output += output_edges_2[i]->output_node->depth;
    }
    n2_avg_output /= output_edges_2.size();

    double new_depth_1 = (n1_avg_input + n1_avg_output) / 2.0;
    double new_depth_2 = (n2_avg_input + n2_avg_output) / 2.0;

    RNN_Node_Interface *new_node_1 = create_node(mu, sigma, lstm_node_rate, node_innovation_count, new_depth_1);
    RNN_Node_Interface *new_node_2 = create_node(mu, sigma, lstm_node_rate, node_innovation_count, new_depth_2);

    //create the new edges
    for (int32_t i = 0; i < (int32_t)input_edges_1.size(); i++) {
        attempt_edge_insert(input_edges_1[i]->input_node, new_node_1, mu, sigma, edge_innovation_count);
    }

    for (int32_t i = 0; i < (int32_t)output_edges_1.size(); i++) {
        attempt_edge_insert(new_node_1, output_edges_1[i]->output_node, mu, sigma, edge_innovation_count);
    }

    for (int32_t i = 0; i < (int32_t)input_edges_2.size(); i++) {
        attempt_edge_insert(input_edges_2[i]->input_node, new_node_2, mu, sigma, edge_innovation_count);
    }

    for (int32_t i = 0; i < (int32_t)output_edges_2.size(); i++) {
        attempt_edge_insert(new_node_2, output_edges_2[i]->output_node, mu, sigma, edge_innovation_count);
    }

    nodes.insert( upper_bound(nodes.begin(), nodes.end(), new_node_1, sort_RNN_Nodes_by_depth()), new_node_1);
    nodes.insert( upper_bound(nodes.begin(), nodes.end(), new_node_2, sort_RNN_Nodes_by_depth()), new_node_2);

    //disable the selected node and it's edges
    for (int32_t i = 0; i < (int32_t)input_edges.size(); i++) {
        input_edges[i]->enabled = false;
    }

    for (int32_t i = 0; i < (int32_t)output_edges.size(); i++) {
        output_edges[i]->enabled = false;
    }
    selected_node->enabled = false;

    return true;
}

bool RNN_Genome::merge_node(double mu, double sigma, double lstm_node_rate, int32_t &edge_innovation_count, int32_t &node_innovation_count) {
    cout << "\tattempting to merge a node!" << endl;
    vector<RNN_Node_Interface*> possible_nodes;
    for (int32_t i = 0; i < (int32_t)nodes.size(); i++) {
        if (nodes[i]->type != RNN_INPUT_NODE && nodes[i]->type != RNN_OUTPUT_NODE) possible_nodes.push_back(nodes[i]);
    }

    if (possible_nodes.size() < 2) return false;

    while (possible_nodes.size() > 2) {
        int32_t position = rng_0_1(generator) * possible_nodes.size();
        possible_nodes.erase(possible_nodes.begin() + position);
    }

    RNN_Node_Interface *n1 = possible_nodes[0];
    RNN_Node_Interface *n2 = possible_nodes[1];
    n1->enabled = false;
    n2->enabled = false;

    double new_depth = (n1->depth + n2->depth) / 2.0;

    RNN_Node_Interface *new_node = create_node(mu, sigma, lstm_node_rate, node_innovation_count, new_depth);
    nodes.insert( upper_bound(nodes.begin(), nodes.end(), new_node, sort_RNN_Nodes_by_depth()), new_node);

    vector<RNN_Edge*> merged_edges;
    for (int32_t i = 0; i < (int32_t)edges.size(); i++) {
        RNN_Edge *e = edges[i];
        
        if (e->input_innovation_number == n1->innovation_number ||
                e->input_innovation_number == n2->innovation_number ||
                e->output_innovation_number == n1->innovation_number ||
                e->output_innovation_number == n2->innovation_number) {

            e->enabled = false;
            merged_edges.push_back(e);
        }
    }

    for (int32_t i = 0; i < (int32_t)merged_edges.size(); i++) {
        if (merged_edges[i]->input_node->depth > new_node->depth) {
            cout << "\tinput node depth " << merged_edges[i]->input_node->depth << " > new node depth " << new_node->depth << endl;
            attempt_edge_insert(new_node, merged_edges[i]->output_node, mu, sigma, edge_innovation_count);

        } else if (merged_edges[i]->output_node->depth < new_node->depth) {
            cout << "\toutput node depth " << merged_edges[i]->output_node->depth << " < new node depth " << new_node->depth << endl;
            attempt_edge_insert(merged_edges[i]->input_node, new_node, mu, sigma, edge_innovation_count);

        } else {
            if (merged_edges[i]->input_innovation_number == n1->innovation_number ||
                    merged_edges[i]->input_innovation_number == n2->innovation_number) {
                //merged edge was an output edge
                cout << "\tthis was an output edge" << endl;
                attempt_edge_insert(new_node, merged_edges[i]->output_node, mu, sigma, edge_innovation_count);
            } else if (merged_edges[i]->output_innovation_number == n1->innovation_number ||
                    merged_edges[i]->output_innovation_number == n2->innovation_number) {
                //mergerd edge was an input edge
                cout << "\tthis was an input edge" << endl;
                attempt_edge_insert(merged_edges[i]->input_node, new_node, mu, sigma, edge_innovation_count);

            } else {
                cerr << "ERROR in merge node, reached statement that should never happen!" << endl;
                exit(1);
            }
        }
    }

    return false;
}


void RNN_Genome::print_graphviz(string filename) {
    ofstream outfile(filename);

    outfile << "digraph RNN {" << endl;
    outfile << "graph [pad=\"0.01\", nodesep=\"0.05\", ranksep=\"0.9\"];" << endl;

    outfile << "\t{" << endl;
    outfile << "\t\trank = source;" << endl;
    for (uint32_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->type != RNN_INPUT_NODE) continue;
        outfile << "\t\tnode" << nodes[i]->innovation_number << " [shape=box,color=green,label=\"input " << nodes[i]->innovation_number << "\\ndepth " << nodes[i]->depth << "\"];" << endl;
    }
    outfile << "\t}" << endl;
    outfile << endl;

    int32_t output_count = 0;
    int32_t input_count = 0;
    for (uint32_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->type == RNN_OUTPUT_NODE) output_count++;
        if (nodes[i]->type == RNN_INPUT_NODE) input_count++;
    }

    outfile << "\t{" << endl;
    outfile << "\t\trank = sink;" << endl;
    for (uint32_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->type != RNN_OUTPUT_NODE) continue;
        outfile << "\t\tnode" << nodes[i]->get_innovation_number() << " [shape=box,color=blue,label=\"output " << nodes[i]->innovation_number << "\\ndepth " << nodes[i]->depth << "\"];" << endl;
    }
    outfile << "\t}" << endl;
    outfile << endl;

    bool printed_first = false;

    if (input_count > 1) {
        for (uint32_t i = 0; i < nodes.size(); i++) {
            if (nodes[i]->type != RNN_INPUT_NODE) continue;

            if (!printed_first) {
                printed_first = true;
                outfile << "\tnode" << nodes[i]->get_innovation_number();
            } else {
                outfile << " -> node" << nodes[i]->get_innovation_number();
            }
        }
        outfile << " [style=invis];" << endl << endl;

        outfile << endl;
    }

    if (output_count > 1) {
        printed_first = false;
        for (uint32_t i = 0; i < nodes.size(); i++) {
            if (nodes[i]->type != RNN_OUTPUT_NODE) continue;

            if (!printed_first) {
                printed_first = true;
                outfile << "\tnode" << nodes[i]->get_innovation_number();
            } else {
                outfile << " -> node" << nodes[i]->get_innovation_number();
            }
        }
        outfile << " [style=invis];" << endl << endl;
        outfile << endl;
    }

    //draw the hidden nodes
    for (uint32_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->type != RNN_HIDDEN_NODE) continue;
        if (!nodes[i]->is_reachable()) continue;

        string color;
        if (nodes[i]->node_type == LSTM_NODE) {
            color = "orange";
        } else {
            color = "black";
        }
        outfile << "\t\tnode" << nodes[i]->get_innovation_number() << " [shape=box,color=" << color << ",label=\"node " << nodes[i]->get_innovation_number() << "\\ndepth " << nodes[i]->depth << "\"];" << endl;
    }
    outfile << endl;

    //draw the enabled edges
    for (uint32_t i = 0; i < edges.size(); i++) {
        if (!edges[i]->is_reachable()) continue;

        outfile << "\tnode" << edges[i]->get_input_node()->get_innovation_number() << " -> node" << edges[i]->get_output_node()->get_innovation_number() << " [color=blue];" << endl;
    }
    outfile << endl;

    //draw the enabled recurrent edges
    for (uint32_t i = 0; i < recurrent_edges.size(); i++) {
        if (!recurrent_edges[i]->is_reachable()) continue;

        outfile << "\tnode" << recurrent_edges[i]->get_input_node()->get_innovation_number() << " -> node" << recurrent_edges[i]->get_output_node()->get_innovation_number() << " [color=green,style=dashed];" << endl;
    }
    outfile << endl;


    outfile << "}" << endl;
    outfile.close();

}