#ifndef CNN_GENOME_H
#define CNN_GENOME_H

#include <random>
using std::mt19937;
using std::uniform_int_distribution;
using std::uniform_real_distribution;

#include <vector>
using std::vector;

#include "image_tools/image_set.hxx"
#include "cnn_node.hxx"
#include "cnn_edge.hxx"

#define SANITY_CHECK_BEFORE_INSERT 0
#define SANITY_CHECK_AFTER_GENERATION 1

class CNN_Genome {
    private:
        vector<CNN_Node*> nodes;
        vector<CNN_Edge*> edges;

        CNN_Node *input_node;
        vector<CNN_Node*> softmax_nodes;

        mt19937 generator;
        uniform_real_distribution<double> rng_double;
        uniform_int_distribution<long> rng_long;

        double initial_mu;
        double mu;
        int epoch;
        int epochs;


        double best_error;
        int best_predictions;
        int best_predictions_epoch;
        int best_error_epoch;

        vector<double> best_class_error;
        vector<int> best_correct_predictions;

        bool started_from_checkpoint;
        vector<long> backprop_order;

        int generation_id;
        string name;
        string checkpoint_filename;
        string output_filename;

    public:
        /**
         *  Initialize a genome from a file
         */
        CNN_Genome(string filename, bool is_checkpoint);

        /**
         *  Iniitalize a genome from a set of nodes and edges
         */
        CNN_Genome(int _generation_id, int seed, int _epochs, const vector<CNN_Node*> &_nodes, const vector<CNN_Edge*> &_edges);

        ~CNN_Genome();

        void print_best_error(ostream &out) const;
        void print_best_predictions(ostream &out) const;

        int get_generation_id() const;
        double get_fitness() const;
        int get_best_error_epoch() const;
        int get_best_predictions() const;
        int get_epoch() const;
        int get_total_epochs() const;
        int get_number_enabled_edges() const;

        bool sanity_check(int type) const;
        bool outputs_connected() const;

        const vector<CNN_Node*> get_nodes() const;
        const vector<CNN_Edge*> get_edges() const;

        CNN_Node* get_node(int node_position);
        CNN_Edge* get_edge(int edge_position);

        int get_number_edges() const;
        int get_number_nodes() const;
        int get_number_softmax_nodes() const;

        void add_node(CNN_Node* node);
        void add_edge(CNN_Edge* edge);
        bool disable_edge(int edge_position);

        void resize_edges_around_node(int node_position);
 
        int evaluate_image(const Image &image, vector<double> &class_error, bool do_backprop);

        void initialize_weights();
        void initialize_bias();

        void save_weights();
        void save_bias();
        void stochastic_backpropagation(const Images &images, bool reset_weights);

        void set_name(string _name);
        void set_output_filename(string _output_filename);
        void set_checkpoint_filename(string _checkpoint_filename);

        void write(ostream &outfile);
        void write_to_file(string filename);

        void read(istream &infile);

        void print_graphviz(ostream &out) const;
};


struct sort_genomes_by_fitness {
    bool operator()(CNN_Genome *g1, CNN_Genome *g2) {
        return g1->get_fitness() < g2->get_fitness();
    }   
};


#endif
