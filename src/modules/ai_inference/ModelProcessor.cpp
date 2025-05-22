#include "ModelProcessor.h"

ModelProcessor::ModelProcessor(const std::string& model_path, int num_steps)
    : model_path_(model_path)
    , num_steps_(num_steps)
    , y_min_(std::numeric_limits<float>::max())
    , y_max_(std::numeric_limits<float>::lowest())
{
    data_.resize(num_steps_);
    for (int i = 0; i < num_steps_; ++i)
    {
        data_[i] = i * 0.001f;
    }
}

bool ModelProcessor::loadModel()
{
    try
    {
        module_ = torch::jit::load(model_path_);
    }
    catch (const c10::Error& e)
    {
        std::cerr << "Error loading the model: " << e.what() << "\n";
        return false;
    }
    return true;
}

void ModelProcessor::prepareInput()
{
    auto          options      = torch::TensorOptions().dtype(torch::kFloat32);
    torch::Tensor input_tensor = torch::from_blob(data_.data(), {num_steps_, 1}, options).clone();
    output_                    = module_.forward({input_tensor}).toTensor();
}

bool ModelProcessor::process()
{
    auto sizes = output_.sizes();
    if (sizes.size() != 2 || sizes[0] != num_steps_)
    {
        std::cerr << "Unexpected output tensor shape\n";
        return false;
    }

    output_col1_.resize(num_steps_);
    output_col2_.resize(num_steps_);

    for (int64_t i = 0; i < sizes[0]; ++i)
    {
        float val1      = output_[i][0].item<float>();
        float val2      = output_[i][1].item<float>();
        output_col1_[i] = val1;
        output_col2_[i] = val2;
        y_min_          = std::min({y_min_, val1, val2});
        y_max_          = std::max({y_max_, val1, val2});
    }

    return true;
}

bool ModelProcessor::run()
{
    return loadModel() && (prepareInput(), process());
}

bool ModelProcessor::saveCSV(const std::string& filename) const
{
    std::ofstream outfile(filename);
    if (!outfile.is_open())
    {
        std::cerr << "Failed to open file: " << filename << "\n";
        return false;
    }

    for (int i = 0; i < num_steps_; ++i)
    {
        outfile << output_col1_[i] << "," << output_col2_[i] << "\n";
    }

    outfile.close();
    return true;
}

bool ModelProcessor::plotOutput(const std::string& filename) const
{
    const int width  = 1024;
    const int height = 768;
    cv::Mat   plot(height, width, CV_8UC3, cv::Scalar(255, 255, 255));

    auto map_x = [=](float x)
    {
        return static_cast<int>((x / 40.0f) * width);
    };
    auto map_y = [=](float y)
    {
        return static_cast<int>(height - ((y - y_min_) / (y_max_ - y_min_)) * height);
    };

    for (int i = 1; i < num_steps_; ++i)
    {
        cv::line(
            plot,
            cv::Point(map_x(data_[i - 1]), map_y(output_col1_[i - 1])),
            cv::Point(map_x(data_[i]), map_y(output_col1_[i])),
            cv::Scalar(0, 0, 255),
            1);
        cv::line(
            plot,
            cv::Point(map_x(data_[i - 1]), map_y(output_col2_[i - 1])),
            cv::Point(map_x(data_[i]), map_y(output_col2_[i])),
            cv::Scalar(255, 0, 0),
            1);
    }

    // Gridlines
    int grid_spacing_x = 100;
    int grid_spacing_y = 100;
    for (int x = 0; x < width; x += grid_spacing_x)
        cv::line(plot, cv::Point(x, 0), cv::Point(x, height), cv::Scalar(240, 240, 240), 1);
    for (int y = 0; y < height; y += grid_spacing_y)
        cv::line(plot, cv::Point(0, y), cv::Point(width, y), cv::Scalar(240, 240, 240), 1);

    // Axis
    cv::line(plot, cv::Point(0, height - 1), cv::Point(width, height - 1), cv::Scalar(0, 0, 0), 1);
    cv::line(plot, cv::Point(0, 0), cv::Point(0, height), cv::Scalar(0, 0, 0), 1);

    // Legends
    cv::putText(
        plot,
        "Red: Col1",
        cv::Point(20, 30),
        cv::FONT_HERSHEY_SIMPLEX,
        0.5,
        cv::Scalar(0, 0, 255),
        1);
    cv::putText(
        plot,
        "Blue: Col2",
        cv::Point(20, 50),
        cv::FONT_HERSHEY_SIMPLEX,
        0.5,
        cv::Scalar(255, 0, 0),
        1);

    if (!cv::imwrite(filename, plot))
    {
        std::cerr << "Failed to write plot image to " << filename << "\n";
        return false;
    }

    return true;
}
