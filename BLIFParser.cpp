#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>
#include <algorithm>
#include <sstream>

struct Gate
{
    std::vector<std::string> inputs;//输入
    std::string output;//输出
    std::vector<std::string> truth_table;
};

class BLIFParser
{
public:
    

    bool Parse(const std::string& fileName) {
        std::fstream file(fileName);
        if (!file.is_open()) {
            std::cerr << "Failed to open the file:" << fileName << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;//跳过空白行和注释行
            if (line.starts_with(".model")) current_model = Trim(line.substr(6));
            else if (line.starts_with(".inputs")) ParseInputs(line);
            else if (line.starts_with(".outputs")) ParseOutputs(line);
            else if (line.starts_with(".names")) ParseGateDefinition(line);
            else if (line.starts_with(".end")) break;
            else {
                //真值表存入
                if (!current_gate.output.empty()) {
                    current_gate.truth_table.push_back(line);
                }
            }
        }
        gates.push_back(current_gate);
        return true;
    }
    void Print() const {
        std::cout << "Model: " << current_model << std::endl;
        std::cout << "Inputs: ";
        for (const auto& input : inputs) {
            std::cout << input << " ";
        }
        std::cout << std::endl;

        std::cout << "Outputs: ";
        for (const auto& output : outputs) {
            std::cout << output << " ";
        }
        std::cout << std::endl;

        std::cout << "Gates:" << std::endl;
        for (const auto& gate : gates) {
            std::cout << "  Inputs: ";
            for (const auto& input : gate.inputs) {
                std::cout << input << " ";
            }
            std::cout << ", Output: " << gate.output << std::endl;

            std::cout << "  Truth Table:" << std::endl;
            for (const auto& row : gate.truth_table) {
                std::cout << "    " << row << std::endl;
            }
        }
    }
	~BLIFParser();

private:
    std::string current_model;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
    std::vector<Gate> gates;
    Gate current_gate;


    //去除空格
    std::string Trim(const std::string& str) {
        if (str.empty()) {
            return str;
        }

        size_t first = str.find_first_not_of(' ');
        if (first == std::string::npos) {
            return ""; // 全部是空格
        }

        size_t last = str.find_last_not_of(' ');
        return str.substr(first, (last - first + 1));
    }

    void ParseInputs(const std::string& line) {
        std::istringstream iss(line.substr(8)); // 从第 9 个字符开始提取子字符串
        std::string input;
        while (iss >> input) { //逐个读取单词
            inputs.push_back(input); //将输入信号名称添加到 inputs 向量中
        }
    }

    void ParseOutputs(const std::string& line) {
        std::istringstream iss(line.substr(9)); //从第 10 个字符开始提取子字符串
        std::string output;
        while (iss >> output) { //同上
            outputs.push_back(output); //将输出信号名称添加到 outputs 向量中
        }
    }

    void ParseGateDefinition(const std::string& line) {
        if (!current_gate.output.empty()) {
            gates.push_back(current_gate);
        }

        current_gate = Gate();
        std::istringstream iss(line.substr(6)); //从第 7 个字符开始提取子字符串
        std::string signal;
        while (iss >> signal) {
            current_gate.inputs.push_back(signal);
        }
        current_gate.output = current_gate.inputs.back(); //最后一个信号是输出
        current_gate.inputs.pop_back(); // 从inputs 中移除输出
    }
    
};


BLIFParser::~BLIFParser()
{
}

int main() {
    BLIFParser parser;
    if (parser.Parse("test1.blif")) {
        parser.Print();
    }
    return 0;
}