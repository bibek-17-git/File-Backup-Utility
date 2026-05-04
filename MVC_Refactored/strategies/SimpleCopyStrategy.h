#pragma once
#include <string>
#include <fstream>

class IBackupStrategy {
public:
    virtual ~IBackupStrategy() = default;
    virtual bool copyFile(const std::string& src, const std::string& dest) = 0;
    virtual std::string getStrategyName() const = 0;
};

class SimpleCopyStrategy : public IBackupStrategy {
public:
    bool copyFile(const std::string& src, const std::string& dest) override {
        std::ifstream in(src, std::ios::binary);
        std::ofstream out(dest, std::ios::binary);
        if (!in.is_open() || !out.is_open()) return false;
        out << in.rdbuf();
        return out.good();
    }
    
    std::string getStrategyName() const override { return "SimpleCopy"; }
};
