#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <vector>

//from undvc_common
#include "arguments.hxx"

#include "image_tools/image_set.hxx"

#include "strategy/cnn_genome.hxx"

using namespace std;

vector<string> arguments;

int main(int argc, char** argv) {
    arguments = vector<string>(argv, argv + argc);

    string binary_samples_filename;
    string genome_filename;
    string output_filename;
    string checkpoint_filename;

    get_argument(arguments, "--samples_file", true, binary_samples_filename);
    get_argument(arguments, "--genome_file", true, genome_filename);
    get_argument(arguments, "--output_file", true, output_filename);
    get_argument(arguments, "--checkpoint_file", true, checkpoint_filename);

    Images images(binary_samples_filename);

    CNN_Genome *genome = NULL;

    ifstream infile(checkpoint_filename);
    if (infile) {
        //start from the checkpoint if it exists
        cout << "starting from checkpoint file: '" << checkpoint_filename << "'" << endl;

        genome = new CNN_Genome(checkpoint_filename, true);
    } else {
        //start from the input genome file otherwise
        cout << "starting from input file: '" << genome_filename << "'" << endl;
        genome = new CNN_Genome(genome_filename, false);
    }
    cout << "parsed intput file" << endl;

    genome->print_graphviz(cout);

    genome->set_checkpoint_filename(checkpoint_filename);
    genome->set_output_filename(output_filename);

    cout << "starting backpropagation!" << endl;
    genome->stochastic_backpropagation(images, true);

    return 0;
}
