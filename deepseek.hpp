#ifndef DEEPSEEK_H_INCLUDE
#define DEEPSEEK_H_INCLUDE
#include <string>
#include <vector>
#include <memory>
#include <curl/curl.h>
#include <cjson/cJSON.h>

class DeepSeek;

class NetWork{
private:
        CURL* curl;
        struct curl_slist* head;
        std::string api_key;
public:
        NetWork();
        int send(const std::string& model,
                const std::string& thinking_type,
                const std::string& message);
        ~NetWork();
};

class Parse{
public:
        void parse_and_run(DeepSeek& ds,
                const std::string& input);
};

class Conversation{
private:
        cJSON* current;
        std::string key_word;
public:
        Conversation(const std::string& message,
                const std::string& key_word);
        void print_key_word();
        bool compare_key_word(const std::string& referance);
        void push_back_message(const std::string& message,
                const std::string& role);
        std::string get_current_str();
        std::string get_key_word();
        ~Conversation();
};

class Memory{
private:
        int current_conversation;
        std::vector<std::unique_ptr<Conversation>> all_conversation;
public:
        Memory();
        std::string get_current_str();
        void new_conversation(const std::string& system_messages,
                const std::string& key_word);
        void del_conversation(const std::string& key_word);
        void switch_conversation(const std::string& key_word);
        void print_history();
        void print_all_key_word();
        void push_back_message(const std::string& message,
                const std::string& role);
        ~Memory();
};

class RunTimeTest{
public:
        RunTimeTest();
        ~RunTimeTest();
};

class DeepSeek{
private:
        RunTimeTest test;
        std::string model;
        std::string thinking_type;
        NetWork network;
        Memory memory;
        Parse parse;
public:
        DeepSeek();
        void input(const std::string& str);
        friend Parse;
};

#endif