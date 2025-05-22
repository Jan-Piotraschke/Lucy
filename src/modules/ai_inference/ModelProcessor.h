#ifndef MODEL_PROCESSOR_H
#define MODEL_PROCESSOR_H

#include <fstream>
#include <iostream>
#include <limits>
#include <opencv2/opencv.hpp>
#include <string>
#include <torch/script.h>
#include <vector>

class ModelProcessor
{
  public:
    ModelProcessor(const std::string& model_path, int num_steps = 40000);
    bool run();
    bool saveCSV(const std::string& filename) const;
    bool plotOutput(const std::string& filename) const;

  private:
    std::string                model_path_;
    int                        num_steps_;
    std::vector<float>         data_;
    std::vector<float>         output_col1_;
    std::vector<float>         output_col2_;
    float                      y_min_;
    float                      y_max_;
    torch::jit::script::Module module_;
    at::Tensor                 output_;

    bool loadModel();
    void prepareInput();
    bool process();
};

#endif // MODEL_PROCESSOR_H
