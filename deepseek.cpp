#include <cstddef>
#include <endian.h>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <isocline.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include "deepseek.hpp"
#define DEEPSEEK_URL "https://api.deepseek.com/chat/completions"
#define DEEPSEEK_CONFIG "/.deepseek/config"
#define DEEPSEEK_HISTORY "/.deepseek/history"
#define DEEPSEEK_LOCK "/.deepseek/lock"

class response
{
private:
    std::string str;

    void parse_event(const std::string& event)
    {
        if (event.size() < 6)
            return;
        if (event.compare(0, 6, "data: ") != 0)
            return;
        const char* data = event.c_str() + 6;
        if (strcmp(data, "[DONE]") == 0)
            return;
        cJSON* json = cJSON_Parse(data);
        if (!json)
            return;
        cJSON* choices =
            cJSON_GetObjectItem(json, "choices");
        cJSON* choice =
            cJSON_GetArrayItem(choices, 0);
        cJSON* delta =
            cJSON_GetObjectItem(choice, "delta");
        cJSON* reasoning =
            cJSON_GetObjectItem(delta, "reasoning_content");
        cJSON* content =
            cJSON_GetObjectItem(delta, "content");
        if (cJSON_IsString(reasoning) &&
            reasoning->valuestring &&
            reasoning->valuestring[0] != '\0'){
            std::cout << reasoning->valuestring;
        }

        if (cJSON_IsString(content) &&
            content->valuestring &&
            content->valuestring[0] != '\0'){
            if (answer.empty())
                std::cout << "\n\033[31m回答:\033[0m\n";

            std::cout << content->valuestring;
            answer.append(content->valuestring);
        }
        std::cout.flush();
        cJSON_Delete(json);
    }

    void show()
    {
        size_t pos;

        while ((pos = str.find("\n\n")) != std::string::npos) {
            std::string event = str.substr(0, pos);
            str.erase(0, pos + 2);
            parse_event(event);
        }
    }

public:
    std::string answer;

    response()
    {
    }

    void append(const char* data, size_t len)
    {
        str.append(data, len);
        show();
    }
};

static std::string read_file(const char* file_path);
static size_t call_back(char* data,size_t n,size_t size,void* usr);

RunTimeTest::RunTimeTest()
{
        FILE* fp;
        std::string config_path = getenv("HOME");
        std::string history_path = getenv("HOME");
        std::string lock_path = getenv("HOME");
        cJSON *empty_arr,*config_json,*index;
        char* buffer;
        size_t f_size;

        std::cout << "\033[34mDeepSeek\033[0m CLI" << std::endl;
        config_path += DEEPSEEK_CONFIG;
        history_path += DEEPSEEK_HISTORY;
        lock_path += DEEPSEEK_LOCK;
        if ((fp = fopen(config_path.c_str(),"r")) == NULL){
                std::cout << "\033[31m请使用deepseek-config配置！\033[0m" << std::endl;
                exit(EXIT_FAILURE);
        }
        fseek(fp,0L,SEEK_END);
        f_size = ftell(fp);
        if (f_size == 0){
                std::cout << "\033[31m配置文件格式错误！\033[0m" << std::endl;
                exit(EXIT_FAILURE);
        }
        buffer = new char[f_size+1];
        fseek(fp,0L,SEEK_SET);
        fread(buffer,f_size,1,fp);
        buffer[f_size] = '\0';
        config_json = cJSON_Parse(buffer);
        delete[] buffer;
        index = cJSON_GetObjectItem(config_json,"api_key");
        if (!cJSON_IsString(index) || !index->valuestring){
                std::cout << "\033[31m配置文件损坏！\033[0m" << std::endl;
                cJSON_Delete(config_json);
                exit(EXIT_FAILURE);
        }
        index = cJSON_GetObjectItem(config_json,"model");
        if (!cJSON_IsString(index) || !index->valuestring){
                std::cout << "\033[31m配置文件损坏！\033[0m" << std::endl;
                cJSON_Delete(config_json);
                exit(EXIT_FAILURE);
        }
        index = cJSON_GetObjectItem(config_json,"thinking_type");
        if (!cJSON_IsString(index) || !index->valuestring){
                std::cout << "\033[31m配置文件损坏！\033[0m" << std::endl;
                cJSON_Delete(config_json); 
                exit(EXIT_FAILURE);
        }
        cJSON_Delete(config_json);
        fclose(fp);
        if ((fp = fopen(lock_path.c_str(),"r")) != NULL){
                std::cout << "\033[31m其他进程正在使用DeepSeek CLI\033[0m" << std::endl;
                fclose(fp);
                exit(EXIT_FAILURE);
        }
        if ((fp = fopen(history_path.c_str(),"r")) == NULL){
                char* temp;
                fp = fopen(history_path.c_str(),"w");
                empty_arr = cJSON_CreateArray();
                temp = cJSON_Print(empty_arr);
                fwrite(temp,strlen(temp)+1,1,fp);
                cJSON_Delete(empty_arr);
                free(temp);
        }
        fclose(fp);
        fp = fopen(lock_path.c_str(),"w");
        fclose(fp);
}

RunTimeTest::~RunTimeTest()
{
        std::string lock_path = getenv("HOME");

        lock_path += DEEPSEEK_LOCK;
        remove(lock_path.c_str());
}

NetWork::NetWork()
{
        std::string config_path = getenv("HOME");
        cJSON* config_json;
        cJSON* temp;
        FILE* fp;
        char* buffer;
        size_t f_size;
        std::string http_author_head = "Authorization: Bearer ";

        response_data = new response();
        curl_global_init(CURL_GLOBAL_ALL);
        config_path += DEEPSEEK_CONFIG;
        this->head = NULL;
        fp = fopen(config_path.c_str(),"r");
        fseek(fp,0L,SEEK_END);
        f_size = ftell(fp);
        buffer = new char[f_size+1];
        fseek(fp,0L,SEEK_SET);
        fread(buffer,f_size,1,fp);
        buffer[f_size] = '\0';
        config_json = cJSON_Parse(buffer);
        temp = config_json;
        config_json = cJSON_GetObjectItem(config_json,"api_key");
        this->api_key = config_json->valuestring;
        cJSON_Delete(temp);
        http_author_head += this->api_key;
        this->curl = curl_easy_init();
        this->head = curl_slist_append(this->head,"Content-Type: application/json");
        this->head = curl_slist_append(this->head,http_author_head.c_str());
        curl_easy_setopt(this->curl,CURLOPT_POST,1L);
        curl_easy_setopt(this->curl,CURLOPT_URL,DEEPSEEK_URL);
        curl_easy_setopt(this->curl,CURLOPT_HTTPHEADER,this->head);
        curl_easy_setopt(this->curl,CURLOPT_WRITEFUNCTION,call_back);
        curl_easy_setopt(this->curl,CURLOPT_WRITEDATA,response_data);
        //curl_easy_setopt(this->curl,CURLOPT_VERBOSE,1L);
        fclose(fp);
        delete[] buffer;
}

int NetWork::send(const std::string& model,
        const std::string& thinking_type,
        const std::string& message)
{
        CURLcode code;
        cJSON* send_json = cJSON_CreateObject();
        cJSON* thinking = cJSON_CreateObject();
        char* send_str;
        long http_code;

        cJSON_AddStringToObject(send_json,"model",model.c_str());
        cJSON_AddStringToObject(thinking,"type",thinking_type.c_str());
        cJSON_AddItemToObject(send_json,"thinking",thinking);
        cJSON_AddItemToObject(send_json,"messages",cJSON_Parse(message.c_str()));
        cJSON_AddBoolToObject(send_json,"stream",1);
        send_str = cJSON_Print(send_json);
        curl_easy_setopt(this->curl,CURLOPT_POSTFIELDS,send_str);
        code = curl_easy_perform(this->curl);
        free(send_str);
        if (code == CURLE_COULDNT_CONNECT)
                fprintf(stderr,"\033[31m无法连接服务器\n\033[0m");
        if (code != CURLE_OK)
                return -1;
        curl_easy_getinfo(this->curl,CURLINFO_RESPONSE_CODE,&http_code);
        switch (http_code){
        case 400:
                fprintf(stderr,"\033[31m请求体格式错误\n\033[0m");
                break;
        case 401:
                fprintf(stderr,"\033[31mAPI Key错误\n\033[0m");
                break;
        case 402:
                fprintf(stderr,"\033[31m余额不足\n\033[0m");
                break;
        case 422:
                fprintf(stderr,"\033[31m请求体参数错误\n\033[0m");
                break;
        case 429:
                fprintf(stderr,"\033[0m请求过快\n\033[0m");
                break;
        case 500:
                fprintf(stderr,"\033[31m服务器故障\n\033[0m");
                break;
        case 503:
                fprintf(stderr,"\033[31m服务器繁忙\n\033[0m");
        }
        if (http_code != 200){
                cJSON_Delete(send_json);
                return -1;
        }
        cJSON_Delete(send_json);
        
        return 0;
}

NetWork::~NetWork()
{
        curl_slist_free_all(this->head);
        curl_easy_cleanup(this->curl);
        curl_global_cleanup();
        delete response_data;
}

Conversation::Conversation(const std::string& message,const std::string& key_word)
{
        this->key_word = key_word;
        this->current = cJSON_Parse(message.c_str());
}

void Conversation::print_key_word()
{
        std::cout << this->key_word;
}

bool Conversation::compare_key_word(const std::string& referance)
{
        if (this->key_word == referance)
                return true;

        return false;
}

void Conversation::push_back_message(const std::string& message,
        const std::string& role)
{
        cJSON* new_message = cJSON_CreateObject();
        cJSON_AddStringToObject(new_message,"role",role.c_str());
        cJSON_AddStringToObject(new_message,"content",message.c_str());
        cJSON_AddItemToArray(this->current,new_message);
}

std::string Conversation::get_current_str()
{
        char* current_str = cJSON_Print(this->current);
        std::string temp = current_str;
        free(current_str);

        return temp;
}

std::string Conversation::get_key_word()
{
        return this->key_word;
}

Conversation::~Conversation()
{
        cJSON_Delete(this->current);
}

Memory::Memory()
{
        std::string history_path = getenv("HOME");
        cJSON* all_message;
        FILE* fp;
        char* buffer;
        size_t f_size;
        int arr_size;

        history_path += DEEPSEEK_HISTORY;
        fp = fopen(history_path.c_str(),"r");
        fseek(fp,0L,SEEK_END);
        f_size = ftell(fp);
        fseek(fp,0L,SEEK_SET);
        buffer = new char[f_size+1];
        fread(buffer,f_size,1,fp);
        buffer[f_size] = '\0';
        all_message = cJSON_Parse(buffer);
        arr_size = cJSON_GetArraySize(all_message);
        if (arr_size == 0){
                cJSON* new_conversation = cJSON_CreateObject();
                cJSON* new_messages_arr = cJSON_CreateArray();
                cJSON* new_msg = cJSON_CreateObject();

                cJSON_AddStringToObject(new_conversation,"key_word","默认");
                cJSON_AddItemToObject(new_conversation,"messages",new_messages_arr);
                cJSON_AddItemToArray(new_messages_arr,new_msg);
                cJSON_AddStringToObject(new_msg,"role","system");
                cJSON_AddStringToObject(new_msg,"content","你是一个乐于助人的助手");
                cJSON_AddItemToArray(all_message,new_conversation);
        }
        arr_size = cJSON_GetArraySize(all_message);
        for (int i = 0;i < arr_size;i++){
                cJSON* temp;
                cJSON* temp_arr;
                cJSON* temp_key_word;
                char* arr_str;
                temp = cJSON_GetArrayItem(all_message,i);
                temp_arr = cJSON_GetObjectItem(temp,"messages");
                temp_key_word = cJSON_GetObjectItem(temp,"key_word");
                arr_str = cJSON_Print(temp_arr);
                this->all_conversation.push_back(std::make_unique<Conversation>(arr_str,temp_key_word->valuestring));
                free(arr_str);
        }
        this->current_conversation = 0;
        delete[] buffer;
        cJSON_Delete(all_message);
}

std::string Memory::get_current_str()
{
        std::string temp = this->all_conversation[current_conversation]->get_current_str();
        
        return temp;
}

void Memory::new_conversation(const std::string& system_messages,
        const std::string& key_word)
{
        cJSON* new_message_arr = cJSON_CreateArray();
        cJSON* new_msg = cJSON_CreateObject();
        char* new_messages_str;
        std::string temp;

        for (std::unique_ptr<Conversation>& i : this->all_conversation){
                if (i->compare_key_word(key_word)){
                        std::cout << "\033[31m关键字冲突！\033[0m" << std::endl;
                        return;
                }
        }
        cJSON_AddStringToObject(new_msg,"role","system");
        cJSON_AddStringToObject(new_msg,"content",system_messages.c_str());
        cJSON_AddItemToArray(new_message_arr,new_msg);
        new_messages_str = cJSON_Print(new_message_arr);
        temp = new_messages_str;
        this->all_conversation.push_back(std::make_unique<Conversation>(temp,key_word));
        this->switch_conversation(key_word);
        std::cout << "\033[31m已创建新对话并切换\033[0m" << std::endl;
        free(new_messages_str);
        cJSON_Delete(new_message_arr);
}

void Memory::del_conversation(const std::string& key_word)
{
        int index = 0;

        for (std::unique_ptr<Conversation>& i : this->all_conversation){
                if (i->compare_key_word(key_word)){
                        if (index == this->current_conversation){
                                std::cout << "\033[31m切换至第一个对话...\033[0m" << std::endl;
                                this->current_conversation = 0;
                        }else if (index < this->current_conversation)
                                this->current_conversation--;
                        this->all_conversation.erase(this->all_conversation.begin()+index);std::cout << "\033[31m删除成功\033[0m" << std::endl;
                        if (this->all_conversation.size() == 0)
                                this->new_conversation("你是一个乐于助人的助手","默认");
                        return;
                }
                index++;
        }
        std::cout << "\033[31m没找到关键字为" << key_word << "的对话\033[0m" << std::endl;
}

void Memory::switch_conversation(const std::string& key_word)
{
        int index = 0;

        for (std::unique_ptr<Conversation>& i : this->all_conversation){
                if (i->compare_key_word(key_word)){
                        this->current_conversation = index;
                        std::cout << "\033[31m切换成功\033[0m" << std::endl;
                        return;
                }
                index++;
        }
        std::cout << "\033[31m没找到关键字为" << key_word << "的对话\033[0m" << std::endl;
}

void Memory::print_history()
{
        cJSON* arr;
        cJSON* index;
        cJSON* content;

        arr = cJSON_Parse(this->all_conversation[current_conversation]->get_current_str().c_str());
        for (int i = 1;i < cJSON_GetArraySize(arr);i++){
                index = cJSON_GetArrayItem(arr,i);
                content = cJSON_GetObjectItem(index,"role");
                std::cout << content->valuestring << ":";
                content = cJSON_GetObjectItem(index,"content");
                std::cout << content->valuestring << std::endl;
        }
        cJSON_Delete(arr);
}

void Memory::push_back_message(const std::string& message,const std::string& role)
{
        this->all_conversation[current_conversation]->push_back_message(message,role);
}

void Memory::print_all_key_word()
{
        std::string temp;
        int index = 1;

        for (std::unique_ptr<Conversation>& i : this->all_conversation){
                temp = i->get_key_word();
                std::cout << index << ":" << temp << std::endl;
                index++;
        }
}

Memory::~Memory()
{
        std::string history_path = getenv("HOME");
        FILE* fp;
        cJSON* all_message;
        cJSON* index;
        cJSON* temp;
        char* str;

        history_path += DEEPSEEK_HISTORY;
        all_message = cJSON_CreateArray();
        for (std::unique_ptr<Conversation>& i : this->all_conversation){
                index = cJSON_CreateObject();
                cJSON_AddStringToObject(index,"key_word",i->get_key_word().c_str());
                temp = cJSON_Parse(i->get_current_str().c_str());
                cJSON_AddItemToObject(index,"messages",temp);
                cJSON_AddItemToArray(all_message,index);
        }
        fp = fopen(history_path.c_str(),"w");
        str = cJSON_Print(all_message);
        fwrite(str,strlen(str)+1,1,fp);
        cJSON_Delete(all_message);
        free(str);
}

void Parse::parse_and_run(DeepSeek& ds,const std::string& input)
{
        int temp;
        char* buffer;

        if (input == "$>exit"){
                throw "exit";
                return;
        }else if (input == "$>msg"){
                ds.memory.print_history();
                return;
        }else if (input == "$>del"){
                std::string temp;
                ds.memory.print_all_key_word();
                buffer = ic_readline("输入关键字>");
                temp = buffer;
                free(buffer);
                ds.memory.del_conversation(temp);
                return;
        }else if (input == "$>new"){
                std::string key_word;
                std::string system_message;
                buffer = ic_readline("请输入新对话的关键字>");
                key_word = buffer;
                free(buffer);
                buffer = ic_readline("请输入新对话中DeepSeek的设定>");
                system_message = buffer;
                free(buffer);
                ds.memory.new_conversation(system_message,key_word);
                return;
        }else if (input == "$>switch"){
                std::string key_word;
                ds.memory.print_all_key_word();
                buffer = ic_readline("请输入关键字>");
                key_word = buffer;
                free(buffer);
                ds.memory.switch_conversation(key_word);
                return;
        }else if (input == "$>all"){
                ds.memory.print_all_key_word();
                return;
        }else if (!strncmp(input.c_str(),"$>file",6)){
                std::string temp;
                char path[256];
                if (sscanf(input.c_str(),"%*s %255s",path) != 1){
                        std::cout << "\033[31m指令格式不正确\033[0m" << std::endl;
                        return;
                };
                path[255] = '\0';
                temp = path;
                temp += ":\n";
                temp += read_file(path);
                ds.memory.push_back_message(temp,"user");
        }else if (input == "$>help"){
                printf("\033[31m$>help显示此帮助\n");
                printf("$>exit退出\n$>new创建新对话\n$>all显示所有对话的关键字\n");
                printf("$>switch选择对话\n$>del删除对话\n$>file <path> <message>上传文件(路径长度小于256字节)\n\033[0m");
                return;
        }else if (!strncmp(input.c_str(),"$>",2)){
                std::cout << "\033[31m错误指令\033[0m" << std::endl;
                return;
        }
        std::cout << "\033[31m正在思考中\033[0m" << std::endl;
        ds.memory.push_back_message(input,"user");
        if (ds.thinking_type == "enabled")
                std::cout << "\033[31m思考过程\033[0m" << std::endl;
        temp = ds.network.send(ds.model,ds.thinking_type,ds.memory.get_current_str());
        if (temp != 0)
                return;
        std::cout << std::endl;
        ds.memory.push_back_message(ds.network.response_data->answer,"assistant");
        ds.network.response_data->answer = "";
}

DeepSeek::DeepSeek()
{
        std::string config_path = getenv("HOME");
        FILE* fp;
        cJSON* config_json;
        cJSON* index;
        char* buffer;
        size_t f_size;

        config_path += DEEPSEEK_CONFIG;
        fp = fopen(config_path.c_str(),"r");
        fseek(fp,0L,SEEK_END);
        f_size = ftell(fp);
        fseek(fp,0L,SEEK_SET);
        buffer = new char[f_size+1];
        fread(buffer,f_size,1,fp);
        buffer[f_size] = '\0';
        config_json = cJSON_Parse(buffer);
        delete[] buffer;
        fclose(fp);
        index = cJSON_GetObjectItem(config_json,"model");
        this->model = index->valuestring;
        index = cJSON_GetObjectItem(config_json,"thinking_type");
        this->thinking_type = index->valuestring;
        cJSON_Delete(config_json);
        std::cout << "当前模型: " << this->model << std::endl;
        std::cout << "思考模式: " << ((this->thinking_type == "enabled")?"开":"关") << std::endl;
        this->memory.print_history();
}

void DeepSeek::input(const std::string& str)
{
        this->parse.parse_and_run(*this,str);
}

static std::string read_file(const char* file_path)
{
        FILE* fp;
        size_t fsize;
        std::string temp;
        char* buffer;

        if ((fp = fopen(file_path,"r")) == NULL)
                return "文件不存在";
        fseek(fp,0L,SEEK_END);
        fsize = ftell(fp);
        fseek(fp,0L,SEEK_SET);
        buffer = (char*)malloc(fsize+1);
        fread(buffer,fsize+1,1,fp);
        buffer[fsize] = '\0';
        if (fsize >= 3 && \
		       	((unsigned char)buffer[0] == 0xEF && \
			 (unsigned char)buffer[1] == 0xBB && \
			 (unsigned char)buffer[2] == 0xBF))
                temp = buffer + 3;
        else
                temp = buffer;
        free(buffer);
        fclose(fp);
        return temp;
}

static size_t call_back(char* data,size_t n,size_t size,void* usr)
{
        response* p = (response*)usr;
        p->append(data,n*size);

        return n * size;
}
