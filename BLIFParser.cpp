#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>
#include <algorithm>
#include <sstream>
#include <set>
#include <map>

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


#pragma region 转换部分

    /// <summary>
    /// .v文件转化
    /// </summary>
    /// <param name="verilogName"></param>
    void GenerateVerilog(const std::string& verilogName) const {
        std::ofstream verilogFile(verilogName);
        if (!verilogFile.is_open()) {
            std::cerr << "Failed to open the verilog file" << verilogName << std::endl;
            return;
        }

        //模块名及后面所在一行
        verilogFile << "module " << current_model << "(clk, rst,";
        for (size_t i = 0; i < outputs.size(); i++) {
            verilogFile << " " << outputs[i];
            if (i != outputs.size() - 1 || !inputs.empty()) {
                verilogFile << ",";
            }
        }
        for (size_t i = 0; i < inputs.size(); i++) {
            verilogFile << " " << inputs[i];
            if (i != inputs.size() - 1) {
                verilogFile << ",";
            }
            else {
                verilogFile << ");" << std::endl;
            }
        }

        verilogFile << "input clk, rst;" << std::endl;

        for (const auto& output : outputs) {
            verilogFile << "output " << output << ";" << std::endl;
        }
        for (const auto& input : inputs) {
            verilogFile << "input " << input << ";" << std::endl;
        }
        verilogFile << '\n';

        std::set<std::string> setter;
        std::vector<std::string> temp;
        std::vector<std::string> tempGateOutputs;

        for (const auto& g : gates) {
            tempGateOutputs.push_back(g.output);
        }

        //存入不重复的wire
        for (const auto& input : inputs) {
            temp.push_back(input);
            setter.insert(input);
        }
        for (const auto& output : outputs) {
            temp.push_back(output);
            setter.insert(output);
        }
        for (const auto& gateOutput : tempGateOutputs) {
            if (setter.find(gateOutput) == setter.end()) {
                temp.push_back(gateOutput);
                setter.insert(gateOutput);
            }
        }

        for (const auto& t : temp) {
            verilogFile << "wire " << t << ";" << std::endl;
        }
        verilogFile << '\n';

        for (const auto& g : gates) {
            verilogFile << "assign " << g.output << " = ";
            bool isFirstLine = true;

            for (const auto& t : g.truth_table) {
                std::istringstream iss(t);
                std::string inputValues, outputValue;
                iss >> inputValues >> outputValue;

                if (outputValue == "1") {
                    if (!isFirstLine) verilogFile << " | ";
                    bool isFirstInput = true;
                    for (size_t i = 0; i < g.inputs.size(); i++) {
                        if (inputValues[i] == '1') {
                            if (!isFirstInput) verilogFile << " & ";
                            verilogFile << g.inputs[i];
                            isFirstInput = false;
                        }
                        else if (inputValues[i] == '0') {
                            if (!isFirstInput)verilogFile << "&";
                            verilogFile << "!" << g.inputs[i];
                            isFirstInput = false;
                        }
                        else {
                            continue;//为“-”的情况
                        }
                    }
                }
                isFirstLine = false;
            }
            verilogFile << ";" << std::endl;
        }

        verilogFile << '\n';
        verilogFile << "endmodule" << std::endl;

        verilogFile.close();
    }
    /// <summary>
    /// .dot文件转换
    /// </summary>
    /// <param name="dotFileName"></param>
    void GenerateDotFile(const std::string& dotFileName) const {
        std::ofstream dotFile(dotFileName);

        if (!dotFile.is_open()) {
            std::cerr << "Failed to create the DOT file: " << dotFileName << std::endl;
            return;
        }

        std::vector<std::string> allNodes;
        std::map<std::string, size_t> nodeIndexMap;

        // 收集所有唯一的节点
        for (const auto& input : inputs) {
            allNodes.push_back(input);
            nodeIndexMap[input] = allNodes.size() - 1;
        }
        for (const auto& output : outputs) {
            if (nodeIndexMap.find(output) == nodeIndexMap.end()) {
                allNodes.push_back(output);
                nodeIndexMap[output] = allNodes.size() - 1;
            }
        }
        for (const auto& gate : gates) {
            if (nodeIndexMap.find(gate.output) == nodeIndexMap.end()) {
                allNodes.push_back(gate.output);
                nodeIndexMap[gate.output] = allNodes.size() - 1;
            }
        }

        dotFile << "digraph " << current_model << " {" << std::endl;
        dotFile << "rankdir=\"LR\"" << std::endl; // 从左到右布局
        dotFile << "node [shape=\"house\" orientation=\"270\"]" << std::endl;
        dotFile << "size=\"10,7.5\"" << std::endl;

        // 定义输入节点
        for (size_t i = 0; i < inputs.size(); ++i) {
            dotFile << "g" << i << " [label=\"" << i << ":" << inputs[i] << "\" style=filled color=palegreen1 shape=house orientation=-90]\n";
            dotFile << "g" << i << "o [shape=point]\n";
            dotFile << "g" << i << " -> g" << i << "o [arrowhead=none arrowtail=none]\n";
        }

        // 定义输出节点及其相关边
        for (size_t i = 0; i < outputs.size(); ++i) {
            size_t index = nodeIndexMap[outputs[i]];
            dotFile << "g" << index << " [label=\"" << index << ":" << outputs[i] << "\" shape=house orientation=-90 style=filled color=pink1]\n";

            // 查找输出节点对应的中间节点
            for (const auto& gate : gates) {
                if (gate.output == outputs[i]) {
                    for (size_t k = 0; k < gate.inputs.size(); ++k) {
                        auto inputIndex = nodeIndexMap[gate.inputs[k]];
                        dotFile << "g" << inputIndex << "o -> g" << index << " [arrowhead=";
                        if (gate.truth_table[0].starts_with("1")) dotFile << "none]";
                        else if (gate.truth_table[0].starts_with("0")) dotFile << "odot]";
                        dotFile << std::endl;
                    }
                }
            }
        }

        // 定义中间节点及边
        for (size_t i = 0; i < gates.size(); ++i) {
            if (std::find(outputs.begin(), outputs.end(), gates[i].output) == outputs.end()) {
                size_t index = nodeIndexMap[gates[i].output];
                dotFile << "g" << index << " [label=\"" << index << ":" << gates[i].output << "\"]\n";
                dotFile << "g" << index << "o [shape=point]\n";
                dotFile << "g" << index << " -> g" << index << "o [arrowhead=none arrowtail=";
                if (gates[i].truth_table.size() > 1)dotFile << "odot]" << std::endl;
                else dotFile << "none]" << std::endl;

                for (const auto& input : gates[i].inputs) {
                    size_t inputIndex = nodeIndexMap[input];
                    dotFile << "g" << inputIndex << "o-> g" << index << " [arrowhead=";
                    if (gates[i].truth_table.size() > 1)dotFile << "odot]" << std::endl;
                    else dotFile << "none]" << std::endl;
                }
            }
        }


        dotFile << "}" << std::endl;
        dotFile.close();
    }

#pragma endregion

#pragma region  外部获取函数

    std::vector<std::string> GetInputs() {
        if (inputs.size() <= 0) std::cerr << "Inputs contains nothing." << std::endl;
        return inputs;
    }
    std::vector<std::string> GetOutputs() {
        if (outputs.size() <= 0) std::cerr << "Outputs contains nothing." << std::endl;
        return outputs;
    }
    std::vector<Gate> GetGates() {
        if (gates.size() <= 0)std::cerr << "Gates contains nothing." << std::endl;
        return gates;
    }

#pragma endregion

	~BLIFParser();

private:
    std::string current_model;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
    std::vector<Gate> gates;
    Gate current_gate;

#pragma region 数据存入部分

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

#pragma endregion

    
};


BLIFParser::~BLIFParser()
{
}

int main() {
    BLIFParser parser;
    if (parser.Parse("test1.blif")) {
        parser.Print();

        parser.GenerateVerilog("TestDoc.v");
        parser.GenerateDotFile("TestDoc.dot");
    }
    return 0;
}